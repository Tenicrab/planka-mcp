#pragma once
#include "transport.h"

class StdioTransport : public Transport {
public:
    void run(MessageHandler handler) override;
};
