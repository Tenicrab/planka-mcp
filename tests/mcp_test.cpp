#include "doctest.h"
#include "mcp/handler.h"
#include "planka/client.h"
#include "resources/registry.h"
#include "tools/registry.h"
#include <wfrest/Json.h>
#include <string>
#include <regex>
#include <ctime>
#include "../src/webhook/router.h"

std::string extract_id(const std::string& text) {
    std::regex re(R"(`([^`]+)`)");
    std::smatch match;
    if (std::regex_search(text, match, re)) return match[1];
    return "";
}

TEST_CASE("McpHandler Verification Suite") {
    PlankaClient::Config config;
    config.url = PlankaClient::get_env("PLANKA_URL");
    config.api_key = PlankaClient::get_env("PLANKA_API_KEY");

    mcp::ResourceRegistry registry;
    mcp::ToolRegistry tool_registry;
    mcp::McpHandler handler(registry, tool_registry);
    std::string ts = std::to_string(std::time(nullptr));

    SUBCASE("1. Tools Discovery & Enum Verification") {
        std::string req = R"({"jsonrpc": "2.0", "id": 1, "method": "tools/list"})";
        auto resp_str = handler.handle_message(req, config);
        REQUIRE(resp_str.has_value());
        wfrest::Json resp = wfrest::Json::parse(*resp_str);
        
        CHECK(resp["result"]["tools"].size() >= 30);
        
        bool enum_found = false;
        for (const auto& tool : resp["result"]["tools"]) {
            if (tool["name"].get<std::string>() == "create_project") {
                enum_found = tool["inputSchema"]["properties"]["type"].has("enum");
            }
        }
        CHECK(enum_found);
    }

    SUBCASE("2. End-to-End Resource Life-cycle") {
        // A. Project
        std::string p_name = "TEST_P_" + ts;
        std::string p_req = R"({"jsonrpc": "2.0", "id": "p1", "method": "tools/call", "params": {"name": "create_project", "arguments": {"name": ")" + p_name + R"(", "type": "shared"}}})";
        auto p_res = handler.handle_message(p_req, config);
        std::string project_id = extract_id(wfrest::Json::parse(*p_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!project_id.empty());

        // B. Board
        std::string b_req = R"({"jsonrpc": "2.0", "id": "b1", "method": "tools/call", "params": {"name": "create_board", "arguments": {"projectId": ")" + project_id + R"(", "name": "TestBoard"}}})";
        auto b_res = handler.handle_message(b_req, config);
        std::string board_id = extract_id(wfrest::Json::parse(*b_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!board_id.empty());

        // C. List
        std::string l_req = R"({"jsonrpc": "2.0", "id": "l1", "method": "tools/call", "params": {"name": "create_list", "arguments": {"boardId": ")" + board_id + R"(", "name": "TestList"}}})";
        auto l_res = handler.handle_message(l_req, config);
        std::string list_id = extract_id(wfrest::Json::parse(*l_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!list_id.empty());

        // D. Card
        std::string c_req = R"({"jsonrpc": "2.0", "id": "c1", "method": "tools/call", "params": {"name": "create_card", "arguments": {"listId": ")" + list_id + R"(", "name": "TestCard"}}})";
        auto c_res = handler.handle_message(c_req, config);
        std::string card_id = extract_id(wfrest::Json::parse(*c_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!card_id.empty());

        // E. Label & Binding
        std::string lb_req = R"({"jsonrpc": "2.0", "id": "lb1", "method": "tools/call", "params": {"name": "create_label", "arguments": {"boardId": ")" + board_id + R"(", "name": "Urgent", "color": "orange-peel"}}})";
        auto lb_res = handler.handle_message(lb_req, config);
        std::string label_id = extract_id(wfrest::Json::parse(*lb_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!label_id.empty());

        std::string al_req = R"({"jsonrpc": "2.0", "id": "al1", "method": "tools/call", "params": {"name": "add_card_label", "arguments": {"cardId": ")" + card_id + R"(", "labelId": ")" + label_id + R"("}}})";
        CHECK(handler.handle_message(al_req, config).has_value());

        // F. Card Operations: Duplicate & Move
        std::string dp_req = R"({"jsonrpc": "2.0", "id": "dp1", "method": "tools/call", "params": {"name": "duplicate_card", "arguments": {"id": ")" + card_id + R"("}}})";
        auto dp_res = handler.handle_message(dp_req, config);
        std::string dp_text = wfrest::Json::parse(*dp_res)["result"]["content"][0]["text"].get<std::string>();
        std::string dup_id = extract_id(dp_text);
        CHECK(!dup_id.empty());

        std::string mv_req = R"({"jsonrpc": "2.0", "id": "mv1", "method": "tools/call", "params": {"name": "move_card", "arguments": {"id": ")" + dup_id + R"(", "listId": ")" + list_id + R"(", "position": 100}}})";
        CHECK(handler.handle_message(mv_req, config).has_value());

        // G. Tasks & Comments
        std::string tl_req = R"({"jsonrpc": "2.0", "id": "tl1", "method": "tools/call", "params": {"name": "create_task_list", "arguments": {"cardId": ")" + card_id + R"(", "name": "Subtasks"}}})";
        auto tl_res = handler.handle_message(tl_req, config);
        std::string task_list_id = extract_id(wfrest::Json::parse(*tl_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!task_list_id.empty());

        std::string tk_req = R"({"jsonrpc": "2.0", "id": "tk1", "method": "tools/call", "params": {"name": "create_task", "arguments": {"taskListId": ")" + task_list_id + R"(", "name": "Task 1"}}})";
        auto tk_res = handler.handle_message(tk_req, config);
        std::string task_id = extract_id(wfrest::Json::parse(*tk_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!task_id.empty());

        std::string cm_req = R"({"jsonrpc": "2.0", "id": "cm1", "method": "tools/call", "params": {"name": "create_comment", "arguments": {"cardId": ")" + card_id + R"(", "text": "Self-destructing project."}}})";
        auto cm_res = handler.handle_message(cm_req, config);
        std::string comment_id = extract_id(wfrest::Json::parse(*cm_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!comment_id.empty());

        // H. Cleanup Phase
        std::string d_cm_req = R"({"jsonrpc": "2.0", "id": "d_cm", "method": "tools/call", "params": {"name": "delete_comment", "arguments": {"id": ")" + comment_id + R"("}}})";
        CHECK(handler.handle_message(d_cm_req, config).has_value());

        std::string d_tk_req = R"({"jsonrpc": "2.0", "id": "d_tk", "method": "tools/call", "params": {"name": "delete_task", "arguments": {"id": ")" + task_id + R"("}}})";
        CHECK(handler.handle_message(d_tk_req, config).has_value());

        std::string d_tl_req = R"({"jsonrpc": "2.0", "id": "d_tl", "method": "tools/call", "params": {"name": "delete_task_list", "arguments": {"id": ")" + task_list_id + R"("}}})";
        CHECK(handler.handle_message(d_tl_req, config).has_value());

        std::string d_al_req = R"({"jsonrpc": "2.0", "id": "d_al", "method": "tools/call", "params": {"name": "remove_card_label", "arguments": {"cardId": ")" + card_id + R"(", "labelId": ")" + label_id + R"("}}})";
        CHECK(handler.handle_message(d_al_req, config).has_value());

        std::string d_c1_req = R"({"jsonrpc": "2.0", "id": "d_c1", "method": "tools/call", "params": {"name": "delete_card", "arguments": {"id": ")" + card_id + R"("}}})";
        CHECK(handler.handle_message(d_c1_req, config).has_value());
        
        std::string d_c2_req = R"({"jsonrpc": "2.0", "id": "d_c2", "method": "tools/call", "params": {"name": "delete_card", "arguments": {"id": ")" + dup_id + R"("}}})";
        CHECK(handler.handle_message(d_c2_req, config).has_value());

        std::string d_lb_req = R"({"jsonrpc": "2.0", "id": "d_lb", "method": "tools/call", "params": {"name": "delete_label", "arguments": {"id": ")" + label_id + R"("}}})";
        CHECK(handler.handle_message(d_lb_req, config).has_value());

        std::string d_l_req = R"({"jsonrpc": "2.0", "id": "d_l", "method": "tools/call", "params": {"name": "delete_list", "arguments": {"id": ")" + list_id + R"("}}})";
        CHECK(handler.handle_message(d_l_req, config).has_value());

        std::string d_b_req = R"({"jsonrpc": "2.0", "id": "d_b", "method": "tools/call", "params": {"name": "delete_board", "arguments": {"id": ")" + board_id + R"("}}})";
        CHECK(handler.handle_message(d_b_req, config).has_value());

        std::string d_p_req = R"({"jsonrpc": "2.0", "id": "d_p", "method": "tools/call", "params": {"name": "delete_project", "arguments": {"id": ")" + project_id + R"("}}})";
        auto final_res = handler.handle_message(d_p_req, config);
        CHECK(wfrest::Json::parse(*final_res)["result"]["content"][0]["text"].get<std::string>().find("Successfully") != std::string::npos);
    }

    SUBCASE("3. System Tools Verification") {
        std::string sn_req = R"({"jsonrpc": "2.0", "id": "sn1", "method": "tools/call", "params": {"name": "mark_all_notifications_read", "arguments": {}}})";
        CHECK(handler.handle_message(sn_req, config).has_value());

        std::string sw_req = R"({"jsonrpc": "2.0", "id": "sw1", "method": "tools/call", "params": {"name": "create_webhook", "arguments": {"name": "TestHook", "url": "https://example.com/callback"}}})";
        auto sw_res = handler.handle_message(sw_req, config);
        std::string hook_id = extract_id(wfrest::Json::parse(*sw_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!hook_id.empty());

        std::string dw_req = R"({"jsonrpc": "2.0", "id": "dw1", "method": "tools/call", "params": {"name": "delete_webhook", "arguments": {"id": ")" + hook_id + R"("}}})";
        CHECK(handler.handle_message(dw_req, config).has_value());
    }

    SUBCASE("4. Webhook Template Engine & Subscription Tool") {
        std::string mock_event_str = R"({
            "action": "createCard",
            "data": {
                "card": { "id": "12345", "name": "Super Important Task", "position": 65536 },
                "project": { "name": "Alpha Project" }
            }
        })";
        wfrest::Json mock_event = wfrest::Json::parse(mock_event_str);
        std::string tmpl = R"({"msg": "New card {{data.card.name}} created in {{data.project.name}}", "id": {{data.card.id}}})";
        std::string rendered = mcp::webhook::render_template(tmpl, mock_event);
        CHECK(rendered.find("Super Important Task") != std::string::npos);

        std::string sub_req = R"({
            "jsonrpc": "2.0", "id": "sub1", "method": "tools/call", "params": {
                "name": "subscribe_planka_events", 
                "arguments": {
                    "forward_url": "http://127.0.0.1:9090/agent-inbox",
                    "payload_template": "Event: {{action}}"
                }
            }
        })";
        auto sub_res = handler.handle_message(sub_req, config);
        CHECK(sub_res.has_value());
    }
}
