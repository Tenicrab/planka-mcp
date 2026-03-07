#include "client.h"
#include <cstdlib>
#include <workflow/HttpMessage.h>

PlankaClient::PlankaClient(const Config& config) : config_(config) {
    if (!config_.url.empty() && config_.url.back() == '/') {
        config_.url.pop_back();
    }
}

PlankaClient PlankaClient::from_env() {
    Config conf;
    const char* url = std::getenv("PLANKA_URL");
    const char* token = std::getenv("PLANKA_TOKEN");
    
    if (url) conf.url = url;
    if (token) conf.token = token;
    
    return PlankaClient(conf);
}

std::string PlankaClient::full_url(const std::string& path) const {
    std::string p = path;
    if (p.empty() || p[0] != '/') {
        p = "/" + p;
    }
    return config_.url + p;
}

void PlankaClient::add_auth_headers(coke::HttpClient::HttpHeader& headers) const {
    if (!config_.token.empty()) {
        headers.push_back({"X-Api-Key", config_.token});
    }
    headers.push_back({"Content-Type", "application/json"});
}

static std::string get_body(const protocol::HttpResponse& resp) {
    const void *body;
    size_t size;
    if (resp.get_parsed_body(&body, &size)) {
        return std::string(static_cast<const char*>(body), size);
    }
    return "";
}

coke::Task<wfrest::Json> PlankaClient::get(const std::string& path) {
    coke::HttpClient::HttpHeader headers;
    add_auth_headers(headers);
    
    std::string url = full_url(path);
    fprintf(stderr, "DEBUG REQ GET: %s\n", url.c_str());
    for(auto& h : headers) fprintf(stderr, "DEBUG HEADER: %s: %s\n", h.first.c_str(), h.second.c_str());

    auto result = co_await http_client_.request(url, "GET", headers, "");
    
    if (result.state == coke::STATE_SUCCESS) {
        co_return wfrest::Json::parse(get_body(result.resp));
    }
    co_return wfrest::Json();
}

coke::Task<wfrest::Json> PlankaClient::post(const std::string& path, const wfrest::Json& body) {
    coke::HttpClient::HttpHeader headers;
    add_auth_headers(headers);
    
    auto result = co_await http_client_.request(full_url(path), "POST", headers, body.dump());
    
    if (result.state == coke::STATE_SUCCESS) {
        co_return wfrest::Json::parse(get_body(result.resp));
    }
    co_return wfrest::Json();
}

coke::Task<wfrest::Json> PlankaClient::patch(const std::string& path, const wfrest::Json& body) {
    coke::HttpClient::HttpHeader headers;
    add_auth_headers(headers);
    
    auto result = co_await http_client_.request(full_url(path), "PATCH", headers, body.dump());
    
    if (result.state == coke::STATE_SUCCESS) {
        co_return wfrest::Json::parse(get_body(result.resp));
    }
    co_return wfrest::Json();
}

coke::Task<wfrest::Json> PlankaClient::del(const std::string& path) {
    coke::HttpClient::HttpHeader headers;
    add_auth_headers(headers);
    
    auto result = co_await http_client_.request(full_url(path), "DELETE", headers, "");
    
    if (result.state == coke::STATE_SUCCESS) {
        co_return wfrest::Json::parse(get_body(result.resp));
    }
    co_return wfrest::Json();
}
