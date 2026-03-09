#pragma once
#include "transport.h"
#include <wfrest/HttpServer.h>
#include <map>
#include <mutex>
#include <queue>
#include <memory>
#include <string>

namespace mcp {

class HttpSseTransport : public Transport {
public:
    HttpSseTransport(int port);
    ~HttpSseTransport() override = default;

    void run(MessageHandler handler) override;

private:
    struct Session {
        std::queue<std::string> messages;
        std::mutex mutex;
        bool initial_sent = false;
    };

    int port_;
    wfrest::HttpServer server_;
    std::map<std::string, std::shared_ptr<Session>> sessions_;
    std::mutex sessions_mutex_;
};

} // namespace mcp
