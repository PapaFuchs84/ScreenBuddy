// © 2026 FuchsLabs
#include "sysinfo.h"

#include <winsock2.h>
#include <ws2ipdef.h>
#include <windows.h>
#include <iphlpapi.h>
#include <pdh.h>
#include <pdhmsg.h>

#include <algorithm>
#include <cstring>
#include <regex>

#include "util.h"

namespace fl::sysinfo {

// ---------- Netzwerk ----------
static std::optional<uint32_t> default_interface() {
    // ifIndex des Adapters, über den Internet-Verkehr läuft (wie Windows' eigene Routenwahl)
    DWORD idx = 0;
    IPAddr dest = 0x01010101;  // 1.1.1.1 (network byte order: 1.1.1.1)
    if (GetBestInterface(dest, &idx) == NO_ERROR) return idx;
    return std::nullopt;
}

template <class F>
static void for_each_if(F f) {
    PMIB_IF_TABLE2 table = nullptr;
    if (GetIfTable2(&table) != NO_ERROR) return;
    for (ULONG i = 0; i < table->NumEntries; ++i) f(table->Table[i]);
    FreeMibTable(table);
}

static std::optional<uint32_t> interface_index(const std::string& name) {
    // ifIndex zu einem Adapternamen (z. B. "Skynet") oder einem Teil der Beschreibung
    std::string want = casefold(name);
    std::optional<uint32_t> hit;
    for_each_if([&](const MIB_IF_ROW2& r) {
        if (hit) return;
        if (casefold(narrow(r.Alias)) == want || casefold(narrow(r.Description)).find(want) != std::string::npos)
            hit = r.InterfaceIndex;
    });
    return hit;
}

ojson list_interfaces() {
    // Aktive Netzwerkadapter (ohne Loopback und virtuelle Hilfsadapter)
    static const std::regex skip("WAN Miniport|Filter|Scheduler|Extension Adapter|Loopback|Pseudo");
    ojson out = ojson::array();
    std::vector<std::string> seen;
    for_each_if([&](const MIB_IF_ROW2& r) {
        std::string alias = narrow(r.Alias), desc = narrow(r.Description);
        if (r.OperStatus != IfOperStatusUp || r.Type == IF_TYPE_SOFTWARE_LOOPBACK || alias.empty()) return;
        if (std::regex_search(desc + " " + alias, skip)) return;
        if (std::count(seen.begin(), seen.end(), alias)) return;
        seen.push_back(alias);
        out.push_back({{"name", alias}, {"description", desc}});
    });
    return out;
}

// ---------- CPU ----------
std::pair<std::string, int> cpu_name() {
    HKEY k;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &k))
        return {"CPU", 0};
    wchar_t buf[256] = {};
    DWORD sz = sizeof buf, mhz = 0, msz = sizeof mhz;
    RegQueryValueExW(k, L"ProcessorNameString", nullptr, nullptr, (BYTE*)buf, &sz);
    RegQueryValueExW(k, L"~MHz", nullptr, nullptr, (BYTE*)&mhz, &msz);
    RegCloseKey(k);
    static const std::regex rx(
        R"(\(R\)|\(TM\)|^AMD |^Intel |\s+\d+-Core.*$|\s+Processor.*$|\s+with .*$|\s+CPU\s+@.*$)");
    std::string name = std::regex_replace(trim(narrow(buf)), rx, "");
    name = collapse_spaces(name);
    return {name.empty() ? "CPU" : name, (int)mhz};
}

