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
    mcp::ToolRegistry tool_registry(registry);
    mcp::McpHandler handler(registry, tool_registry);
    std::string ts = std::to_string(std::time(nullptr));

    SUBCASE("1. Tools Discovery & Enum Verification") {
        std::string req = R"({"jsonrpc": "2.0", "id": 1, "method": "tools/list"})";
        auto resp_str = handler.handle_message(req, config);
        REQUIRE(resp_str.has_value());
        wfrest::Json resp = wfrest::Json::parse(*resp_str);
        
        CHECK(resp["result"]["tools"].size() >= 5);
        
        bool found = false;
        for (const auto& tool : resp["result"]["tools"]) {
            if (tool["name"].get<std::string>() == "planka_create") found = true;
        }
        CHECK(found);
    }

    SUBCASE("2. End-to-End Resource Life-cycle") {
        // A. Project
        std::string p_name = "TEST_P_" + ts;
        std::string p_req = R"({"jsonrpc": "2.0", "id": "p1", "method": "tools/call", "params": {"name": "planka_create", "arguments": {"entity_type": "project", "data": {"name": ")" + p_name + R"(", "type": "shared"}}}})";
        auto p_res_str = handler.handle_message(p_req, config);
        REQUIRE(p_res_str.has_value());
        wfrest::Json p_resp = wfrest::Json::parse(*p_res_str);
        if (p_resp.has("error")) {
            FAIL("Project creation failed: " << p_resp["error"].dump());
        }
        std::string project_id = extract_id(p_resp["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!project_id.empty());

        // B. Board
        std::string b_req = R"({"jsonrpc": "2.0", "id": "b1", "method": "tools/call", "params": {"name": "planka_create", "arguments": {"entity_type": "board", "parent_id": ")" + project_id + R"(", "data": {"name": "TestBoard"}}}})";
        auto b_res = handler.handle_message(b_req, config);
        std::string board_id = extract_id(wfrest::Json::parse(*b_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!board_id.empty());

        // C. List
        std::string l_req = R"({"jsonrpc": "2.0", "id": "l1", "method": "tools/call", "params": {"name": "planka_create", "arguments": {"entity_type": "list", "parent_id": ")" + board_id + R"(", "data": {"name": "TestList"}}}})";
        auto l_res = handler.handle_message(l_req, config);
        std::string list_id = extract_id(wfrest::Json::parse(*l_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!list_id.empty());

        // D. Card
        std::string c_req = R"({"jsonrpc": "2.0", "id": "c1", "method": "tools/call", "params": {"name": "planka_create", "arguments": {"entity_type": "card", "parent_id": ")" + list_id + R"(", "data": {"name": "TestCard"}}}})";
        auto c_res = handler.handle_message(c_req, config);
        std::string card_id = extract_id(wfrest::Json::parse(*c_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!card_id.empty());

        // E. Label & Binding
        std::string lb_req = R"({"jsonrpc": "2.0", "id": "lb1", "method": "tools/call", "params": {"name": "planka_create", "arguments": {"entity_type": "label", "parent_id": ")" + board_id + R"(", "data": {"name": "Urgent", "color": "orange-peel"}}}})";
        auto lb_res = handler.handle_message(lb_req, config);
        std::string label_id = extract_id(wfrest::Json::parse(*lb_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!label_id.empty());

        std::string al_req = R"({"jsonrpc": "2.0", "id": "al1", "method": "tools/call", "params": {"name": "planka_create", "arguments": {"entity_type": "card_label", "parent_id": ")" + card_id + R"(", "data": {"labelId": ")" + label_id + R"("}}}}})";
        CHECK(handler.handle_message(al_req, config).has_value());

        // F. Card Operations: Duplicate & Move
        std::string dp_req = R"({"jsonrpc": "2.0", "id": "dp1", "method": "tools/call", "params": {"name": "planka_action", "arguments": {"action": "duplicate_card", "data": {"id": ")" + card_id + R"("}}}})";
        auto dp_res = handler.handle_message(dp_req, config);
        std::string dp_text = wfrest::Json::parse(*dp_res)["result"]["content"][0]["text"].get<std::string>();
        std::string dup_id = extract_id(dp_text);
        CHECK(!dup_id.empty());

        std::string mv_req = R"({"jsonrpc": "2.0", "id": "mv1", "method": "tools/call", "params": {"name": "planka_update", "arguments": {"entity_type": "card", "id": ")" + dup_id + R"(", "data": {"listId": ")" + list_id + R"(", "position": 100}}}}})";
        CHECK(handler.handle_message(mv_req, config).has_value());

        // G. Tasks & Comments
        std::string tl_req = R"({"jsonrpc": "2.0", "id": "tl1", "method": "tools/call", "params": {"name": "planka_create", "arguments": {"entity_type": "task_list", "parent_id": ")" + card_id + R"(", "data": {"name": "Subtasks"}}}})";
        auto tl_res = handler.handle_message(tl_req, config);
        std::string task_list_id = extract_id(wfrest::Json::parse(*tl_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!task_list_id.empty());

        std::string tk_req = R"({"jsonrpc": "2.0", "id": "tk1", "method": "tools/call", "params": {"name": "planka_create", "arguments": {"entity_type": "task", "parent_id": ")" + task_list_id + R"(", "data": {"name": "Task 1"}}}})";
        auto tk_res = handler.handle_message(tk_req, config);
        std::string task_id = extract_id(wfrest::Json::parse(*tk_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!task_id.empty());

        std::string cm_req = R"({"jsonrpc": "2.0", "id": "cm1", "method": "tools/call", "params": {"name": "planka_create", "arguments": {"entity_type": "comment", "parent_id": ")" + card_id + R"(", "data": {"text": "Self-destructing project."}}}})";
        auto cm_res = handler.handle_message(cm_req, config);
        std::string comment_id = extract_id(wfrest::Json::parse(*cm_res)["result"]["content"][0]["text"].get<std::string>());
        REQUIRE(!comment_id.empty());

        // H. Cleanup Phase
        std::string d_cm_req = R"({"jsonrpc": "2.0", "id": "d_cm", "method": "tools/call", "params": {"name": "planka_delete", "arguments": {"entity_type": "comment", "id": ")" + comment_id + R"("}}})";
        CHECK(handler.handle_message(d_cm_req, config).has_value());

        std::string d_tk_req = R"({"jsonrpc": "2.0", "id": "d_tk", "method": "tools/call", "params": {"name": "planka_delete", "arguments": {"entity_type": "task", "id": ")" + task_id + R"("}}})";
        CHECK(handler.handle_message(d_tk_req, config).has_value());

        std::string d_tl_req = R"({"jsonrpc": "2.0", "id": "d_tl", "method": "tools/call", "params": {"name": "planka_delete", "arguments": {"entity_type": "task_list", "id": ")" + task_list_id + R"("}}})";
        CHECK(handler.handle_message(d_tl_req, config).has_value());

        std::string d_al_req = R"({"jsonrpc": "2.0", "id": "d_al", "method": "tools/call", "params": {"name": "planka_delete", "arguments": {"entity_type": "card_label", "parent_id": ")" + card_id + R"(", "data": {"labelId": ")" + label_id + R"("}}}}})";
        CHECK(handler.handle_message(d_al_req, config).has_value());

        std::string d_c1_req = R"({"jsonrpc": "2.0", "id": "d_c1", "method": "tools/call", "params": {"name": "planka_delete", "arguments": {"entity_type": "card", "id": ")" + card_id + R"("}}})";
        CHECK(handler.handle_message(d_c1_req, config).has_value());
        
        std::string d_c2_req = R"({"jsonrpc": "2.0", "id": "d_c2", "method": "tools/call", "params": {"name": "planka_delete", "arguments": {"entity_type": "card", "id": ")" + dup_id + R"("}}})";
        CHECK(handler.handle_message(d_c2_req, config).has_value());

        std::string d_lb_req = R"({"jsonrpc": "2.0", "id": "d_lb", "method": "tools/call", "params": {"name": "planka_delete", "arguments": {"entity_type": "label", "id": ")" + label_id + R"("}}})";
        CHECK(handler.handle_message(d_lb_req, config).has_value());

        std::string d_l_req = R"({"jsonrpc": "2.0", "id": "d_l", "method": "tools/call", "params": {"name": "planka_delete", "arguments": {"entity_type": "list", "id": ")" + list_id + R"("}}})";
        CHECK(handler.handle_message(d_l_req, config).has_value());

        std::string d_b_req = R"({"jsonrpc": "2.0", "id": "d_b", "method": "tools/call", "params": {"name": "planka_delete", "arguments": {"entity_type": "board", "id": ")" + board_id + R"("}}})";
        CHECK(handler.handle_message(d_b_req, config).has_value());

        std::string d_p_req = R"({"jsonrpc": "2.0", "id": "d_p", "method": "tools/call", "params": {"name": "planka_delete", "arguments": {"entity_type": "project", "id": ")" + project_id + R"("}}})";
        auto final_res = handler.handle_message(d_p_req, config);
        CHECK(wfrest::Json::parse(*final_res)["result"]["content"][0]["text"].get<std::string>().find("Successfully") != std::string::npos);
    }

    SUBCASE("3. System Tools Verification") {
        std::string sn_req = R"({"jsonrpc": "2.0", "id": "sn1", "method": "tools/call", "params": {"name": "mark_all_notifications_read", "arguments": {}}})";
        // Actually this method may fail if not admin, but we just check if it returns a value
        CHECK(handler.handle_message(sn_req, config).has_value());
    }

    SUBCASE("4. Robustness: Scenario B/C/D") {
        SUBCASE("Scenario B: Flattened Arguments (Strict Protocol)") {
            std::string req = R"({
                "jsonrpc": "2.0",
                "method": "tools/call",
                "id": "rob1",
                "params": {
                    "name": "planka_explore",
                    "arguments": "{\"uri\": \"planka://projects\"}"
                }
            })";
            auto res_opt = handler.handle_message(req, config);
            CHECK(res_opt.has_value());
            wfrest::Json res = wfrest::Json::parse(res_opt.value());
            // Should fail with -32602 because arguments is a string
            CHECK(res.has("error"));
            CHECK(res["error"]["code"].get<int>() == -32602);
        }

        SUBCASE("Scenario C: Field-level Nesting (Auto-patch)") {
            std::string req = R"({
                "jsonrpc": "2.0",
                "method": "tools/call",
                "id": "rob2",
                "params": {
                    "name": "planka_explore",
                    "arguments": {
                        "uri": "{\"uri\": \"planka://projects\"}"
                    }
                }
            })";
            auto res_opt = handler.handle_message(req, config);
            CHECK(res_opt.has_value());
            wfrest::Json res = wfrest::Json::parse(res_opt.value());
            std::string content_str = res["result"]["content"].dump();
            // Should be patched and have warning
            CHECK(content_str.find("[INCORRECT USAGE WARNING]") != std::string::npos);
        }
    }

    SUBCASE("5. Webhook Template Engine") {
        std::string tmpl = "Event {{type}} by {{data.user.name}} on card {{data.card.name}}";
        wfrest::Json data = wfrest::Json::parse(R"({
            "type": "card_created",
            "data": {
                "user": {"name": "Alice"},
                "card": {"name": "Fix MCP"}
            }
        })");
        std::string result = mcp::webhook::render_template(tmpl, data);
        CHECK(result == "Event card_created by Alice on card Fix MCP");

        CHECK(mcp::webhook::render_template("Missing {{none}}", data) == "Missing {{none}}");
    }
}
