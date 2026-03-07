#pragma once
#include <wfrest/Json.h>
#include <string>
#include <map>
#include <optional>

namespace mcp {
namespace webhook {

struct ForwardRule {
    std::string url;
    std::map<std::string, std::string> headers;
    std::string payload_template;
};

class Router {
public:
    static Router& instance() {
        static Router instance;
        return instance;
    }

    void set_forward_rule(const ForwardRule& rule);
    void clear_forward_rule();

    void handle_event(const wfrest::Json& event);

private:
    std::optional<ForwardRule> rule_;
};

std::string render_template(const std::string& tmpl, const wfrest::Json& data);

} // namespace webhook
} // namespace mcp
