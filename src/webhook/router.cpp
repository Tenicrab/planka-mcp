#include "router.h"
#include <regex>
#include <workflow/WFTaskFactory.h>
#include <workflow/HttpMessage.h>
#include <iostream>

namespace mcp {
namespace webhook {

void Router::set_forward_rule(const ForwardRule& rule) {
    rule_ = rule;
}

void Router::clear_forward_rule() {
    rule_.reset();
}

static std::string extract_value(const wfrest::Json& node, const std::vector<std::string>& keys, size_t index) {
    if (!node.is_valid() || !node.is_object() || !node.has(keys[index])) return "";
    wfrest::Json val = node[keys[index]];
    if (index == keys.size() - 1) {
        if (val.is_string()) return val.get<std::string>();
        if (val.is_number()) return val.dump();
        if (val.is_boolean()) return val.get<bool>() ? "true" : "false";
        return val.dump();
    }
    return extract_value(val, keys, index + 1);
}

static std::string get_json_value(const std::string& path, const wfrest::Json& data) {
    if (!data.is_valid()) return "";
    std::vector<std::string> keys;
    size_t start = 0;
    size_t end = path.find('.');
    while (end != std::string::npos) {
        keys.push_back(path.substr(start, end - start));
        start = end + 1;
        end = path.find('.', start);
    }
    keys.push_back(path.substr(start));
    if (keys.empty()) return "";
    return extract_value(data, keys, 0);
}

std::string render_template(const std::string& tmpl, const wfrest::Json& data) {
    std::regex re(R"(\{\{([^}]+)\}\})");
    std::string result = tmpl;
    std::smatch match;
    size_t offset = 0;
    while (std::regex_search(result.cbegin() + offset, result.cend(), match, re)) {
        std::string var_path = match[1];
        std::string replacement = get_json_value(var_path, data);
        result.replace(offset + match.position(), match.length(), replacement);
        offset += match.position() + replacement.length();
    }
    return result;
}

void Router::handle_event(const wfrest::Json& event) {
    if (!rule_) {
        // Fallback for SSE notifications could be hooked here
        return;
    }
    
    std::string payload;
    if (rule_->payload_template.empty()) {
        payload = event.dump();
    } else {
        payload = render_template(rule_->payload_template, event);
    }

    auto* task = WFTaskFactory::create_http_task(rule_->url, 0, 0, [](WFHttpTask* t){
        int state = t->get_state();
        int error = t->get_error();
        if (state != 0) {
            std::cerr << "Webhook Forward Error! State=" << state << " Error=" << error << "\n";
        }
    });

    auto* req = task->get_req();
    req->set_method("POST");
    req->add_header_pair("Content-Type", "application/json");
    for (const auto& kv : rule_->headers) {
        req->add_header_pair(kv.first, kv.second);
    }
    req->append_output_body(payload.c_str(), payload.size());
    task->start();
}

} // namespace webhook
} // namespace mcp
