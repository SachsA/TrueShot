// Standalone `trueshot_server` entry point.
// All the server logic is in the Server class — see netcode/server.h.
// This file exists only to provide a `main()` so we don't ODR-violate
// by compiling Server.cpp twice (once into the library, once into the
// executable).

#include "netcode/server.h"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

namespace {

void printUsage(const char* argv0) {
    std::cout << "Usage: " << argv0 << " [options]\n"
              << "\n"
              << "Options:\n"
              << "  --port <N>             UDP listen port (default 7777)\n"
              << "  --simulate-loss <P>    Drop packets with probability P (0.0-1.0)\n"
              << "  --simulate-delay <ms>  Add a fixed delay to every outbound packet\n"
              << "  --simulate-jitter <ms> Add uniform random +/- jitter to outbound\n"
              << "  --help                 Show this help and exit\n"
              << "\n"
              << "Examples:\n"
              << "  " << argv0 << "                                    # 7777, no sim\n"
              << "  " << argv0 << " --port 9000\n"
              << "  " << argv0 << " --simulate-loss 0.05               # 5% loss\n"
              << "  " << argv0 << " --simulate-delay 50 --simulate-jitter 20\n";
}

bool parseUint(const char* s, uint32_t& out) {
    if (!s) return false;
    char* end    = nullptr;
    const long v = std::strtol(s, &end, 10);
    if (end == s || *end != '\0' || v < 0) return false;
    out = static_cast<uint32_t>(v);
    return true;
}

bool parseFloat(const char* s, float& out) {
    if (!s) return false;
    char* end      = nullptr;
    const double v = std::strtod(s, &end);
    if (end == s || *end != '\0') return false;
    out = static_cast<float>(v);
    return true;
}

} // namespace

int main(int argc, char** argv) {
    uint16_t port = Net::kDefaultPort;
    Net::Server::NetSimSettings sim;

    // Backwards-compat: bare positional first arg = port (preserved from
    // Phase 1.5 so old smoke scripts keep working).
    int argi = 1;
    if (argc >= 2 && argv[1][0] != '-') {
        uint32_t p = 0;
        if (parseUint(argv[1], p) && p > 0 && p < 65536) {
            port = static_cast<uint16_t>(p);
            argi = 2;
        }
    }

    for (; argi < argc; ++argi) {
        const char* a = argv[argi];
        if (std::strcmp(a, "--help") == 0 || std::strcmp(a, "-h") == 0) {
            printUsage(argv[0]);
            return 0;
        }
        if (std::strcmp(a, "--port") == 0 && argi + 1 < argc) {
            uint32_t p = 0;
            if (!parseUint(argv[++argi], p) || p == 0 || p > 65535) {
                std::cerr << "[Server] invalid --port value\n";
                return 1;
            }
            port = static_cast<uint16_t>(p);
            continue;
        }
        if (std::strcmp(a, "--simulate-loss") == 0 && argi + 1 < argc) {
            if (!parseFloat(argv[++argi], sim.lossProbability) || sim.lossProbability < 0.0f ||
                sim.lossProbability > 1.0f) {
                std::cerr << "[Server] --simulate-loss must be in [0.0, 1.0]\n";
                return 1;
            }
            continue;
        }
        if (std::strcmp(a, "--simulate-delay") == 0 && argi + 1 < argc) {
            if (!parseUint(argv[++argi], sim.baseDelayMs) || sim.baseDelayMs > 5000) {
                std::cerr << "[Server] --simulate-delay must be 0..5000 (ms)\n";
                return 1;
            }
            continue;
        }
        if (std::strcmp(a, "--simulate-jitter") == 0 && argi + 1 < argc) {
            if (!parseUint(argv[++argi], sim.jitterMs) || sim.jitterMs > 1000) {
                std::cerr << "[Server] --simulate-jitter must be 0..1000 (ms)\n";
                return 1;
            }
            continue;
        }
        std::cerr << "[Server] unknown argument: " << a << "\n";
        printUsage(argv[0]);
        return 1;
    }

    Net::Server server;
    server.setNetSimSettings(sim);

    if (!server.start(port)) return 1;

    if (sim.lossProbability > 0.0f || sim.baseDelayMs > 0 || sim.jitterMs > 0) {
        std::cout << "[Server] NetSim ENABLED:"
                  << "  loss=" << (sim.lossProbability * 100.0f) << "%"
                  << "  delay=" << sim.baseDelayMs << " ms"
                  << "  jitter=+/-" << sim.jitterMs << " ms\n";
    }

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
