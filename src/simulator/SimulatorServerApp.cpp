/**
 * SimulatorServerApp — headless host: FeatureHub discovery + control API + physics.
 * Usage:
 *   SimulatorServer [--port 20780] [--announce] [--track DIR] [--ai N] [--name NAME]
 */
#include "SimulationLoop.h"
#include "FeatureHub.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>

namespace {
std::atomic<bool> g_run{true};
void onSig(int) { g_run = false; }
}

int main(int argc, char** argv) {
    bool announce = false;
    int ai = 0;
    std::string trackDir;
    std::string hostName = "ksengine-server";
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--announce")) announce = true;
        else if (!std::strcmp(argv[i], "--ai") && i + 1 < argc) ai = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--track") && i + 1 < argc) trackDir = argv[++i];
        else if (!std::strcmp(argv[i], "--name") && i + 1 < argc) hostName = argv[++i];
        else if (!std::strcmp(argv[i], "--help")) {
            std::fprintf(stderr, "SimulatorServer [--announce] [--track DIR] [--ai N] [--name NAME]\n");
            return 0;
        }
    }

    std::signal(SIGINT, onSig);
    std::signal(SIGTERM, onSig);

    ks::sim::SimulationLoop loop;
    if (!loop.initialize()) {
        std::fprintf(stderr, "SimulatorServer: initialize failed\n");
        return 1;
    }
    if (!trackDir.empty())
        loop.loadTrackFolder(trackDir);

    loop.features().setSessionMode(ks::sim::GameSessionMode::Practice);
    loop.startFeatureServices(announce);
    if (announce)
        loop.features().announceHost(hostName, trackDir.empty() ? "unknown" : trackDir, 9600, 1, 24);

    if (ai > 0)
        loop.setAiCarCount(ai);

    loop.beginSession(ks::sim::GameSessionMode::Practice);
    loop.start();

    std::fprintf(stderr, "SimulatorServer: running (discovery UDP :20779, control TCP :20780)\n");
    auto last = std::chrono::steady_clock::now();
    while (g_run) {
        loop.tick();
        auto now = std::chrono::steady_clock::now();
        double dt = std::chrono::duration<double>(now - last).count();
        last = now;
        if (dt < 1.0 / 120.0)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    loop.stop();
    loop.features().stopServices();
    std::fprintf(stderr, "SimulatorServer: stopped\n");
    return 0;
}
