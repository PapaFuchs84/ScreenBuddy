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

class SysMonitor {
public:
    explicit SysMonitor(const ojson& cfg);
    ~SysMonitor();
    void start();
    void stop();
    SysSnapshot snapshot();
    void sample();               // eine Messung (auch für --demo)
    const SysSnapshot& raw() const { return snap_; }

private:
    void run();
    void sample_net();
    void sample_gpu();
    void sample_hwinfo();

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
    struct Pdh;
    std::unique_ptr<Pdh> pdh_;
};

}  // namespace fl::sysinfo
