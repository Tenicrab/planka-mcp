#pragma once
#include <string>
#include <wfrest/Json.h>
#include <coke/coke.h>
#include <coke/http/http_client.h>

class PlankaClient {
public:
    struct Config {
        std::string url;
        std::string api_key;
    };

    explicit PlankaClient(const Config& config);
    
    static PlankaClient from_env();
    static std::string get_env(const std::string& name);

    // Core async requester
    coke::Task<wfrest::Json> get(const std::string& path);
    coke::Task<wfrest::Json> post(const std::string& path, const wfrest::Json& body);
    coke::Task<wfrest::Json> patch(const std::string& path, const wfrest::Json& body);
    coke::Task<wfrest::Json> del(const std::string& path);

private:
    Config config_;
    coke::HttpClient http_client_;
    
    std::string full_url(const std::string& path) const;
    void add_auth_headers(coke::HttpClient::HttpHeader& headers) const;
};
