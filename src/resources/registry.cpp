#include "registry.h"
#include <regex>
#include <string>
#include <wfrest/Json.h>

namespace mcp {

ResourceRegistry::ResourceRegistry(PlankaClient& client) : client_(client) {}

std::vector<Resource> ResourceRegistry::list_resources() {
    return {
        {"planka://projects", "Projects", "List of all Planka projects"},
        {"planka://users", "Users", "List of all users in Planka"},
        {"planka://notifications", "Notifications", "User notifications"},
        {"planka://webhooks", "Webhooks", "List of all system webhooks"}
    };
}

coke::Task<wfrest::Json> ResourceRegistry::read_resource(const std::string& uri) {
    if (uri == "planka://projects") {
        wfrest::Json response = co_await client_.get("/api/projects");
        
        wfrest::Json projects;
        if (response.is_array()) {
            projects = response;
        } else if (response.has("items") && response["items"].is_array()) {
            projects = response["items"];
        }

        wfrest::Json contents = wfrest::Json::Array();
        std::string text = "Planka Projects:\n";
        
        if (projects.is_array() && projects.size() > 0) {
            for (const auto& p : projects) {
                std::string id = p["id"].get<std::string>();
                std::string name = p["name"].get<std::string>();
                text += "- **" + name + "** (ID: `" + id + "`)\n";
                text += "  Use `planka://projects/" + id + "/boards` to list boards in this project.\n";
            }
        } else {
            text += "No projects found or API returned empty list.";
        }

        wfrest::Json item = wfrest::Json::Object();
        item.push_back("uri", uri);
        item.push_back("mimeType", "text/markdown");
        item.push_back("text", text);
        contents.push_back(item);
        
        co_return contents;
    }

    // Handle planka://projects/{id}/boards
    std::regex boards_regex(R"(planka://projects/([^/]+)/boards)");
    std::smatch match;
    if (std::regex_match(uri, match, boards_regex)) {
        std::string project_id = match[1];
        wfrest::Json response = co_await client_.get("/api/projects/" + project_id + "/boards");
        
        wfrest::Json boards;
        if (response.is_array()) {
            boards = response;
        } else if (response.has("items") && response["items"].is_array()) {
            boards = response["items"];
        }

        wfrest::Json contents = wfrest::Json::Array();
        std::string text = "Boards for project " + project_id + ":\n";
        
        if (boards.is_array() && boards.size() > 0) {
            for (const auto& b : boards) {
                std::string id = b["id"].get<std::string>();
                std::string name = b["name"].get<std::string>();
                text += "- **" + name + "** (ID: `" + id + "`)\n";
                // text += "  Use `planka://boards/" + id + "/lists` to see lists.\n";
            }
        } else {
            text += "No boards found in this project or API error.";
        }

        wfrest::Json item = wfrest::Json::Object();
        item.push_back("uri", uri);
        item.push_back("mimeType", "text/markdown");
        item.push_back("text", text);
        contents.push_back(item);
        
        co_return contents;
    }

    // Handle planka://boards/{id}/lists
    std::regex lists_regex(R"(planka://boards/([^/]+)/lists)");
    if (std::regex_match(uri, match, lists_regex)) {
        std::string board_id = match[1];
        wfrest::Json response = co_await client_.get("/api/boards/" + board_id + "/lists");
        
        wfrest::Json lists;
        if (response.is_array()) lists = response;
        else if (response.has("items")) lists = response["items"];

        wfrest::Json contents = wfrest::Json::Array();
        std::string text = "Lists for board " + board_id + ":\n";
        if (lists.is_array() && lists.size() > 0) {
            for (const auto& l : lists) {
                text += "- **" + l["name"].get<std::string>() + "** (ID: `" + l["id"].get<std::string>() + "`)\n";
            }
        } else {
            text += "No lists found.";
        }

        wfrest::Json item = wfrest::Json::Object();
        item.push_back("uri", uri);
        item.push_back("mimeType", "text/markdown");
        item.push_back("text", text);
        contents.push_back(item);
        co_return contents;
    }

    // Handle planka://boards/{id}/labels
    std::regex labels_regex(R"(planka://boards/([^/]+)/labels)");
    if (std::regex_match(uri, match, labels_regex)) {
        std::string board_id = match[1];
        wfrest::Json response = co_await client_.get("/api/boards/" + board_id + "/labels");
        wfrest::Json labels;
        if (response.is_array()) labels = response;
        else if (response.has("items")) labels = response["items"];

        wfrest::Json contents = wfrest::Json::Array();
        std::string text = "Labels for board " + board_id + ":\n";
        if (labels.is_array() && labels.size() > 0) {
            for (const auto& l : labels) {
                text += "- **" + l["name"].get<std::string>() + "** (Color: `" + l["color"].get<std::string>() + "`, ID: `" + l["id"].get<std::string>() + "`)\n";
            }
        } else {
            text += "No labels found.";
        }
        wfrest::Json item = wfrest::Json::Object();
        item.push_back("uri", uri);
        item.push_back("mimeType", "text/markdown");
        item.push_back("text", text);
        contents.push_back(item);
        co_return contents;
    }

    // Handle planka://cards/{id}/task-lists
    std::regex task_lists_regex(R"(planka://cards/([^/]+)/task-lists)");
    if (std::regex_match(uri, match, task_lists_regex)) {
        std::string card_id = match[1];
        wfrest::Json response = co_await client_.get("/api/cards/" + card_id + "/task-lists");
        wfrest::Json tl;
        if (response.is_array()) tl = response;
        else if (response.has("items")) tl = response["items"];

        wfrest::Json contents = wfrest::Json::Array();
        std::string text = "Task Lists for card " + card_id + ":\n";
        if (tl.is_array() && tl.size() > 0) {
            for (const auto& l : tl) {
                text += "- **" + l["name"].get<std::string>() + "** (ID: `" + l["id"].get<std::string>() + "`)\n";
                text += "  Use `planka://task-lists/" + l["id"].get<std::string>() + "/tasks` to see tasks.\n";
            }
        } else {
            text += "No task lists found.";
        }
        wfrest::Json item = wfrest::Json::Object();
        item.push_back("uri", uri);
        item.push_back("mimeType", "text/markdown");
        item.push_back("text", text);
        contents.push_back(item);
        co_return contents;
    }

    // Handle planka://task-lists/{id}/tasks
    std::regex tasks_regex(R"(planka://task-lists/([^/]+)/tasks)");
    if (std::regex_match(uri, match, tasks_regex)) {
        std::string tl_id = match[1];
        wfrest::Json response = co_await client_.get("/api/task-lists/" + tl_id + "/tasks");
        wfrest::Json tasks;
        if (response.is_array()) tasks = response;
        else if (response.has("items")) tasks = response["items"];

        wfrest::Json contents = wfrest::Json::Array();
        std::string text = "Tasks for task-list " + tl_id + ":\n";
        if (tasks.is_array() && tasks.size() > 0) {
            for (const auto& t : tasks) {
                std::string status = t["isCompleted"].get<bool>() ? "✅" : "⬜";
                text += status + " **" + t["name"].get<std::string>() + "** (ID: `" + t["id"].get<std::string>() + "`)\n";
            }
        } else {
            text += "No tasks found.";
        }
        wfrest::Json item = wfrest::Json::Object();
        item.push_back("uri", uri);
        item.push_back("mimeType", "text/markdown");
        item.push_back("text", text);
        contents.push_back(item);
        co_return contents;
    }

    // Handle planka://cards/{id}/comments
    std::regex comments_regex(R"(planka://cards/([^/]+)/comments)");
    if (std::regex_match(uri, match, comments_regex)) {
        std::string card_id = match[1];
        wfrest::Json response = co_await client_.get("/api/cards/" + card_id + "/comments");
        wfrest::Json comments;
        if (response.is_array()) comments = response;
        else if (response.has("items")) comments = response["items"];

        wfrest::Json contents = wfrest::Json::Array();
        std::string text = "Comments for card " + card_id + ":\n";
        if (comments.is_array() && comments.size() > 0) {
            for (const auto& c : comments) {
                text += "- " + c["text"].get<std::string>() + " (ID: `" + c["id"].get<std::string>() + "`)\n";
            }
        } else {
            text += "No comments found.";
        }
        wfrest::Json item = wfrest::Json::Object();
        item.push_back("uri", uri);
        item.push_back("mimeType", "text/markdown");
        item.push_back("text", text);
        contents.push_back(item);
        co_return contents;
    }

    // Handle planka://users
    if (uri == "planka://users") {
        wfrest::Json response = co_await client_.get("/api/users");
        wfrest::Json users;
        if (response.is_array()) users = response;
        else if (response.has("items")) users = response["items"];

        wfrest::Json contents = wfrest::Json::Array();
        std::string text = "Planka Users:\n";
        if (users.is_array() && users.size() > 0) {
            for (const auto& u : users) {
                std::string email = u.has("email") && !u["email"].is_null() ? u["email"].get<std::string>() : "No Email";
                std::string username = u.has("username") && !u["username"].is_null() ? u["username"].get<std::string>() : "No Username";
                text += "- **" + username + "** (" + email + ") - ID: `" + u["id"].get<std::string>() + "`\n";
            }
        } else {
            text += "No users found.";
        }
        wfrest::Json item = wfrest::Json::Object();
        item.push_back("uri", uri);
        item.push_back("mimeType", "text/markdown");
        item.push_back("text", text);
        contents.push_back(item);
        co_return contents;
    }

    // Handle planka://boards/{id}/actions
    std::regex b_actions_regex(R"(planka://boards/([^/]+)/actions)");
    if (std::regex_match(uri, match, b_actions_regex)) {
        std::string board_id = match[1];
        wfrest::Json response = co_await client_.get("/api/boards/" + board_id + "/actions");
        wfrest::Json actions;
        if (response.is_array()) actions = response;
        else if (response.has("items")) actions = response["items"];

        wfrest::Json contents = wfrest::Json::Array();
        std::string text = "Actions for board " + board_id + ":\n";
        if (actions.is_array() && actions.size() > 0) {
            for (const auto& a : actions) {
                std::string type = a.has("type") ? a["type"].get<std::string>() : "Unknown";
                text += "- Action: `" + type + "` (ID: `" + a["id"].get<std::string>() + "`)\n";
            }
        } else {
            text += "No actions found.";
        }
        wfrest::Json item = wfrest::Json::Object();
        item.push_back("uri", uri);
        item.push_back("mimeType", "text/markdown");
        item.push_back("text", text);
        contents.push_back(item);
        co_return contents;
    }

    // Handle planka://cards/{id}/actions
    std::regex c_actions_regex(R"(planka://cards/([^/]+)/actions)");
    if (std::regex_match(uri, match, c_actions_regex)) {
        std::string card_id = match[1];
        wfrest::Json response = co_await client_.get("/api/cards/" + card_id + "/actions");
        wfrest::Json actions;
        if (response.is_array()) actions = response;
        else if (response.has("items")) actions = response["items"];

        wfrest::Json contents = wfrest::Json::Array();
        std::string text = "Actions for card " + card_id + ":\n";
        if (actions.is_array() && actions.size() > 0) {
            for (const auto& a : actions) {
                std::string type = a.has("type") ? a["type"].get<std::string>() : "Unknown";
                text += "- Action: `" + type + "` (ID: `" + a["id"].get<std::string>() + "`)\n";
            }
        } else {
            text += "No actions found.";
        }
        wfrest::Json item = wfrest::Json::Object();
        item.push_back("uri", uri);
        item.push_back("mimeType", "text/markdown");
        item.push_back("text", text);
        contents.push_back(item);
        co_return contents;
    }

    // Handle planka://notifications
    if (uri == "planka://notifications") {
        wfrest::Json response = co_await client_.get("/api/notifications");
        wfrest::Json notifications;
        if (response.is_array()) notifications = response;
        else if (response.has("items")) notifications = response["items"];

        wfrest::Json contents = wfrest::Json::Array();
        std::string text = "Your Notifications:\n";
        if (notifications.is_array() && notifications.size() > 0) {
            for (const auto& n : notifications) {
                std::string read_status = n["isRead"].get<bool>() ? " (Read)" : " **(Unread)**";
                text += "- [" + n["createdAt"].get<std::string>() + "] ID: `" + n["id"].get<std::string>() + "`" + read_status + "\n";
            }
        } else {
            text += "No notifications found.";
        }
        wfrest::Json item = wfrest::Json::Object();
        item.push_back("uri", uri);
        item.push_back("mimeType", "text/markdown");
        item.push_back("text", text);
        contents.push_back(item);
        co_return contents;
    }

    // Handle planka://webhooks
    if (uri == "planka://webhooks") {
        wfrest::Json response = co_await client_.get("/api/webhooks");
        wfrest::Json webhooks;
        if (response.is_array()) webhooks = response;
        else if (response.has("items")) webhooks = response["items"];

        wfrest::Json contents = wfrest::Json::Array();
        std::string text = "System Webhooks:\n";
        if (webhooks.is_array() && webhooks.size() > 0) {
            for (const auto& w : webhooks) {
                text += "- **" + w["name"].get<std::string>() + "** (URL: `" + w["url"].get<std::string>() + "`) - ID: `" + w["id"].get<std::string>() + "`\n";
            }
        } else {
            text += "No webhooks found.";
        }
        wfrest::Json item = wfrest::Json::Object();
        item.push_back("uri", uri);
        item.push_back("mimeType", "text/markdown");
        item.push_back("text", text);
        contents.push_back(item);
        co_return contents;
    }

    co_return wfrest::Json(); 
}

} // namespace mcp
