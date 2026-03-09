#include "stdio.h"
#include <iostream>

StdioTransport::StdioTransport(const PlankaClient::Config& config) : config_(config) {}

void StdioTransport::run(MessageHandler handler) {
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        auto response = handler(line, config_);
        if (response) {
            std::cout << *response << "\n" << std::flush;
        }
    }
}
