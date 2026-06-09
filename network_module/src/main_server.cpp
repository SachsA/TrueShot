// Standalone `trueshot_server` entry point.
// All the server logic is in the Server class — see Network/Server.h.
// This file exists only to provide a `main()` so we don't ODR-violate
// by compiling Server.cpp twice (once into the library, once into the
// executable).

#include "Network/Server.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

int main(int argc, char** argv) {
    uint16_t port = Net::kDefaultPort;
    if (argc > 1) {
        const int p = std::atoi(argv[1]);
        if (p > 0 && p < 65536) port = static_cast<uint16_t>(p);
    }

    Net::Server server;
    if (!server.start(port)) return 1;

    using clock = std::chrono::steady_clock;
    auto prev   = clock::now();
    while (true) {
        const auto now = clock::now();
        const double dt =
            std::chrono::duration_cast<std::chrono::duration<double>>(now - prev).count();
        prev = now;

        server.step(dt);

        // Light sleep: under one tick so we stay responsive but don't burn
        // a core spinning. Real production uses a wait on a future event
        // (epoll/kqueue/IOCP) hooked into ENet — Phase 8.
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
}