struct SysMonitor::Pdh {
    PDH_HQUERY q = nullptr;
    PDH_HCOUNTER cpu = nullptr, perf = nullptr;
    Pdh() {
        if (PdhOpenQueryW(nullptr, 0, &q) != ERROR_SUCCESS) { q = nullptr; return; }
        // wie im Task-Manager ("% Processor Utility" statt Leerlaufzeit)
        if (PdhAddEnglishCounterW(q, L"\\Processor Information(_Total)\\% Processor Utility", 0, &cpu) != ERROR_SUCCESS)
            cpu = nullptr;
        if (PdhAddEnglishCounterW(q, L"\\Processor Information(_Total)\\% Processor Performance", 0, &perf) != ERROR_SUCCESS)
            perf = nullptr;
        PdhCollectQueryData(q);
    }
    ~Pdh() {
        if (q) PdhCloseQuery(q);
    }
    std::optional<double> value(PDH_HCOUNTER c) {
        if (!c) return std::nullopt;
        PDH_FMT_COUNTERVALUE v{};
        if (PdhGetFormattedCounterValue(c, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, nullptr, &v) != ERROR_SUCCESS)
            return std::nullopt;
        return v.doubleValue;
    }
};

// ---------- HWiNFO ----------
std::optional<std::vector<Reading>> hwinfo_readings() {
    // Nur öffnen, nie anlegen
    HANDLE h = OpenFileMappingW(FILE_MAP_READ, FALSE, L"Global\\HWiNFO_SENS_SM2");
    if (!h) return std::nullopt;
    std::optional<std::vector<Reading>> result;
    const uint8_t* view = (const uint8_t*)MapViewOfFile(h, FILE_MAP_READ, 0, 0, 0);
    if (view) {
        MEMORY_BASIC_INFORMATION mbi{};
        size_t avail = VirtualQuery(view, &mbi, sizeof mbi) ? mbi.RegionSize : 0;
        auto u32 = [&](size_t o) { uint32_t v; memcpy(&v, view + o, 4); return v; };
        if (avail >= 44 && u32(0) == 0x53695748) {  // "HWiS"; "DEAD" = HWiNFO beendet
            uint32_t s_off = u32(20), s_size = u32(24), s_num = u32(28), r_off = u32(32), r_size = u32(36), r_num = u32(40);
            uint64_t need = std::max<uint64_t>((uint64_t)s_off + (uint64_t)s_size * s_num, (uint64_t)r_off + (uint64_t)r_size * r_num);
            if (need <= avail && s_size >= 264 && r_size >= 292) {
                auto cstr = [&](size_t o, size_t n) {
                    std::wstring w;
                    for (size_t i = 0; i < n && view[o + i]; ++i) w += (wchar_t)view[o + i];  // Latin-1
                    return narrow(w);
                };
                std::vector<std::string> sensors;
                for (uint32_t i = 0; i < s_num; ++i) {
                    size_t o = s_off + (size_t)i * s_size;
                    std::string n = cstr(o + 136, 128);
                    sensors.push_back(n.empty() ? cstr(o + 8, 128) : n);
                }
                std::vector<Reading> out;
                for (uint32_t i = 0; i < r_num; ++i) {
                    size_t o = r_off + (size_t)i * r_size;
                    Reading r;
                    r.type = u32(o);
                    uint32_t sidx = u32(o + 4);
                    r.label = cstr(o + 140, 128);
                    if (r.label.empty()) r.label = cstr(o + 12, 128);
                    r.unit = cstr(o + 268, 16);
                    memcpy(&r.value, view + o + 284, 8);
                    r.sensor = sidx < sensors.size() ? sensors[sidx] : "";
                    out.push_back(std::move(r));
                }
                result = std::move(out);
            }
        }
        UnmapViewOfFile(view);
    }
    CloseHandle(h);
    return result;
}

static std::optional<double> pick(const std::vector<Reading>& readings, uint32_t type, std::vector<std::string> wanted,
                                  const std::string& sensor_hint = "") {
    wanted.erase(std::remove(wanted.begin(), wanted.end(), std::string()), wanted.end());
    std::vector<const Reading*> cands;
    std::string hint = casefold(sensor_hint);
    for (auto& r : readings)
        if (r.type == type && (hint.empty() || casefold(r.sensor).find(hint) != std::string::npos)) cands.push_back(&r);
    for (auto& w : wanted)
        for (auto* r : cands)
            if (casefold(r->label) == casefold(w)) return r->value;
    for (auto& w : wanted)
        for (auto* r : cands)
            if (casefold(r->label).find(casefold(w)) != std::string::npos) return r->value;
    return std::nullopt;
}

