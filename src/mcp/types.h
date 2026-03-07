#pragma once

#include <string>
#include <wfrest/Json.h>

namespace mcp {

struct Resource {
    std::string uri;
    std::string name;
    std::string description;
};

struct Tool {
    std::string name;
    std::string description;
    wfrest::Json inputSchema;
};

struct JsonRpcResponse {
    std::string jsonrpc = "2.0";
    wfrest::Json id;
    wfrest::Json result;
    wfrest::Json error;

    static JsonRpcResponse make_success(const wfrest::Json& id, const wfrest::Json& result);
    static JsonRpcResponse make_error(const wfrest::Json& id, int code, const std::string& message);
    
    std::string to_string() const;
};

} // namespace mcp
