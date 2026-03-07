#pragma once
#include <string>
#include <vector>
#include <wfrest/Json.h>
#include <coke/coke.h>
#include "planka/client.h"
#include "mcp/types.h"

namespace mcp
{

struct ToolParam
{
    std::string name;
    std::string type;
    std::string description;
    bool required = true;
    std::vector<std::string> enum_values = {};
};

struct ToolDef
{
    std::string name;
    std::string description;
    std::string method;
    std::string path_template;
    std::vector<ToolParam> params;
    std::string item_type;          // e.g. "project", "card"
    std::string body_type = "json"; // "json" or "none"
};

class ToolRegistry
{
public:
    explicit ToolRegistry(PlankaClient& client);

    // tools/list
    std::vector<Tool> list_tools();

    // tools/call
    coke::Task<wfrest::Json> call_tool(const std::string& name, const wfrest::Json& arguments);

private:
    PlankaClient& client_;
    std::vector<ToolDef> definitions_;
    void init_definitions();

    // Helper to build schema from ToolDef
    wfrest::Json build_schema(const ToolDef& def);

    // Generic API caller based on ToolDef
    coke::Task<wfrest::Json> execute_generic(const ToolDef& def, const wfrest::Json& args);
};

} // namespace mcp