ojson hwinfo_sensors() {
    auto r = hwinfo_readings();
    if (!r) return {{"available", false}, {"temps", ojson::array()}, {"fans", ojson::array()}};
    auto names = [&](uint32_t t) {
        std::vector<std::string> out;
        for (auto& x : *r)
            if (x.type == t && !std::count(out.begin(), out.end(), x.label)) out.push_back(x.label);
        return out;
    };
    return {{"available", true}, {"temps", names(SENSOR_TEMP)}, {"fans", names(SENSOR_FAN)}};
}

// ---------- Sammler ----------
SysMonitor::SysMonitor(const ojson& cfg) {
    auto s = [&](const char* k) { return cfg.contains(k) && cfg[k].is_string() ? cfg[k].get<std::string>() : std::string(); };
    net_adapter_ = trim(s("net_adapter"));
    cpu_temp_sensor_ = s("cpu_temp_sensor");
    cpu_fan_sensor_ = s("cpu_fan_sensor");
    auto [name, mhz] = cpu_name();
    snap_.cpu_name = name;
    base_mhz_ = mhz;
    std::string disk = s("disk");
    if (disk.empty()) disk = "C:";
    while (!disk.empty() && disk.back() == '\\') disk.pop_back();
    snap_.disk_label = disk;
    published_ = snap_;
}

SysMonitor::~SysMonitor() { stop(); }

void SysMonitor::start() {
    th_ = std::thread([this] { run(); });
}

void SysMonitor::stop() {
    stop_ = true;
    if (th_.joinable()) th_.join();
}

SysSnapshot SysMonitor::snapshot() {
    std::lock_guard lk(mu_);
    return published_;
}

void SysMonitor::run() {
    while (!stop_) {
        double t = mono();
        try {
            sample();
            std::lock_guard lk(mu_);
            published_ = snap_;
        } catch (const std::exception& e) {
            LOGW("sysinfo", "Systemwerte konnten nicht gelesen werden: %s", e.what());
        }
        double wait = std::max(0.1, 1.0 - (mono() - t));
        for (double w = 0; w < wait && !stop_; w += 0.05) Sleep(50);
    }
}

void SysMonitor::sample() {
    SysSnapshot& s = snap_;
    if (!pdh_) pdh_ = std::make_unique<Pdh>();  // beim ersten Aufruf anlegen; Werte gibt es ab dem zweiten Aufruf
    if (pdh_->q && PdhCollectQueryData(pdh_->q) == ERROR_SUCCESS) {
        if (auto v = pdh_->value(pdh_->cpu)) s.cpu_pct = std::min(100.0, *v);
        if (auto v = pdh_->value(pdh_->perf); v && base_mhz_) s.cpu_ghz = base_mhz_ * *v / 100 / 1000;
    }
    MEMORYSTATUSEX ms{sizeof ms};
    if (GlobalMemoryStatusEx(&ms)) {
        s.ram_total = ms.ullTotalPhys / double(1ull << 30);
        s.ram_used = (ms.ullTotalPhys - ms.ullAvailPhys) / double(1ull << 30);
        s.ram_pct = *s.ram_used / *s.ram_total * 100;
    }
    ULARGE_INTEGER freeb{}, total{};
    if (GetDiskFreeSpaceExW(widen(s.disk_label + "\\").c_str(), nullptr, &total, &freeb))
        s.disk_pct = total.QuadPart ? std::optional<double>((1 - (double)freeb.QuadPart / total.QuadPart) * 100) : std::nullopt;
    sample_net();
    if (mono() >= gpu_next_) {
        gpu_next_ = mono() + 2;
        sample_gpu();
        sample_hwinfo();
    }
}

