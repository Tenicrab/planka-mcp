#include "doctest.h"
#include "mcp/handler.h"
#include "planka/client.h"
#include "resources/registry.h"
#include "tools/registry.h"
#include <wfrest/Json.h>
#include <string>
#include <regex>
#include <iostream>
#include <ctime>
#include "../src/webhook/router.h"

std::string extract_id(const std::string& text) {
    std::regex re(R"(`(\d+)`)");
    std::smatch match;
    if (std::regex_search(text, match, re)) return match[1];
    return "";
}

TEST_CASE("McpHandler Verification Suite") {
    PlankaClient client = PlankaClient::from_env();
    mcp::ResourceRegistry registry(client);
    mcp::ToolRegistry tool_registry(client);
    mcp::McpHandler handler(client, registry, tool_registry);
    std::string ts = std::to_string(std::time(nullptr));

    SUBCASE("1. Tools Discovery & Enum Verification") {
        std::string req = R"({"jsonrpc": "2.0", "id": 1, "method": "tools/list"})";
        auto resp_str = handler.handle_message(req);
        REQUIRE(resp_str.has_value());
        wfrest::Json resp = wfrest::Json::parse(*resp_str);
        
        // Count tools: 32 + 1 (custom_field) = 33
        CHECK(resp["result"]["tools"].size() >= 30);
        
        // Verify Enum for AI
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
        auto p_res = handler.handle_message(p_req);
        std::string project_id = extract_id(wfrest::Json::parse(*p_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!project_id.empty());

        // B. Board
        std::string b_req = R"({"jsonrpc": "2.0", "id": "b1", "method": "tools/call", "params": {"name": "create_board", "arguments": {"projectId": ")" + project_id + R"(", "name": "TestBoard"}}})";
        auto b_res = handler.handle_message(b_req);
        std::string board_id = extract_id(wfrest::Json::parse(*b_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!board_id.empty());

        // C. List
        std::string l_req = R"({"jsonrpc": "2.0", "id": "l1", "method": "tools/call", "params": {"name": "create_list", "arguments": {"boardId": ")" + board_id + R"(", "name": "TestList"}}})";
        auto l_res = handler.handle_message(l_req);
        std::string list_id = extract_id(wfrest::Json::parse(*l_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!list_id.empty());

        // D. Card
        std::string c_req = R"({"jsonrpc": "2.0", "id": "c1", "method": "tools/call", "params": {"name": "create_card", "arguments": {"listId": ")" + list_id + R"(", "name": "TestCard"}}})";
        auto c_res = handler.handle_message(c_req);
        std::string card_id = extract_id(wfrest::Json::parse(*c_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!card_id.empty());

        // E. Label & Binding
        std::string lb_req = R"({"jsonrpc": "2.0", "id": "lb1", "method": "tools/call", "params": {"name": "create_label", "arguments": {"boardId": ")" + board_id + R"(", "name": "Urgent", "color": "orange-peel"}}})";
        auto lb_res = handler.handle_message(lb_req);
        std::string label_id = extract_id(wfrest::Json::parse(*lb_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!label_id.empty());

        std::string al_req = R"({"jsonrpc": "2.0", "id": "al1", "method": "tools/call", "params": {"name": "add_card_label", "arguments": {"cardId": ")" + card_id + R"(", "labelId": ")" + label_id + R"("}}})";
        CHECK(handler.handle_message(al_req).has_value());

        // F. Card Operations: Duplicate & Move
        std::string dp_req = R"({"jsonrpc": "2.0", "id": "dp1", "method": "tools/call", "params": {"name": "duplicate_card", "arguments": {"id": ")" + card_id + R"("}}})";
        auto dp_res = handler.handle_message(dp_req);
        std::string dp_text = wfrest::Json::parse(*dp_res)["result"]["content"][0]["text"].get<std::string>();
        std::string dup_id = extract_id(dp_text);
        if (dup_id.empty()) std::cout << "Duplicate Card Error: " << dp_text << "\n";
        CHECK(!dup_id.empty());

        std::string mv_req = R"({"jsonrpc": "2.0", "id": "mv1", "method": "tools/call", "params": {"name": "move_card", "arguments": {"id": ")" + dup_id + R"(", "listId": ")" + list_id + R"(", "position": 100}}})";
        CHECK(handler.handle_message(mv_req).has_value());

        // G. Tasks & Comments
        std::string tl_req = R"({"jsonrpc": "2.0", "id": "tl1", "method": "tools/call", "params": {"name": "create_task_list", "arguments": {"cardId": ")" + card_id + R"(", "name": "Subtasks"}}})";
        auto tl_res = handler.handle_message(tl_req);
        std::string task_list_id = extract_id(wfrest::Json::parse(*tl_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!task_list_id.empty());

        std::string tk_req = R"({"jsonrpc": "2.0", "id": "tk1", "method": "tools/call", "params": {"name": "create_task", "arguments": {"taskListId": ")" + task_list_id + R"(", "name": "Task 1"}}})";
        auto tk_res = handler.handle_message(tk_req);
        std::string task_id = extract_id(wfrest::Json::parse(*tk_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!task_id.empty());

        std::string cm_req = R"({"jsonrpc": "2.0", "id": "cm1", "method": "tools/call", "params": {"name": "create_comment", "arguments": {"cardId": ")" + card_id + R"(", "text": "Self-destructing project."}}})";
        auto cm_res = handler.handle_message(cm_req);
        std::string comment_id = extract_id(wfrest::Json::parse(*cm_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!comment_id.empty());

        // H. Cleanup Phase (Pop the stack)
        // 1. Delete Comment
        std::string d_cm_req = R"({"jsonrpc": "2.0", "id": "d_cm", "method": "tools/call", "params": {"name": "delete_comment", "arguments": {"id": ")" + comment_id + R"("}}})";
        CHECK(handler.handle_message(d_cm_req).has_value());

        // 2. Delete Task
        std::string d_tk_req = R"({"jsonrpc": "2.0", "id": "d_tk", "method": "tools/call", "params": {"name": "delete_task", "arguments": {"id": ")" + task_id + R"("}}})";
        CHECK(handler.handle_message(d_tk_req).has_value());

        // 3. Delete Task List
        std::string d_tl_req = R"({"jsonrpc": "2.0", "id": "d_tl", "method": "tools/call", "params": {"name": "delete_task_list", "arguments": {"id": ")" + task_list_id + R"("}}})";
        CHECK(handler.handle_message(d_tl_req).has_value());

        // 4. Delete Card Binding
        std::string d_al_req = R"({"jsonrpc": "2.0", "id": "d_al", "method": "tools/call", "params": {"name": "remove_card_label", "arguments": {"cardId": ")" + card_id + R"(", "labelId": ")" + label_id + R"("}}})";
        CHECK(handler.handle_message(d_al_req).has_value());

        // 5. Delete Cards (original and dup)
        std::string d_c1_req = R"({"jsonrpc": "2.0", "id": "d_c1", "method": "tools/call", "params": {"name": "delete_card", "arguments": {"id": ")" + card_id + R"("}}})";
        CHECK(handler.handle_message(d_c1_req).has_value());
        
        std::string d_c2_req = R"({"jsonrpc": "2.0", "id": "d_c2", "method": "tools/call", "params": {"name": "delete_card", "arguments": {"id": ")" + dup_id + R"("}}})";
        CHECK(handler.handle_message(d_c2_req).has_value());

        // 6. Delete Label
        std::string d_lb_req = R"({"jsonrpc": "2.0", "id": "d_lb", "method": "tools/call", "params": {"name": "delete_label", "arguments": {"id": ")" + label_id + R"("}}})";
        CHECK(handler.handle_message(d_lb_req).has_value());

        // 7. Delete List
        std::string d_l_req = R"({"jsonrpc": "2.0", "id": "d_l", "method": "tools/call", "params": {"name": "delete_list", "arguments": {"id": ")" + list_id + R"("}}})";
        CHECK(handler.handle_message(d_l_req).has_value());

        // 8. Delete Board
        std::string d_b_req = R"({"jsonrpc": "2.0", "id": "d_b", "method": "tools/call", "params": {"name": "delete_board", "arguments": {"id": ")" + board_id + R"("}}})";
        CHECK(handler.handle_message(d_b_req).has_value());

        // 9. Delete Project
        std::string d_p_req = R"({"jsonrpc": "2.0", "id": "d_p", "method": "tools/call", "params": {"name": "delete_project", "arguments": {"id": ")" + project_id + R"("}}})";
        auto final_res = handler.handle_message(d_p_req);
        std::string final_text = wfrest::Json::parse(*final_res)["result"]["content"][0]["text"].get<std::string>();
        if (final_text.find("Successfully") == std::string::npos) std::cout << "Delete Project Error: " << final_text << "\n";
        CHECK(final_text.find("Successfully") != std::string::npos);

        // 10. Verify Cascade Blocking (Creating an isolated Project+Board, trigger error, then clean up)
        std::string cascade_p_req = R"({"jsonrpc": "2.0", "id": "cp1", "method": "tools/call", "params": {"name": "create_project", "arguments": {"name": "CASCADE_TEST", "type": "private"}}})";
        auto cp_res = handler.handle_message(cascade_p_req);
        std::string cp_id = extract_id(wfrest::Json::parse(*cp_res)["result"]["content"][0]["text"].get<std::string>());
        
        std::string cascade_b_req = R"({"jsonrpc": "2.0", "id": "cb1", "method": "tools/call", "params": {"name": "create_board", "arguments": {"projectId": ")" + cp_id + R"(", "name": "CascadeBoard"}}})";
        auto cb_res = handler.handle_message(cascade_b_req);
        std::string cb_id = extract_id(wfrest::Json::parse(*cb_res)["result"]["content"][0]["text"].get<std::string>());
        
        // Block test: Try deleting project without deleting board
        std::string block_p_req = R"({"jsonrpc": "2.0", "id": "dp2", "method": "tools/call", "params": {"name": "delete_project", "arguments": {"id": ")" + cp_id + R"("}}})";
        auto block_res = handler.handle_message(block_p_req);
        std::string block_text = wfrest::Json::parse(*block_res)["result"]["content"][0]["text"].get<std::string>();
        CHECK(block_text.find("Must not have boards") != std::string::npos);
        
        // Clean up the isolated cascade test
        std::string safe_b_req = R"({"jsonrpc": "2.0", "id": "cb2", "method": "tools/call", "params": {"name": "delete_board", "arguments": {"id": ")" + cb_id + R"("}}})";
        CHECK(handler.handle_message(safe_b_req).has_value());
        
        std::string safe_p_req = R"({"jsonrpc": "2.0", "id": "cp2", "method": "tools/call", "params": {"name": "delete_project", "arguments": {"id": ")" + cp_id + R"("}}})";
        auto safe_p_res = handler.handle_message(safe_p_req);
        CHECK(wfrest::Json::parse(*safe_p_res)["result"]["content"][0]["text"].get<std::string>().find("Successfully") != std::string::npos);
    }

    SUBCASE("3. System Tools Verification") {
        std::string sn_req = R"({"jsonrpc": "2.0", "id": "sn1", "method": "tools/call", "params": {"name": "mark_all_notifications_read", "arguments": {}}})";
        CHECK(handler.handle_message(sn_req).has_value());

        // We now have Admin privileges, so webhooks should work natively
        std::string sw_req = R"({"jsonrpc": "2.0", "id": "sw1", "method": "tools/call", "params": {"name": "create_webhook", "arguments": {"name": "TestHook", "url": "https://example.com/callback"}}})";
        auto sw_res = handler.handle_message(sw_req);
        std::string sw_text = wfrest::Json::parse(*sw_res)["result"]["content"][0]["text"].get<std::string>();
        std::string hook_id = extract_id(sw_text);
        if (hook_id.empty()) std::cout << "Create Webhook Error: " << sw_text << "\n";
        REQUIRE(!hook_id.empty());

        std::string dw_req = R"({"jsonrpc": "2.0", "id": "dw1", "method": "tools/call", "params": {"name": "delete_webhook", "arguments": {"id": ")" + hook_id + R"("}}})";
        CHECK(handler.handle_message(dw_req).has_value());
    }

    SUBCASE("4. Webhook Template Engine & Subscription Tool") {
        // Test Template Rendering directly
        std::string mock_event_str = R"({
            "action": "createCard",
            "data": {
                "card": {
                    "id": "12345",
                    "name": "Super Important Task",
                    "position": 65536
                },
                "project": {
                    "name": "Alpha Project"
                }
            }
        })";
        wfrest::Json mock_event = wfrest::Json::parse(mock_event_str);
        
        std::string tmpl = R"({"msg": "New card {{data.card.name}} created in {{data.project.name}}", "id": {{data.card.id}}, "pos": {{data.card.position}}})";
        std::string rendered = mcp::webhook::render_template(tmpl, mock_event);
        
        CHECK(rendered.find("Super Important Task") != std::string::npos);
        CHECK(rendered.find("Alpha Project") != std::string::npos);
        CHECK(rendered.find("12345") != std::string::npos);
        CHECK(rendered.find("65536") != std::string::npos);

        // Test Tool Call (subscribe_planka_events)
        std::string sub_req = R"({
            "jsonrpc": "2.0", 
            "id": "sub1", 
            "method": "tools/call", 
            "params": {
                "name": "subscribe_planka_events", 
                "arguments": {
                    "forward_url": "http://127.0.0.1:9090/agent-inbox",
                    "forward_headers": {"Authorization": "Bearer secret"},
                    "payload_template": "Event: {{action}} Card: {{data.card.name}}"
                }
            }
        })";
        auto sub_res = handler.handle_message(sub_req);
        REQUIRE(sub_res.has_value());
        std::string sub_text = wfrest::Json::parse(*sub_res)["result"]["content"][0]["text"].get<std::string>();
        CHECK(sub_text.find("Successfully subscribed") != std::string::npos);
        CHECK(sub_text.find("http://127.0.0.1:9090/agent-inbox") != std::string::npos);
    }
}
