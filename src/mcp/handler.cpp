#include "handler.h"
#include "mcp/types.h"
#include "resources/registry.h"
#include "tools/registry.h"
#include <coke/coke.h>

namespace mcp {

McpHandler::McpHandler(PlankaClient& client, ResourceRegistry& registry, ToolRegistry& tool_registry) 
    : client_(client), registry_(registry), tool_registry_(tool_registry) {}

std::optional<std::string> McpHandler::handle_message(const std::string& json_str) {
    wfrest::Json req = wfrest::Json::parse(json_str);
    if (!req.is_valid()) {
        return JsonRpcResponse::make_error(wfrest::Json(), -32700, "Parse error").to_string();
    }
    
    if (!req.has("jsonrpc") || req["jsonrpc"].get<std::string>() != "2.0") {
         return JsonRpcResponse::make_error(wfrest::Json(), -32600, "Invalid Request").to_string();
    }
    
    std::string method;
    if (req.has("method") && req["method"].is_string()) {
        method = req["method"].get<std::string>();
    } else {
        return JsonRpcResponse::make_error(wfrest::Json(), -32600, "Invalid Request: missing method").to_string();
    }
    
    wfrest::Json id = req.has("id") ? req["id"] : wfrest::Json();

    if (method == "initialize") {
        wfrest::Json result = wfrest::Json::Object();
        result.push_back("protocolVersion", "2024-11-05");
        
        wfrest::Json serverInfo = wfrest::Json::Object();
        serverInfo.push_back("name", "planka-mcp");
        serverInfo.push_back("version", "1.0.0");
        result.push_back("serverInfo", serverInfo);
        
        wfrest::Json capabilities = wfrest::Json::Object();
        capabilities.push_back("resources", wfrest::Json::Object());
        capabilities.push_back("tools", wfrest::Json::Object());
        result.push_back("capabilities", capabilities);
        
        return JsonRpcResponse::make_success(id, result).to_string();
    } else if (method == "resources/list") {
        auto resources = registry_.list_resources();
        wfrest::Json result = wfrest::Json::Object();
        wfrest::Json res_list = wfrest::Json::Array();
        for (const auto& res : resources) {
            wfrest::Json r = wfrest::Json::Object();
            r.push_back("uri", res.uri);
            r.push_back("name", res.name);
            r.push_back("description", res.description);
            res_list.push_back(r);
        }
        result.push_back("resources", res_list);
        return JsonRpcResponse::make_success(id, result).to_string();
    } else if (method == "resources/read") {
        if (!req.has("params") || !req["params"].has("uri")) {
            return JsonRpcResponse::make_error(id, -32602, "Invalid params: missing uri").to_string();
        }
        std::string uri = req["params"]["uri"].get<std::string>();
        wfrest::Json contents = coke::sync_wait(registry_.read_resource(uri));
        if (contents.is_valid()) {
            wfrest::Json result = wfrest::Json::Object();
            result.push_back("contents", contents);
            return JsonRpcResponse::make_success(id, result).to_string();
        } else {
            return JsonRpcResponse::make_error(id, -32602, "Resource not found").to_string();
        }
    } else if (method == "tools/list") {
        auto tools = tool_registry_.list_tools();
        wfrest::Json result = wfrest::Json::Object();
        wfrest::Json tool_list = wfrest::Json::Array();
        for (const auto& t : tools) {
            wfrest::Json tj = wfrest::Json::Object();
            tj.push_back("name", t.name);
            tj.push_back("description", t.description);
            tj.push_back("inputSchema", t.inputSchema);
            tool_list.push_back(tj);
        }
        result.push_back("tools", tool_list);
        return JsonRpcResponse::make_success(id, result).to_string();
    } else if (method == "tools/call") {
        if (!req.has("params") || !req["params"].has("name")) {
            return JsonRpcResponse::make_error(id, -32602, "Invalid params: missing name").to_string();
        }
        std::string name = req["params"]["name"].get<std::string>();
        wfrest::Json args = req["params"].has("arguments") ? req["params"]["arguments"] : wfrest::Json::Object();
        
        wfrest::Json content = coke::sync_wait(tool_registry_.call_tool(name, args));
        if (content.is_valid()) {
            wfrest::Json result = wfrest::Json::Object();
            result.push_back("content", content);
            return JsonRpcResponse::make_success(id, result).to_string();
        } else {
            return JsonRpcResponse::make_error(id, -32601, "Tool not found or failed").to_string();
        }
    } else if (method == "ping") {
        return JsonRpcResponse::make_success(id, wfrest::Json::Object()).to_string();
    }
    
    if (!id.is_null()) {
        return JsonRpcResponse::make_error(id, -32601, "Method not found").to_string();
    }
    return std::nullopt;
}

} // namespace mcp
