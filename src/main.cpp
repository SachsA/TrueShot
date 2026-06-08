#include "application.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

namespace {

void printUsage(const char* prog) {
    std::cout << "Usage: " << prog << " [options]\n"
              << "  --offline               Solo practice range (default).\n"
              << "  --server host[:port]    Connect to a TrueShot server.\n"
              << "                          Default port: 7777.\n"
              << "  --help                  Print this message and exit.\n";
}

bool parseHostPort(const std::string& arg, std::string& outHost, uint16_t& outPort) {
    const auto colon = arg.find(':');
    if (colon == std::string::npos) {
        outHost = arg;
        return true;
    }
    outHost             = arg.substr(0, colon);
    const std::string p = arg.substr(colon + 1);
    char* end           = nullptr;
    const long port     = std::strtol(p.c_str(), &end, 10);
    if (end == p.c_str() || port <= 0 || port > 65535) return false;
    outPort = static_cast<uint16_t>(port);
    return true;
}

bool parseArgs(int argc, char** argv, AppConfig& cfg) {
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--help" || a == "-h") {
            printUsage(argv[0]);
            return false;
        } else if (a == "--offline") {
            cfg.mode = AppConfig::Mode::Offline;
        } else if (a == "--server") {
            if (i + 1 >= argc) {
                std::cerr << "--server requires a host[:port] argument\n";
                return false;
            }
            if (!parseHostPort(argv[++i], cfg.serverHost, cfg.serverPort)) {
                std::cerr << "Invalid --server argument: " << argv[i] << '\n';
                return false;
            }
            cfg.mode = AppConfig::Mode::Client;
        } else {
            std::cerr << "Unknown argument: " << a << "\n";
            printUsage(argv[0]);
            return false;
        }
    }
    return true;
}

} // namespace

int main(int argc, char** argv) {
    AppConfig cfg;
    cfg.width  = 1280;
    cfg.height = 720;
    cfg.title  = "TrueShot - Tactical FPS";

    if (!parseArgs(argc, argv, cfg)) return 1;

    Application app;
    if (!app.init(cfg)) {
        std::cerr << "Failed to initialize TrueShot.\n";
        return 1;
    }
    return app.run();
}
