#pragma once
#include <string>
#include <wfrest/Json.h>

namespace planka {

struct Project {
    std::string id;
    std::string name;
    std::string description;
    
    static Project from_json(const wfrest::Json& j) {
        return {
            j["id"].get<std::string>(),
            j["name"].get<std::string>(),
            j.has("description") && j["description"].is_string() ? j["description"].get<std::string>() : ""
        };
    }
    
    wfrest::Json to_json() const {
        wfrest::Json j = wfrest::Json::Object();
        j.push_back("id", id);
        j.push_back("name", name);
        j.push_back("description", description);
        return j;
    }
};

struct Board {
    std::string id;
    std::string projectId;
    std::string name;
    // ... we can add more fields like type, position when needed
    
    static Board from_json(const wfrest::Json& j) {
        return {
            j["id"].get<std::string>(),
            j["projectId"].get<std::string>(),
            j["name"].get<std::string>()
        };
    }
};

// 后续根据 Phase 2 进度会继续扩充 List, Card 等模型

} // namespace planka
