#pragma once
#include <functional>
#include <optional>
#include <string>

class Transport {
public:
    virtual ~Transport() = default;

    using MessageHandler = std::function<std::optional<std::string>(const std::string&)>;
    
    virtual void run(MessageHandler handler) = 0;
};
