// © 2026 FuchsLabs
// Systemwerte für die Systemansicht – nur Windows-APIs.
// Netzwerk: 64-Bit-Zähler des Adapters, über den die Standardroute läuft (nicht Loopback).
// CPU-Temperatur und Lüfter kommen aus dem Shared Memory von HWiNFO, sofern es läuft.
#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "config.h"
#include "models.h"

namespace fl::sysinfo {

struct Reading {
    uint32_t type;
    std::string sensor, label, unit;
    double value;
};
inline constexpr uint32_t SENSOR_TEMP = 1, SENSOR_FAN = 3;

std::optional<std::vector<Reading>> hwinfo_readings();
ojson list_interfaces();   // [{"name", "description"}]
ojson hwinfo_sensors();    // {"available", "temps", "fans"}
std::pair<std::string, int> cpu_name();

struct FrameStats {
    size_t intervals = 0;
    std::optional<double> fps, frametime_ms, low;
    std::deque<double> history;
};
// DXGI Present-call cadence, not scanout/displayed-frame FPS.
class FrameHistory {
public:
    void present(double timestamp);
    FrameStats stats(double now) const;
    void clear() { times_.clear(); }
private:
    std::deque<double> times_;
};
// One sample per second; invalid samples leave gaps and PID changes clear history.
class MetricHistory {
public:
    void sample(SysSnapshot& snapshot, uint32_t pid, double now);
private:
    uint32_t pid_=0;
    double sampled_=-1;
    std::deque<double> fps_, times_;
};
class SysMonitor {
public:
    explicit SysMonitor(const ojson& cfg);
    ~SysMonitor();
    void start();
    void stop();
    SysSnapshot snapshot();
    void set_game_pid(uint32_t pid) { game_pid_=pid; }
    void sample();               // eine Messung (auch für --demo)
    const SysSnapshot& raw() const { return snap_; }

private:
    void run();
    void sample_net();
    void sample_gpu();
    void sample_hwinfo();
    void sample_ping();

    std::string net_adapter_, cpu_temp_sensor_, cpu_fan_sensor_;
    SysSnapshot snap_, published_;
    int base_mhz_ = 0;
    std::mutex mu_;
    std::thread th_;
    std::atomic<bool> stop_{false};
    std::optional<double> net_prev_t_;
    uint32_t net_prev_idx_ = 0;
    uint64_t net_prev_in_ = 0, net_prev_out_ = 0;
    std::optional<uint32_t> net_idx_;
    double gpu_next_ = 0;
    std::string ping_target_;
    std::optional<double> ping_previous_;
    std::atomic<uint32_t> game_pid_{0};
    MetricHistory metric_history_;
    struct PresentTrace;
    std::unique_ptr<PresentTrace> trace_;
    struct GpuApi;
    std::unique_ptr<GpuApi> gpu_;
    struct Pdh;
    std::unique_ptr<Pdh> pdh_;
};

}  // namespace fl::sysinfo
