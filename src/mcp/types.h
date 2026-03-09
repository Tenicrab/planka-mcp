#pragma once

#include <string>
#include <wfrest/Json.h>

namespace mcp {

struct Resource {
    std::string uri;
    std::string name;
    std::string description;
    std::string mimeType;
};

struct FieldDef {
    std::string name;
    std::string description;
    std::string type;
    bool required;
    std::vector<std::string> options = {};
};

struct ToolDef {
    std::string name;
    std::string description;
    std::string method;
    std::string path;
    std::vector<FieldDef> fields;
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
