// © 2026 FuchsLabs – isolated worker, immutable configuration until restart.
#pragma once
#include "game_logo.h"
#include <atomic>
#include <mutex>
#include <thread>
namespace fl::game {
struct Snapshot { std::optional<Game> game;ImagePtr logo;double started=0;uint64_t generation=0; };
class Runtime {
public:
    Runtime(std::filesystem::path cache,std::string additional,std::string excluded,std::function<void(uint32_t)> changed);
    ~Runtime();
    void start();void stop();Snapshot snapshot() const;
private:
    void run();
    std::filesystem::path cache_;
    Discovery discovery_;
    Detector detector_;
    std::function<void(uint32_t)> changed_;
    mutable std::mutex mutex_;
    Snapshot snapshot_;
    std::atomic<bool> stop_{false};
    std::thread thread_;
};
}
