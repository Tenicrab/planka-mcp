#include "registry.h"
#include <regex>
#include <string>
#include <wfrest/Json.h>
#include <unordered_set>
#include <logger.h>

namespace mcp {

static std::string safe_get_string(const wfrest::Json& j, const std::string& key, const std::string& def = "") {
    if (j.has(key)) {
        if (j[key].is_string()) return j[key].get<std::string>();
        return j[key].dump();
    }
    return def;
}

static bool is_planka_error(const wfrest::Json& j) {
    if (!j.is_object()) return false;
    if (j.has("code") && j["code"].is_string()) {
        std::string code = j["code"].get<std::string>();
        // Planka v2 errors typically start with E_
        return code.find("E_") == 0;
    }
    return false;
}

std::vector<Resource> ResourceRegistry::list_resources() {
    return {
        {"planka://hub", "Planka Hub", "Global dashboard summary", "application/json"},
        {"planka://me", "My Profile", "Current user account details", "application/json"},
        {"planka://projects", "Projects", "List of all Planka projects", "application/json"},
        {"planka://users", "Users", "List of all site users", "application/json"},
        {"planka://notifications", "Notifications", "Personal inbox", "application/json"},
        {"planka://webhooks", "System Webhooks", "Admin: System-wide webhooks", "application/json"}
    };
}

std::vector<ResourceTemplate> ResourceRegistry::list_templates() {
    return {
        {"planka://hub", "System Hub", "Overview of projects, users, and activity", "application/json"},
        {"planka://me", "Current User", "Details of the authenticated user", "application/json"},
        {"planka://projects", "All Projects", "List of all accessible projects", "application/json"},
        {"planka://projects/{id}", "Project Summary", "Metadata + Board list", "application/json"},
        {"planka://projects/{id}/details", "Project Full Data", "Raw JSON project record", "application/json"},
        {"planka://boards/{id}", "Board Summary", "Metadata + List overview + Available Labels", "application/json"},
        {"planka://boards/{id}/details", "Board Full Data", "Raw JSON board record", "application/json"},
        {"planka://boards/{id}/lists", "Board Hierarchical View", "JSON tree of lists -> cards", "application/json"},
        {"planka://boards/{id}/actions", "Board Activity Log", "Recent actions on the board", "application/json"},
        {"planka://lists/{id}", "List Metadata", "Specific list record", "application/json"},
        {"planka://lists/{id}/cards", "List Cards", "Array of card items in list", "application/json"},
        {"planka://cards/{id}", "Card Summary", "Metadata + Tasks + Hint", "application/json"},
        {"planka://cards/{id}/details", "Card Full Data", "Raw JSON card record (with inclusions)", "application/json"},
        {"planka://cards/{id}/comments", "Card Comments", "Comment thread for a card", "application/json"},
        {"planka://cards/{id}/attachments", "Card Attachments", "List of files and links on card", "application/json"},
        {"planka://users", "Directory", "List of all users on the instance", "application/json"},
        {"planka://notifications", "Inbox", "Recent personal notifications", "application/json"},
        {"planka://webhooks", "Webhooks (Admin)", "Global webhook configurations", "application/json"}
    };
}

coke::Task<wfrest::Json> ResourceRegistry::read_resource(const std::string& uri, PlankaClient& client) {
    LOG_DEBUG() << "[ResourceRegistry] Reading URI: " << uri;
    std::smatch match;

    auto wrap_content = [&](const std::string& u, const std::string& text) {
        wfrest::Json contents = wfrest::Json::Array();
        wfrest::Json item = wfrest::Json::Object();
        item.push_back("uri", u);
        item.push_back("mimeType", "application/json");
        item.push_back("text", text);
        contents.push_back(item);
        return contents;
    };

    static std::regex hub_re(R"(planka://hub/?)");
    if (std::regex_match(uri, match, hub_re)) {
        wfrest::Json hub = wfrest::Json::Object();
        auto projs = co_await client.get("/api/projects");
        auto users = co_await client.get("/api/users");
        auto notifs = co_await client.get("/api/notifications");
        
        hub.push_back("projectsCount", projs.is_array() ? projs.size() : (projs.has("items") ? projs["items"].size() : 0));
        hub.push_back("usersCount", users.is_array() ? users.size() : (users.has("items") ? users["items"].size() : 0));
        hub.push_back("unreadNotifications", notifs.is_array() ? notifs.size() : 0);
        
        hub.push_back("_hint", "This is a global overview. Start exploring with planka://projects for deeper data.");
        co_return wrap_content(uri, hub.dump());
    }

    static std::regex me_re(R"(planka://me/?)");
    if (std::regex_match(uri, match, me_re)) {
        // Find self in users list by checking who we are (often first in some lists or via notifications)
        auto users = co_await client.get("/api/users");
        auto notifs = co_await client.get("/api/notifications");
        
        wfrest::Json me = wfrest::Json::Object();
        me.push_back("note", "Planka v2 doesn't have a direct /me endpoint. Listing recent notification context.");
        if (notifs.is_array() && !notifs.empty()) {
             me.push_back("recentNotification", notifs[0]);
        }
        co_return wrap_content(uri, me.dump());
    }

    static std::regex projects_base_re(R"(planka://projects/?)");
    if (std::regex_match(uri, match, projects_base_re)) {
        wfrest::Json response = co_await client.get("/api/projects");
        if (!response.is_valid() || is_planka_error(response)) co_return wfrest::Json::Array();
        
        wfrest::Json projects;
        if (response.is_array()) projects = response;
        else if (response.has("items")) projects = response["items"];
        co_return wrap_content(uri, projects.dump());
    }

    static std::regex users_base_re(R"(planka://users/?)");
    if (std::regex_match(uri, match, users_base_re)) {
        wfrest::Json response = co_await client.get("/api/users");
        if (!response.is_valid() || is_planka_error(response)) co_return wfrest::Json::Array();
        wfrest::Json users;
        if (response.is_array()) users = response;
        else if (response.has("items")) users = response["items"];
        co_return wrap_content(uri, users.dump());
    }

    static std::regex notif_base_re(R"(planka://notifications/?)");
    if (std::regex_match(uri, match, notif_base_re)) {
        wfrest::Json response = co_await client.get("/api/notifications");
        if (!response.is_valid()) co_return wfrest::Json::Array();
        co_return wrap_content(uri, response.dump());
    }

    static std::regex webhooks_base_re(R"(planka://webhooks/?)");
    if (std::regex_match(uri, match, webhooks_base_re)) {
        wfrest::Json response = co_await client.get("/api/webhooks");
        if (!response.is_valid()) co_return wfrest::Json::Array();
        co_return wrap_content(uri, response.dump());
    }

    // Match specific boards for a project (JSON)
    static std::regex boards_re(R"(planka://projects/([0-9]+)/boards/?)");
    if (std::regex_match(uri, match, boards_re)) {
        std::string pid = match[1];
        wfrest::Json p = co_await client.get("/api/projects/" + pid);
        if (!p.is_valid() || is_planka_error(p)) co_return wfrest::Json::Array();
        
        wfrest::Json boards = wfrest::Json::Array();
        if (p.has("included") && p["included"].has("boards")) {
            boards = p["included"]["boards"];
        }
        co_return wrap_content(uri, boards.dump());
    }

    // Match project detail
    static std::regex project_re(R"(planka://projects/([0-9]+)(/details)?/?)");
    if (std::regex_match(uri, match, project_re)) {
        std::string pid = match[1];
        bool is_details = uri.find("/details") != std::string::npos;
        
        LOG_DEBUG() << "[ResourceRegistry] Reading project " << (is_details ? "details" : "summary") << ": " << pid;
        wfrest::Json p = co_await client.get("/api/projects/" + pid);
        if (!p.is_valid() || is_planka_error(p)) co_return wfrest::Json::Array();
        
        if (is_details) {
            co_return wrap_content(uri, p.dump());
        } else {
            // Highly aggregated summary for AI
            wfrest::Json summary = wfrest::Json::Object();
            summary.push_back("id", safe_get_string(p["item"], "id"));
            summary.push_back("name", safe_get_string(p["item"], "name"));
            
            wfrest::Json boards_summary = wfrest::Json::Array();
            if (p.has("included") && p["included"].has("boards")) {
                for (const auto& b : p["included"]["boards"]) {
                    wfrest::Json bj = wfrest::Json::Object();
                    std::string bid = safe_get_string(b, "id");
                    bj.push_back("id", bid);
                    bj.push_back("name", safe_get_string(b, "name"));
                    
                    // Count lists for this board
                    int list_count = 0;
                    if (p["included"].has("lists")) {
                        for (const auto& l : p["included"]["lists"]) {
                            if (safe_get_string(l, "boardId") == bid) list_count++;
                        }
                    }
                    bj.push_back("listsCount", list_count);
                    boards_summary.push_back(bj);
                }
            }
            summary.push_back("boards", boards_summary);
            co_return wrap_content(uri, summary.dump());
        }
    }
    // Card Sub-resources (Task Lists, Comments, Actions, Custom Fields, Attachments)
    static std::regex card_sub_re(R"(planka://cards/([0-9]+)/(task-lists|comments|actions|custom-field-groups|attachments)/?)");
    if (std::regex_match(uri, match, card_sub_re)) {
        std::string cid = match[1];
        std::string sub = match[2];
        LOG_DEBUG() << "[ResourceRegistry] Reading card " << sub << " for card: " << cid;
        wfrest::Json response = co_await client.get("/api/cards/" + cid + "/" + sub);
        if (!response.is_valid() || is_planka_error(response)) co_return wfrest::Json::Array();

        co_return wrap_content(uri, response.dump());
    }
    
    // Task-list sub-resources (Tasks)
    static std::regex tasklist_sub_re(R"(planka://task-lists/([0-9]+)/tasks/?)");
    if (std::regex_match(uri, match, tasklist_sub_re)) {
        std::string tlid = match[1];
        LOG_DEBUG() << "[ResourceRegistry] Reading tasks for task-list: " << tlid;
        wfrest::Json response = co_await client.get("/api/task-lists/" + tlid + "/tasks");
        if (!response.is_valid() || is_planka_error(response)) co_return wfrest::Json::Array();

        co_return wrap_content(uri, response.dump());
    }

    // Board Lists Summary (JSON)
    static std::regex board_lists_re(R"(planka://boards/([0-9]+)/lists/?)");
    if (std::regex_match(uri, match, board_lists_re)) {
        std::string bid = match[1];
        wfrest::Json b = co_await client.get("/api/boards/" + bid);
        if (!b.is_valid() || is_planka_error(b)) co_return wfrest::Json::Array();
        
        wfrest::Json summary = wfrest::Json::Object();
        summary.push_back("boardId", bid);
        summary.push_back("boardName", safe_get_string(b["item"], "name"));
        
        wfrest::Json lists_summary = wfrest::Json::Array();
        if (b.has("included") && b["included"].has("lists")) {
            for (const auto& l : b["included"]["lists"]) {
                wfrest::Json lj = wfrest::Json::Object();
                std::string lid = safe_get_string(l, "id");
                lj.push_back("id", lid);
                lj.push_back("name", safe_get_string(l, "name"));
                
                // Count cards
                int card_count = 0;
                if (b["included"].has("cards")) {
                    for (const auto& c : b["included"]["cards"]) {
                        if (safe_get_string(c, "listId") == lid) card_count++;
                    }
                }
                lj.push_back("cardsCount", card_count);
                lists_summary.push_back(lj);
            }
        }
        summary.push_back("lists", lists_summary);

        co_return wrap_content(uri, summary.dump());
    }

    // List Cards (JSON View)
    static std::regex list_cards_re(R"(planka://lists/([0-9]+)/cards/?)");
    if (std::regex_match(uri, match, list_cards_re)) {
        std::string lid = match[1];
        wfrest::Json response = co_await client.get("/api/lists/" + lid + "/cards");
        if (!response.is_valid() || is_planka_error(response)) co_return wfrest::Json::Array();

        wfrest::Json cards;
        if (response.is_array()) cards = response;
        else if (response.has("items")) cards = response["items"];

        co_return wrap_content(uri, cards.dump());
    }

    // Boards Detail & Summary
    static std::regex board_re(R"(planka://boards/([0-9]+)(/details)?/?)");
    if (std::regex_match(uri, match, board_re)) {
        std::string bid = match[1];
        bool is_details = uri.find("/details") != std::string::npos;
        
        LOG_DEBUG() << "[ResourceRegistry] Reading board " << (is_details ? "details" : "summary") << ": " << bid;
        wfrest::Json b = co_await client.get("/api/boards/" + bid);
        if (!b.is_valid() || is_planka_error(b)) co_return wfrest::Json::Array();
        
        if (is_details) {
            co_return wrap_content(uri, b.dump());
        } else {
            wfrest::Json summary = wfrest::Json::Object();
            summary.push_back("id", safe_get_string(b["item"], "id"));
            summary.push_back("name", safe_get_string(b["item"], "name"));
            
            wfrest::Json lists_summary = wfrest::Json::Array();
            if (b.has("included") && b["included"].has("lists")) {
                for (const auto& l : b["included"]["lists"]) {
                    wfrest::Json lj = wfrest::Json::Object();
                    std::string lid = safe_get_string(l, "id");
                    lj.push_back("id", lid);
                    lj.push_back("name", safe_get_string(l, "name"));
                    
                    int card_count = 0;
                    if (b["included"].has("cards")) {
                        for (const auto& c : b["included"]["cards"]) {
                            if (safe_get_string(c, "listId") == lid) card_count++;
                        }
                    }
                    lj.push_back("cardsCount", card_count);
                    lists_summary.push_back(lj);
                }
            }
            summary.push_back("lists", lists_summary);
            
            wfrest::Json labels_options = wfrest::Json::Array();
            if (b.has("included") && b["included"].has("labels")) {
                for (const auto& l : b["included"]["labels"]) {
                    wfrest::Json lj = wfrest::Json::Object();
                    lj.push_back("id", safe_get_string(l, "id"));
                    lj.push_back("name", safe_get_string(l, "name"));
                    lj.push_back("color", safe_get_string(l, "color"));
                    labels_options.push_back(lj);
                }
            }
            summary.push_back("availableLabels", labels_options);
            summary.push_back("_hint", "Use planka_action with add_card_label or remove_card_label to manage card labels chosen from availableLabels.");
            
            co_return wrap_content(uri, summary.dump());
        }
    }

    // Board Sub-resources (Labels, Actions, Custom Fields)
    static std::regex board_sub_re(R"(planka://boards/([0-9]+)/(labels|actions|custom-field-groups)/?)");
    if (std::regex_match(uri, match, board_sub_re)) {
        std::string bid = match[1];
        std::string sub = match[2];
        LOG_DEBUG() << "[ResourceRegistry] Reading board " << sub << " for board: " << bid;
        wfrest::Json response = co_await client.get("/api/boards/" + bid + "/" + sub);
        if (!response.is_valid() || is_planka_error(response)) co_return wfrest::Json::Array();

        co_return wrap_content(uri, response.dump());
    }

    // Lists Detail
    static std::regex list_re(R"(planka://lists/([0-9]+)/?)");
    if (std::regex_match(uri, match, list_re)) {
        std::string lid = match[1];
        LOG_DEBUG() << "[ResourceRegistry] Reading list: " << lid;
        wfrest::Json l = co_await client.get("/api/lists/" + lid);
        if (!l.is_valid() || is_planka_error(l)) co_return wfrest::Json::Array();
        
        co_return wrap_content(uri, l.dump());
    }

    // Cards Detail & Summary
    static std::regex card_re(R"(planka://cards/([0-9]+)(/details)?/?)");
    if (std::regex_match(uri, match, card_re)) {
        std::string cid = match[1];
        bool is_details = uri.find("/details") != std::string::npos;
        static const std::unordered_set<std::string> blacklist = {"task-lists", "comments", "actions", "custom-field-groups"};
        
        if (blacklist.find(cid) == blacklist.end()) {
            LOG_DEBUG() << "[ResourceRegistry] Reading card " << (is_details ? "details" : "summary") << ": " << cid;
            wfrest::Json c = co_await client.get("/api/cards/" + cid);
            if (!c.is_valid() || is_planka_error(c)) co_return wfrest::Json::Array();
            
            if (is_details) {
                co_return wrap_content(uri, c.dump());
            } else {
                wfrest::Json summary = wfrest::Json::Object();
                summary.push_back("id", safe_get_string(c["item"], "id"));
                summary.push_back("name", safe_get_string(c["item"], "name"));
                summary.push_back("description", safe_get_string(c["item"], "description"));
                summary.push_back("_hint", "Description supports Markdown formatting. Mention users using @username. Use board-level availableLabels for card labeling.");
                
                if (c["item"].has("dueDate")) summary.push_back("dueDate", c["item"]["dueDate"]);
                if (c["item"].has("isDueCompleted")) summary.push_back("isDueCompleted", c["item"]["isDueCompleted"]);
                if (c["item"].has("stopwatch")) summary.push_back("stopwatch", c["item"]["stopwatch"]);

                wfrest::Json tasks = wfrest::Json::Array();
                if (c.has("included") && c["included"].has("tasks")) {
                    for (const auto& t : c["included"]["tasks"]) {
                        wfrest::Json tj = wfrest::Json::Object();
                        tj.push_back("id", safe_get_string(t, "id"));
                        tj.push_back("name", safe_get_string(t, "name"));
                        tj.push_back("isCompleted", safe_get_string(t, "isCompleted") == "true" || (t.has("isCompleted") && t["isCompleted"].get<bool>()));
                        tj.push_back("assigneeUserId", safe_get_string(t, "assigneeUserId"));
                        tasks.push_back(tj);
                    }
                }
                summary.push_back("tasks", tasks);
                co_return wrap_content(uri, summary.dump());
            }
        }
    }

    // Users Detail
    static std::regex user_re(R"(planka://users/([0-9]+)/?)");
    if (std::regex_match(uri, match, user_re)) {
        std::string uid = match[1];
        LOG_DEBUG() << "[ResourceRegistry] Reading user: " << uid;
        wfrest::Json u = co_await client.get("/api/users/" + uid);
        if (!u.is_valid() || is_planka_error(u)) co_return wfrest::Json::Array();
        
        co_return wrap_content(uri, u.dump());
    }

    // Default fallthrough
    LOG_WARN() << "[ResourceRegistry] No match for: " << uri;
    co_return wfrest::Json::Array(); 
}

} // namespace mcp
