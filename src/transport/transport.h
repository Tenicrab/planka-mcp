#pragma once
#include <functional>
#include <optional>
#include <string>
#include "planka/client.h"

class Transport {
public:
    virtual ~Transport() = default;

    using MessageHandler = std::function<std::optional<std::string>(const std::string&, const PlankaClient::Config&)>;
    
    virtual void run(MessageHandler handler) = 0;
};
