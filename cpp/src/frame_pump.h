// © 2026 FuchsLabs
// Latest-frame handoff for slow synchronous outputs (USB serial display).
// The display loop submits a frame and continues immediately; a single worker
// sends it. A frame waiting while a transfer runs is replaced by newer ones, so
// the slow output never builds a backlog and never blocks Companion rendering.
#pragma once

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

#include "gfx.h"

namespace fl {

class FramePump {
public:
    struct Job {
        Image frame;
        bool full = false;      // send the complete image (page change, periodic refresh)
        bool playback = false;  // visible Spotify playback while transferring
        std::function<void()> done;  // after the send attempt; never for a replaced frame
    };
    using Send = std::function<void(const Job&)>;

    explicit FramePump(Send send) : send_(std::move(send)), worker_([this] { run(); }) {}
    ~FramePump() { stop(); }
    FramePump(const FramePump&) = delete;
    FramePump& operator=(const FramePump&) = delete;

    // Never waits for a running transfer. A full request stays sticky when a
    // newer partial frame replaces a pending full one.
    void submit(Job job) {
        {
            std::lock_guard lk(mu_);
            if (stopping_) return;
            if (pending_) {
                job.full = job.full || pending_->full;
                ++replaced_;
            }
            pending_ = std::move(job);
        }
        cv_.notify_one();
    }

    // Drops a pending frame and waits for an in-flight transfer to finish.
    void stop() {
        {
            std::lock_guard lk(mu_);
            if (stopping_ && !worker_.joinable()) return;
            stopping_ = true;
            pending_.reset();
        }
        cv_.notify_all();
        if (worker_.joinable() && worker_.get_id() != std::this_thread::get_id()) worker_.join();
    }

    bool busy() const { std::lock_guard lk(mu_); return sending_ || pending_.has_value(); }
    uint64_t replaced() const { std::lock_guard lk(mu_); return replaced_; }
    uint64_t sent() const { std::lock_guard lk(mu_); return sent_; }

private:
    void run() {
        for (;;) {
            Job job;
            {
                std::unique_lock lk(mu_);
                cv_.wait(lk, [&] { return stopping_ || pending_.has_value(); });
                if (stopping_) return;
                job = std::move(*pending_);
                pending_.reset();
                sending_ = true;
            }
            try { send_(job); } catch (...) {}  // Send reports its own errors.
            {
                std::lock_guard lk(mu_);
                sending_ = false;
                ++sent_;
            }
        }
    }

    Send send_;
    mutable std::mutex mu_;
    std::condition_variable cv_;
    std::optional<Job> pending_;
    bool stopping_ = false, sending_ = false;
    uint64_t replaced_ = 0, sent_ = 0;
    std::thread worker_;  // last member: starts after all state is initialised
};

}  // namespace fl
