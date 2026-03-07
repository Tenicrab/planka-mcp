#include "registry.h"
#include "../webhook/router.h"

namespace mcp
{

ToolRegistry::ToolRegistry(PlankaClient& client) : client_(client)
{
    init_definitions();
}

void ToolRegistry::init_definitions()
{
    // =========================================================================
    // 1. Projects
    // =========================================================================
    definitions_.push_back({"create_project", "Create a project", "POST", "/api/projects", {{"name", "string", "Project name"}, {"type", "string", "Project type", false, {"private", "shared"}}}, "project"});
    definitions_.push_back({"delete_project", "Delete a project", "DELETE", "/api/projects/{id}", {{"id", "string", "Project ID"}}, "project"});
    definitions_.push_back({"update_project", "Update a project", "PATCH", "/api/projects/{id}", {{"id", "string", "Project ID"}, {"name", "string", "New name", false}, {"description", "string", "New description", false}}, "project"});
    definitions_.push_back({"add_project_manager", "Add project manager", "POST", "/api/projects/{projectId}/project-managers", {{"projectId", "string", "Project ID"}, {"userId", "string", "User ID"}}, "manager"});
    definitions_.push_back({"remove_project_manager", "Remove project manager", "DELETE", "/api/project-managers/{id}", {{"id", "string", "Manager ID"}}, "manager"});

    // =========================================================================
    // 2. Boards
    // =========================================================================
    definitions_.push_back({"create_board", "Create a board", "POST", "/api/projects/{projectId}/boards", {{"projectId", "string", "Project ID"}, {"name", "string", "Board name"}, {"position", "number", "Position", false}}, "board"});
    definitions_.push_back({"delete_board", "Delete a board", "DELETE", "/api/boards/{id}", {{"id", "string", "Board ID"}}, "board"});
    definitions_.push_back({"update_board", "Update a board", "PATCH", "/api/boards/{id}", {{"id", "string", "Board ID"}, {"name", "string", "New name", false}, {"position", "number", "New position", false}}, "board"});
    definitions_.push_back({"create_board_membership", "Add member to board", "POST", "/api/boards/{boardId}/board-memberships", {{"boardId", "string", "Board ID"}, {"userId", "string", "User ID"}, {"role", "string", "Role", false, {"editor", "viewer"}}}, "membership"});
    definitions_.push_back({"update_board_membership", "Update board membership", "PATCH", "/api/board-memberships/{id}", {{"id", "string", "Membership ID"}, {"role", "string", "Role", true, {"editor", "viewer"}}}, "membership"});
    definitions_.push_back({"delete_board_membership", "Remove board member", "DELETE", "/api/board-memberships/{id}", {{"id", "string", "Membership ID"}}, "membership"});

    // =========================================================================
    // 3. Lists
    // =========================================================================
    definitions_.push_back({"create_list", "Create a list", "POST", "/api/boards/{boardId}/lists", {{"boardId", "string", "Board ID"}, {"name", "string", "List name"}, {"type", "string", "Type", false, {"active", "closed"}}, {"position", "number", "Position", false}}, "list"});
    definitions_.push_back({"delete_list", "Delete a list", "DELETE", "/api/lists/{id}", {{"id", "string", "List ID"}}, "list"});
    definitions_.push_back({"update_list", "Update a list", "PATCH", "/api/lists/{id}", {{"id", "string", "List ID"}, {"name", "string", "New name", false}, {"position", "number", "New position", false}}, "list"});
    definitions_.push_back({"clear_list", "Clear all cards from list", "POST", "/api/lists/{id}/clear", {{"id", "string", "List ID"}}, "list", "none"});
    definitions_.push_back({"sort_list", "Sort cards in list", "POST", "/api/lists/{id}/sort", {{"id", "string", "List ID"}}, "list", "none"});
    definitions_.push_back({"move_cards_in_list", "Move all cards to another list", "POST", "/api/lists/{id}/move-cards", {{"id", "string", "Source list ID"}, {"listId", "string", "Target list ID"}}, "list", "none"});

    // =========================================================================
    // 4. Cards
    // =========================================================================
    definitions_.push_back({"create_card", "Create a card", "POST", "/api/lists/{listId}/cards", {{"listId", "string", "List ID"}, {"name", "string", "Card name"}, {"type", "string", "Card type", false, {"project", "story"}}, {"description", "string", "Optional description", false}, {"position", "number", "Position", false}}, "card"});
    definitions_.push_back({"delete_card", "Delete a card", "DELETE", "/api/cards/{id}", {{"id", "string", "Card ID"}}, "card"});
    definitions_.push_back({"update_card", "Update a card", "PATCH", "/api/cards/{id}", {{"id", "string", "Card ID"}, {"name", "string", "New name", false}, {"description", "string", "New description", false}, {"position", "number", "New position", false}, {"isDueCompleted", "boolean", "Due completed", false}}, "card"});
    definitions_.push_back({"duplicate_card", "Duplicate a card", "POST", "/api/cards/{id}/duplicate", {{"id", "string", "Card ID"}, {"position", "number", "New position", false}}, "card"});
    definitions_.push_back({"move_card", "Move a card", "PATCH", "/api/cards/{id}", {{"id", "string", "Card ID"}, {"listId", "string", "New list ID"}, {"position", "number", "New position", false}}, "card"});
    definitions_.push_back({"add_card_member", "Add user as card member", "POST", "/api/cards/{cardId}/card-memberships", {{"cardId", "string", "Card ID"}, {"userId", "string", "User ID"}}, "membership"});
    definitions_.push_back({"remove_card_member", "Remove card member", "DELETE", "/api/cards/{cardId}/card-memberships/userId:{userId}", {{"cardId", "string", "Card ID"}, {"userId", "string", "User ID"}}, "membership", "none"});
    definitions_.push_back({"read_card_notifications", "Mark specific card notifications read", "POST", "/api/cards/{id}/read-notifications", {{"id", "string", "Card ID"}}, "notification", "none"});

    // =========================================================================
    // 5. Labels
    // =========================================================================
    definitions_.push_back({"create_label", "Create a label", "POST", "/api/boards/{boardId}/labels", {{"boardId", "string", "Board ID"}, {"name", "string", "Label name"}, {"color", "string", "Color", false, {"berry-red", "orange-peel", "egg-yellow", "fresh-salad", "midnight-blue", "lilac-eyes", "apricot-red"}}, {"position", "number", "Position", false}}, "label"});
    definitions_.push_back({"update_label", "Update label details", "PATCH", "/api/labels/{id}", {{"id", "string", "Label ID"}, {"name", "string", "New name", false}, {"color", "string", "New color", false, {"berry-red", "orange-peel", "egg-yellow", "fresh-salad", "midnight-blue", "lilac-eyes", "apricot-red"}}}, "label"});
    definitions_.push_back({"delete_label", "Delete a label", "DELETE", "/api/labels/{id}", {{"id", "string", "Label ID"}}, "label"});
    definitions_.push_back({"add_card_label", "Add label to card", "POST", "/api/cards/{cardId}/card-labels", {{"cardId", "string", "Card ID"}, {"labelId", "string", "Label ID"}}, "label"});
    definitions_.push_back({"remove_card_label", "Remove label from card", "DELETE", "/api/cards/{cardId}/card-labels/labelId:{labelId}", {{"cardId", "string", "Card ID"}, {"labelId", "string", "Label ID"}}, "label", "none"});

    // =========================================================================
    // 6. Task Lists & Tasks
    // =========================================================================
    definitions_.push_back({"create_task_list", "Create task list", "POST", "/api/cards/{cardId}/task-lists", {{"cardId", "string", "Card ID"}, {"name", "string", "Name"}, {"position", "number", "Position", false}}, "task-list"});
    definitions_.push_back({"update_task_list", "Update task list", "PATCH", "/api/task-lists/{id}", {{"id", "string", "Task list ID"}, {"name", "string", "New name", false}, {"position", "number", "New position", false}}, "task-list"});
    definitions_.push_back({"delete_task_list", "Delete a task list", "DELETE", "/api/task-lists/{id}", {{"id", "string", "Task list ID"}}, "task-list"});
    definitions_.push_back({"create_task", "Create task", "POST", "/api/task-lists/{taskListId}/tasks", {{"taskListId", "string", "Task List ID"}, {"name", "string", "Name"}, {"position", "number", "Position", false}}, "task"});
    definitions_.push_back({"update_task", "Update task", "PATCH", "/api/tasks/{id}", {{"id", "string", "Task ID"}, {"name", "string", "Name", false}, {"isCompleted", "boolean", "Completed", false}, {"position", "number", "New position", false}}, "task"});
    definitions_.push_back({"delete_task", "Delete a task", "DELETE", "/api/tasks/{id}", {{"id", "string", "Task ID"}}, "task"});

    // =========================================================================
    // 7. Comments
    // =========================================================================
    definitions_.push_back({"create_comment", "Add comment to card", "POST", "/api/cards/{cardId}/comments", {{"cardId", "string", "Card ID"}, {"text", "string", "Comment text"}}, "comment"});
    definitions_.push_back({"update_comment", "Update comment", "PATCH", "/api/comments/{id}", {{"id", "string", "Comment ID"}, {"text", "string", "New text"}}, "comment"});
    definitions_.push_back({"delete_comment", "Delete a comment", "DELETE", "/api/comments/{id}", {{"id", "string", "Comment ID"}}, "comment"});

    // =========================================================================
    // 8. Custom Fields
    // =========================================================================
    definitions_.push_back({"create_board_custom_field_group", "Create board CF group", "POST", "/api/boards/{boardId}/custom-field-groups", {{"boardId", "string", "Board ID"}, {"name", "string", "Name"}}, "custom-field-group"});
    definitions_.push_back({"create_card_custom_field_group", "Create card CF group", "POST", "/api/cards/{cardId}/custom-field-groups", {{"cardId", "string", "Card ID"}, {"name", "string", "Name"}}, "custom-field-group"});
    definitions_.push_back({"update_custom_field_group", "Update CF group", "PATCH", "/api/custom-field-groups/{id}", {{"id", "string", "Group ID"}, {"name", "string", "New name", false}}, "custom-field-group"});
    definitions_.push_back({"delete_custom_field_group", "Delete CF group", "DELETE", "/api/custom-field-groups/{id}", {{"id", "string", "Group ID"}}, "custom-field-group"});

    definitions_.push_back({"create_base_custom_field", "Create base CF", "POST", "/api/base-custom-field-groups/{baseCustomFieldGroupId}/custom-fields", {{"baseCustomFieldGroupId", "string", "Base CF Group ID"}, {"name", "string", "Field Name"}, {"type", "string", "Field Type", true, {"text", "number", "dropdown", "checkbox", "date"}}}, "custom-field"});
    definitions_.push_back({"create_custom_field", "Create CF in group", "POST", "/api/custom-field-groups/{customFieldGroupId}/custom-fields", {{"customFieldGroupId", "string", "Group ID"}, {"name", "string", "Field Name"}, {"type", "string", "Field Type", true, {"text", "number", "dropdown", "checkbox", "date"}}, {"position", "number", "Position", false}}, "custom-field"});
    definitions_.push_back({"update_custom_field", "Update CF", "PATCH", "/api/custom-fields/{id}", {{"id", "string", "Field ID"}, {"name", "string", "New name", false}, {"position", "number", "New position", false}}, "custom-field"});
    definitions_.push_back({"delete_custom_field", "Delete CF", "DELETE", "/api/custom-fields/{id}", {{"id", "string", "Field ID"}}, "custom-field"});

    // =========================================================================
    // 9. System (Users, Webhooks, etc)
    // =========================================================================
    definitions_.push_back({"mark_all_notifications_read", "Mark all notifications read", "POST", "/api/notifications/read-all", {}, "notifications"});
    definitions_.push_back({"update_notification", "Update a notification", "PATCH", "/api/notifications/{id}", {{"id", "string", "Notification ID"}, {"isRead", "boolean", "Mark as read"}}, "notification"});
    
    definitions_.push_back({"create_webhook", "Create a webhook", "POST", "/api/webhooks", {{"name", "string", "Name"}, {"url", "string", "URL"}}, "webhook"});
    definitions_.push_back({"update_webhook", "Update a webhook", "PATCH", "/api/webhooks/{id}", {{"id", "string", "Webhook ID"}, {"name", "string", "New name", false}, {"url", "string", "New URL", false}}, "webhook"});
    definitions_.push_back({"delete_webhook", "Delete a webhook", "DELETE", "/api/webhooks/{id}", {{"id", "string", "Webhook ID"}}, "webhook"});
    
    definitions_.push_back({"create_user", "Create a new user", "POST", "/api/users", {{"email", "string", "Email"}, {"password", "string", "Password", false}, {"name", "string", "Name", false}}, "user"});
    definitions_.push_back({"update_user", "Update a user", "PATCH", "/api/users/{id}", {{"id", "string", "User ID"}, {"name", "string", "New name", false}, {"email", "string", "New email", false}}, "user"});
    definitions_.push_back({"delete_user", "Delete a user", "DELETE", "/api/users/{id}", {{"id", "string", "User ID"}}, "user"});
    definitions_.push_back({"update_user_password", "Update user password", "PATCH", "/api/users/{id}/password", {{"id", "string", "User ID"}, {"password", "string", "New Password"}}, "user"});
}

wfrest::Json ToolRegistry::build_schema(const ToolDef& def)
{
    wfrest::Json schema = wfrest::Json::Object();
    schema.push_back("type", "object");
    wfrest::Json props = wfrest::Json::Object();
    wfrest::Json required = wfrest::Json::Array();
    for (const auto& p : def.params)
    {
        wfrest::Json p_meta = wfrest::Json::Object();
        p_meta.push_back("type", p.type);
        p_meta.push_back("description", p.description);
        if (!p.enum_values.empty())
        {
            wfrest::Json enums = wfrest::Json::Array();
            for (const auto& v : p.enum_values)
                enums.push_back(v);
            p_meta.push_back("enum", enums);
        }
        props.push_back(p.name, p_meta);
        if (p.required)
            required.push_back(p.name);
    }
    schema.push_back("properties", props);
    if (!required.empty())
        schema.push_back("required", required);
    return schema;
}

std::vector<Tool> ToolRegistry::list_tools()
{
    std::vector<Tool> tools;
    for (const auto& def : definitions_)
    {
        tools.push_back({def.name, def.description, build_schema(def)});
    }
    Tool set_cfv;
    set_cfv.name = "set_card_custom_field";
    set_cfv.description = "Set a custom field value on a card";
    set_cfv.inputSchema = wfrest::Json::parse(R"({
        "type": "object",
        "properties": {
            "cardId": {"type": "string", "description": "Card ID"},
            "customFieldGroupId": {"type": "string", "description": "Group ID"},
            "customFieldId": {"type": "string", "description": "Field ID"},
            "value": {"type": "string", "description": "New value"}
        },
        "required": ["cardId", "customFieldGroupId", "customFieldId", "value"]
    })");
    tools.push_back(set_cfv);
    
