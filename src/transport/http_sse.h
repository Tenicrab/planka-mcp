#pragma once
#include "transport.h"
#include <wfrest/HttpServer.h>

namespace mcp {

class HttpSseTransport : public Transport {
public:
    HttpSseTransport(int port);
    ~HttpSseTransport() override = default;

    void run(MessageHandler handler) override;

private:
    int port_;
    wfrest::HttpServer server_;
};

} // namespace mcp
