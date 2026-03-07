#include "http_sse.h"
#include "../webhook/router.h"
#include <workflow/WFFacilities.h>
#include <iostream>

namespace mcp {

HttpSseTransport::HttpSseTransport(int port) : port_(port) {}

void HttpSseTransport::run(MessageHandler handler) {
    server_.POST("/message", [handler](const wfrest::HttpReq* req, wfrest::HttpResp* resp) {
        std::string body = req->body();
        if (body.empty()) {
            resp->set_status(400);
            return;
        }
        auto result = handler(body);
        if (result) {
            resp->add_header_pair("Content-Type", "application/json");
            resp->append_output_body(*result);
        } else {
            resp->set_status(500);
        }
    });

    server_.POST("/webhook/callback", [](const wfrest::HttpReq* req, wfrest::HttpResp* resp) {
        std::string body = req->body();
        if (body.empty()) {
            resp->set_status(400);
            return;
        }
        wfrest::Json event = wfrest::Json::parse(body);
        if (event.is_valid()) {
            webhook::Router::instance().handle_event(event);
            resp->set_status(200);
            resp->append_output_body(R"({"status":"ok"})");
        } else {
            resp->set_status(400);
            resp->append_output_body(R"({"error":"invalid json"})");
        }
    });

    if (server_.start(port_) == 0) {
        std::cout << "MCP HTTP+SSE Server running on port " << port_ << " for Webhooks..." << std::endl;
        WFFacilities::WaitGroup wait_group(1);
        wait_group.wait();
        server_.stop();
    } else {
        std::cerr << "Failed to start HTTP Server on port " << port_ << std::endl;
    }
}

} // namespace mcp