    Tool del_cfv;
    del_cfv.name = "delete_card_custom_field_value";
    del_cfv.description = "Delete a custom field value from a card";
    del_cfv.inputSchema = wfrest::Json::parse(R"({
        "type": "object",
        "properties": {
            "cardId": {"type": "string", "description": "Card ID"},
            "customFieldGroupId": {"type": "string", "description": "Group ID"},
            "customFieldId": {"type": "string", "description": "Field ID"}
        },
        "required": ["cardId", "customFieldGroupId", "customFieldId"]
    })");
    tools.push_back(del_cfv);
    Tool sub;
    sub.name = "subscribe_planka_events";
    sub.description = "Subscribe to Planka events by proxying them to an HTTP endpoint (HTTP mode only)";
    sub.inputSchema = wfrest::Json::parse(R"({
        "type": "object",
        "properties": {
            "forward_url": { "type": "string" },
            "forward_headers": { "type": "object" },
            "payload_template": { "type": "string", "description": "Optional payload template" }
        },
        "required": ["forward_url"]
    })");
    tools.push_back(sub);

    return tools;
}

static std::string replace_all(std::string str, const std::string& from, const std::string& to)
{
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

coke::Task<wfrest::Json> ToolRegistry::execute_generic(const ToolDef& def, const wfrest::Json& args)
{
    std::string path = def.path_template;
    wfrest::Json body = wfrest::Json::Object();
    for (const auto& p : def.params)
    {
        if (args.has(p.name))
        {
            std::string placeholder = "{" + p.name + "}";
            if (path.find(placeholder) != std::string::npos)
            {
                // In wfrest, .dump() on string prints quotes, but .get<std::string>() returns string directly.
                std::string val_str;
                if (args[p.name].is_string()) val_str = args[p.name].get<std::string>();
                else val_str = args[p.name].dump();
                
                path = replace_all(path, placeholder, val_str);
            }
            else if (def.body_type == "json")
            {
                body.push_back(p.name, args[p.name]);
            }
        }
    }

    if (def.method == "POST")
    {
        if (def.name == "create_project" && !body.has("type"))
            body.push_back("type", "shared");
        if (def.name == "create_board" && !body.has("position"))
            body.push_back("position", 65536);
        if (def.name == "create_list" && !body.has("position"))
            body.push_back("position", 65536);
        if (def.name == "create_list" && !body.has("type"))
            body.push_back("type", "active");
        if (def.name == "create_card" && !body.has("type"))
            body.push_back("type", "story");
        if (def.name == "create_card" && !body.has("position"))
            body.push_back("position", 65536);
        if (def.name == "duplicate_card" && !body.has("position"))
            body.push_back("position", 65536);
        if (def.name == "create_label" && !body.has("position"))
            body.push_back("position", 65536);
        if (def.name == "create_label" && !body.has("color"))
            body.push_back("color", "berry-red");
        if (def.name == "create_task_list" && !body.has("position"))
            body.push_back("position", 65536);
        if (def.name == "create_task" && !body.has("position"))
            body.push_back("position", 65536);
    }

    wfrest::Json res;
    if (def.method == "POST")
        res = co_await client_.post(path, body);
    else if (def.method == "PATCH")
        res = co_await client_.patch(path, body);
    else if (def.method == "DELETE")
        res = co_await client_.del(path);
    else
        res = co_await client_.get(path);

    wfrest::Json content = wfrest::Json::Array();
    wfrest::Json item = wfrest::Json::Object();
    item.push_back("type", "text");
    wfrest::Json target = res.has("item") ? res["item"] : res;
    
    if (target.is_valid() && (target.has("id") || res.is_array() || res.is_null() || res.has("success")))
    {
        std::string msg = "Successfully executed " + def.name + ".";
        if (target.has("id"))
        {
            std::string id_str;
            if (target["id"].is_string())
            {
                id_str = target["id"].get<std::string>();
            }
            else
            {
                // .dump() cleanly handles number types as literal string output.
                id_str = target["id"].dump();
            }
            if (!id_str.empty())
            {
                msg += " New " + def.item_type + " ID: `" + id_str + "`";
            }
        }
        item.push_back("text", msg);
    }
    else
    {
        item.push_back("text", "Error calling " + def.name + ": " + res.dump());
    }
    content.push_back(item);
    co_return content;
}

coke::Task<wfrest::Json> ToolRegistry::call_tool(const std::string& name, const wfrest::Json& arguments)
{
    for (const auto& def : definitions_)
    {
        if (def.name == name)
            co_return co_await execute_generic(def, arguments);
    }
    
    if (name == "set_card_custom_field")
    {
        std::string path = "/api/cards/" + arguments["cardId"].get<std::string>() +
            "/custom-field-values/customFieldGroupId:" + arguments["customFieldGroupId"].get<std::string>() +
            ":customFieldId:" + arguments["customFieldId"].get<std::string>();
        wfrest::Json body = wfrest::Json::Object();
        body.push_back("value", arguments["value"]);
        wfrest::Json res = co_await client_.patch(path, body);
        wfrest::Json content = wfrest::Json::Array();
        wfrest::Json item = wfrest::Json::Object();
        item.push_back("type", "text");
        item.push_back("text", (res.has("id") || res.has("item")) ? "Successfully updated custom field." : "Error: " + res.dump());
        content.push_back(item);
        co_return content;
    }
    else if (name == "delete_card_custom_field_value")
    {
        std::string path = "/api/cards/" + arguments["cardId"].get<std::string>() +
            "/custom-field-values/customFieldGroupId:" + arguments["customFieldGroupId"].get<std::string>() +
            ":customFieldId:" + arguments["customFieldId"].get<std::string>();
        wfrest::Json res = co_await client_.del(path);
        wfrest::Json content = wfrest::Json::Array();
        wfrest::Json item = wfrest::Json::Object();
        item.push_back("type", "text");
        item.push_back("text", res.is_null() ? "Successfully deleted custom field value." : "Error: " + res.dump());
        content.push_back(item);
        co_return content;
    }
    else if (name == "subscribe_planka_events")
    {
        webhook::ForwardRule rule;
        rule.url = arguments["forward_url"].get<std::string>();
        if (arguments.has("forward_headers")) {
            auto headers = arguments["forward_headers"];
            if (headers.is_object()) {
                for (auto it = headers.begin(); it != headers.end(); ++it) {
                    rule.headers[(*it).key()] = (*it).is_string() ? (*it).get<std::string>() : (*it).dump();
                }
            }
        }
        if (arguments.has("payload_template") && arguments["payload_template"].is_string()) {
            rule.payload_template = arguments["payload_template"].get<std::string>();
        }
        webhook::Router::instance().set_forward_rule(rule);

        wfrest::Json content = wfrest::Json::Array();
        wfrest::Json item = wfrest::Json::Object();
        item.push_back("type", "text");
        item.push_back("text", "Successfully subscribed to Planka events. Forwarding to " + rule.url);
        content.push_back(item);
        co_return content;
    }

    co_return wfrest::Json::Array();
}

} // namespace mcp