void SysMonitor::sample_net() {
    SysSnapshot& s = snap_;
    if (!net_adapter_.empty() && !net_idx_) net_idx_ = interface_index(net_adapter_);
    std::optional<uint32_t> idx = net_adapter_.empty() ? default_interface() : net_idx_;
    MIB_IF_ROW2 row{};
    bool ok = false;
    if (idx) {
        row.InterfaceIndex = *idx;
        ok = GetIfEntry2(&row) == NO_ERROR;
    }
    if (!ok) {
        s.net_name = "kein Netzwerk";
        s.net_down = s.net_up = std::nullopt;
        net_prev_t_.reset();
        return;
    }
    double now = mono();
    bool have_prev = net_prev_t_ && net_prev_idx_ == *idx && now > *net_prev_t_;
    double dt = have_prev ? now - *net_prev_t_ : 0;
    uint64_t pin = net_prev_in_, pout = net_prev_out_;
    net_prev_t_ = now;
    net_prev_idx_ = *idx;
    net_prev_in_ = row.InOctets;
    net_prev_out_ = row.OutOctets;
    s.net_name = narrow(row.Alias);
    s.net_link = row.ReceiveLinkSpeed ? std::optional<uint64_t>(row.ReceiveLinkSpeed) : std::nullopt;
    if (have_prev) {
        // Zähler können beim Adapter-Reset zurückspringen → negativ = 0
        s.net_down = (row.InOctets >= pin ? double(row.InOctets - pin) : 0.0) / dt;
        s.net_up = (row.OutOctets >= pout ? double(row.OutOctets - pout) : 0.0) / dt;
        s.down_hist.push_back(*s.net_down);
        s.up_hist.push_back(*s.net_up);
        while (s.down_hist.size() > 60) s.down_hist.pop_front();
        while (s.up_hist.size() > 60) s.up_hist.pop_front();
    }
}

void SysMonitor::sample_gpu() {
    SysSnapshot& s = snap_;
    std::string out = run_hidden(
        L"nvidia-smi --query-gpu=name,utilization.gpu,temperature.gpu,fan.speed,memory.used,memory.total "
        L"--format=csv,noheader,nounits",
        5000);
    if (trim(out).empty()) return;
    std::string first = split(trim(out), "\n")[0];
    auto parts = split(first, ",");
    if (parts.size() < 6) return;
    for (auto& p : parts) p = trim(p);
    auto num = [](const std::string& x) -> std::optional<double> {
        char* end = nullptr;
        double v = strtod(x.c_str(), &end);
        if (x.empty() || !end || *end) return std::nullopt;
        return v;
    };
    s.gpu_name = replace_all(replace_all(parts[0], "NVIDIA ", ""), "GeForce ", "");
    s.gpu_pct = num(parts[1]);
    s.gpu_temp = num(parts[2]);
    s.gpu_fan = num(parts[3]);
    s.gpu_mem_used = num(parts[4]);
    s.gpu_mem_total = num(parts[5]);
}

void SysMonitor::sample_hwinfo() {
    SysSnapshot& s = snap_;
    auto r = hwinfo_readings();
    s.hwinfo = r.has_value();
    if (!r || r->empty()) return;
    s.cpu_temp = pick(*r, SENSOR_TEMP, {cpu_temp_sensor_, "CPU (Tctl/Tdie)", "CPU Package", "CPU Die (average)", "CPU"});
    s.cpu_fan = pick(*r, SENSOR_FAN,
                     {cpu_fan_sensor_, "CPU", "CPU_OPT", "AIO Pump", "AIO Pumpe", "Pumpe", "Chassis1"});
    std::vector<Reading> disk;
    for (auto& x : *r) {
        auto seg = split(x.sensor, "[").back();
        if (x.sensor.find("S.M.A.R.T") != std::string::npos && seg.find(s.disk_label) != std::string::npos) disk.push_back(x);
    }
    s.disk_temp = pick(disk, SENSOR_TEMP, {"Drive Temperature", "Laufwerkstemperatur"});
}

}  // namespace fl::sysinfo
