#include "stdio.h"
#include <iostream>

void StdioTransport::run(MessageHandler handler) {
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        auto response = handler(line);
        if (response) {
            std::cout << *response << "\n" << std::flush;
        }
    }
}
