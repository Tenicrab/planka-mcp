#pragma once
#include "transport.h"
#include "planka/client.h"

class StdioTransport : public Transport {
public:
    explicit StdioTransport(const PlankaClient::Config& config);
    void run(MessageHandler handler) override;

private:
    PlankaClient::Config config_;
};
