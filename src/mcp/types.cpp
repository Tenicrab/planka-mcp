#include "types.h"
#include <wfrest/Json.h>

namespace mcp {

JsonRpcResponse JsonRpcResponse::make_success(const wfrest::Json& id, const wfrest::Json& result) {
    JsonRpcResponse resp;
    resp.id = id;
    resp.result = result;
    resp.error = nullptr;
    return resp;
}

JsonRpcResponse JsonRpcResponse::make_error(const wfrest::Json& id, int code, const std::string& message) {
    JsonRpcResponse resp;
    resp.id = id;
    resp.result = wfrest::Json(); // null
    wfrest::Json err = wfrest::Json::Object();
    err.push_back("code", code);
    err.push_back("message", message);
    resp.error = std::move(err);
    return resp;
}

std::string JsonRpcResponse::to_string() const {
    wfrest::Json j = wfrest::Json::Object();
    j.push_back("jsonrpc", "2.0");
    
    if (id.is_valid()) {
        j.push_back("id", id);
    } else {
        j.push_back("id", nullptr);
    }
    
    if (error.is_valid() && !error.is_null()) {
        j.push_back("error", error);
    } else if (result.is_valid()) {
        j.push_back("result", result);
    } else {
        j.push_back("result", nullptr);
    }
    return j.dump();
}

} // namespace mcp
