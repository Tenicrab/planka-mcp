#pragma once
#include <string>
#include <vector>
#include <wfrest/Json.h>
#include <coke/coke.h>
#include "planka/client.h"
#include "mcp/types.h"

namespace mcp {

class ResourceRegistry {
public:
    explicit ResourceRegistry(PlankaClient& client);

    // resources/list
    std::vector<Resource> list_resources();

    // resources/read
    coke::Task<wfrest::Json> read_resource(const std::string& uri);

private:
    PlankaClient& client_;
};

} // namespace mcp
