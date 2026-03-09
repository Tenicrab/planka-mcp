#pragma once
#include <string>
#include <vector>
#include <wfrest/Json.h>
#include <coke/coke.h>
#include "planka/client.h"
#include "mcp/types.h"

namespace mcp {

struct ResourceTemplate {
    std::string uriTemplate;
    std::string name;
    std::string description;
    std::string mimeType;
};

class ResourceRegistry {
public:
    ResourceRegistry() = default;

    // resources/list
    std::vector<Resource> list_resources();

    // resources/templates/list
    std::vector<ResourceTemplate> list_templates();

    // resources/read
    coke::Task<wfrest::Json> read_resource(const std::string& uri, PlankaClient& client);
};

} // namespace mcp
