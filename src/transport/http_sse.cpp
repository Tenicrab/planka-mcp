#include "http_sse.h"
#include "../webhook/router.h"
#include <iostream>
#include <ctime>
#include <cstdlib>
#include <sys/socket.h>
#include <workflow/WFFacilities.h>
#include <logger.h>

namespace mcp {

HttpSseTransport::HttpSseTransport(int port) : port_(port) {
    std::srand(std::time(nullptr));
}

void HttpSseTransport::run(MessageHandler handler) {
    // Webhook - POST /webhook/callback
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

    // 1. GET /sse - SSE 连接 (必须是 /sse 匹配官方行为规范)
    auto sse_handler = [this](const wfrest::HttpReq* req, wfrest::HttpResp* resp) {
        LOG_DEBUG() << "GET SSE CONNECTION RECEIVED - URI: " << req->get_request_uri();
                  
        std::string sessionId = "s_" + std::to_string(std::time(nullptr)) + "_" + std::to_string(std::rand() % 1000);
        auto session = std::make_shared<Session>();
        
        {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            sessions_[sessionId] = session;
        }

        resp->add_header("Content-Type", "text/event-stream");
        resp->add_header("Cache-Control", "no-cache");
        resp->add_header("Connection", "keep-alive");

        std::string endpointUrl = "/messages?sessionId=" + sessionId;

        // 使用 wfrest 官方推荐的 Push 方式发送 SSE 事件
        resp->Push(sessionId, [this, sessionId, session, endpointUrl](std::string &body) {
            LOG_DEBUG() << "Push Callback executing for " << sessionId;
            std::lock_guard<std::mutex> lock(session->mutex);
            body.clear();

            // 发送初始化 Endpoint
            if (!session->initial_sent) {
                LOG_DEBUG() << "Sending initial endpoint: " << endpointUrl;
                body.append("event: endpoint\n");
                body.append("data: " + endpointUrl + "\n\n");
                session->initial_sent = true;
            }

            // 发送所有积压的消息
            bool sent_msg = false;
            while (!session->messages.empty()) {
                LOG_DEBUG() << "Sending message from queue to " << sessionId;
                body.append("event: message\n");
                body.append("data: " + session->messages.front() + "\n\n");
                session->messages.pop();
                sent_msg = true;
            }

            if (!sent_msg && session->initial_sent) {
                body.append(": heartbeat\n\n");
            }
        }, [this, sessionId]() {
            // 当客户端断开连接时，清理 session
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            sessions_.erase(sessionId);
        });

        // 定时心跳，每 15 秒触发一次当前会话的 Push 发送一次心跳包
        // 这里使用 Functor 传值，避免闭包循环引用
        struct HeartbeatTimer {
            std::string sessionId;
            HttpSseTransport* transport;
            void operator()(WFTimerTask*) const {
                bool active = false;
                {
                    std::lock_guard<std::mutex> lock(transport->sessions_mutex_);
                    if (transport->sessions_.find(sessionId) != transport->sessions_.end()) {
                        active = true;
                    }
                }
                if (active) {
                    wfrest::sse_signal(sessionId);
                    // 递归启动下一个定时器（完全独立于当前 Series，防止阻塞）
                    WFTaskFactory::create_timer_task(15, 0, *this)->start();
                }
            }
        };
        HeartbeatTimer ht{sessionId, this};
        
        // 分别向独立的队列中丢入定时器
        WFTaskFactory::create_timer_task(15, 0, ht)->start();
        
        // 第一个端点建立事件 (延迟1毫秒确保push回调注册好)
        WFTaskFactory::create_timer_task(0, 1000 * 1000, [sessionId](WFTimerTask *timer) {
            LOG_DEBUG() << "Init timer fired, signaling session " << sessionId;
            wfrest::sse_signal(sessionId);
        })->start();
    };

    server_.GET("/", sse_handler);
    server_.GET("/sse", sse_handler);

    // 2. POST /messages - 处理 JSON-RPC
    auto message_handler = [this, handler](const wfrest::HttpReq* req, wfrest::HttpResp* resp) {
        std::string sessionId = req->query("sessionId");
        
        if (sessionId.empty()) {
            std::string uri = req->get_request_uri();
            auto pos = uri.find("sessionId=");
            if (pos != std::string::npos) {
                sessionId = uri.substr(pos + 10);
                auto and_pos = sessionId.find("&");
                if (and_pos != std::string::npos) sessionId = sessionId.substr(0, and_pos);
            }
        }

        LOG_DEBUG() << "POST RECEIVED - URI: " << req->get_request_uri() << " Session: " << sessionId;

        std::string url = req->header("X-Planka-Url");
        std::string key = req->header("X-Planka-Api-Key");

        if (url.empty() || key.empty()) {
            resp->set_status(401);
            resp->append_output_body("Missing Planka credentials");
            return;
        }

        std::shared_ptr<Session> session;
        if (!sessionId.empty()) {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            auto it = sessions_.find(sessionId);
            if (it != sessions_.end()) session = it->second;
        }

        if (!sessionId.empty() && !session) {
            LOG_ERROR() << "Session [" << sessionId << "] not found in map!";
            resp->set_status(404);
            resp->append_output_body("Session not found");
            return;
        }

        auto result = handler(req->body(), {url, key});

        if (result) {
            if (session) {
                // Async SSE mode
                {
                    std::lock_guard<std::mutex> lock(session->mutex);
                    session->messages.push(*result);
                }
                wfrest::sse_signal(sessionId);
                resp->set_status(202); 
            } else {
                // Synchronous Stateless HTTP POST mode
                LOG_DEBUG() << "Stateless HTTP POST response triggered. Returning synchronously.";
                resp->set_status(200);
                resp->add_header("Content-Type", "application/json");
                resp->append_output_body(*result);
            }
        } else {
            // No result produced
            if (!session) resp->set_status(200);
        }
    };

    server_.POST("/messages", message_handler);
    server_.POST("/message", message_handler); // 兼容旧客户端
    server_.POST("/sse", message_handler); // 有些客户端直接往初始地址 POST
    server_.POST("/", message_handler); // 为了保持 url 干净
    server_.POST("/mcp", message_handler); // 专属纯净 MCP 入口

    // 3. 通配符，拦截所有其他的 请求，看看它到底发到哪里去了
    // 为了彻底捕获 Antigravity 所有的请求，我们要把它的任何打底请求打印出来
    auto wild_handler = [](const wfrest::HttpReq* req, wfrest::HttpResp* resp) {
        std::cerr << "\n[STRAY REQUEST] Method: " << req->get_method() 
                  << " URI: " << req->get_request_uri() << "\n" << std::flush;
        resp->set_status(404);
        resp->append_output_body("Not found in debug track");
    };

    server_.POST("/*", wild_handler);
    server_.GET("/*", wild_handler);

    if (server_.start(AF_INET6, nullptr, static_cast<unsigned short>(port_)) == 0) {
        LOG_INFO() << "MCP Server (SSE) started on [::]" << ":" << port_ << " (dual-stack IPv4+IPv6)";
        WFFacilities::WaitGroup wait_group(1);
        wait_group.wait();
        server_.stop();
    } else {
        LOG_ERROR() << "Failed to start server on port " << port_;
    }
}

} // namespace mcp
