// © 2026 FuchsLabs
// Schaltbare Frame-Telemetrie für Companion-Geräte (Diagnose der Animationsglätte).
// Aktiv nur, wenn die Umgebungsvariable SB_FRAME_TRACE auf einen Dateipfad zeigt; sonst kostet record() nichts
// (ein einziger bool-Test, keine Uhr, kein Lock). Eine CSV-Zeile je tatsächlich gesendetem Frame, Auswertung:
// cpp/tools/frame_trace_stats.py. Plattformneutral (Standard-C++), daher auch in den Linux-Tests nutzbar.
#pragma once

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <mutex>
#include <string>

namespace fl::companion {

class FrameTrace {
public:
    struct Row {
        int64_t ts_us = 0;       // monotone Uhr (steady_clock), Zeitpunkt: Sendevorgang abgeschlossen
        std::string device;
        uint64_t seq = 0;        // Veröffentlichungsnummer des Servers (frame_changed)
        int64_t render_us = 0;   // Zeichnen dieses Frames in der App (0 = unbekannt)
        int64_t encode_us = 0;   // JPEG-Kodierung im Verbindungs-Thread (0 bei Zwischenspeicher-Treffer)
        int64_t send_us = 0;     // send_raw (TLS + Socket) bis zur Rückkehr
        uint64_t bytes = 0;      // JPEG-Nutzlast
        bool animating = false;  // Quelle meldet Animationsrate (producer-paced)
    };

    // Pfad leer = aus. Eine nicht beschreibbare Datei schaltet die Telemetrie ebenfalls aus (nie ein Fehler im Betrieb).
    explicit FrameTrace(const std::string& path = std::string()) {
        if (path.empty()) return;
#ifdef _WIN32
        if (fopen_s(&file_, path.c_str(), "ab") != 0) file_ = nullptr;
#else
        file_ = std::fopen(path.c_str(), "ab");
#endif
        if (!file_) return;
        std::fseek(file_, 0, SEEK_END);
        if (std::ftell(file_) == 0) std::fputs(header(), file_);
        on_ = true;
    }
    ~FrameTrace() { if (file_) std::fclose(file_); }
    FrameTrace(const FrameTrace&) = delete;
    FrameTrace& operator=(const FrameTrace&) = delete;

    // Prozessweite Instanz; liest SB_FRAME_TRACE genau einmal.
    static FrameTrace& get() {
        static FrameTrace instance(env_path());
        return instance;
    }

    bool enabled() const noexcept { return on_; }
    static int64_t now_us() {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    static const char* header() { return "ts_us,device,seq,render_us,encode_us,send_us,bytes,gap_us,animating\n"; }

    // Eine Zeile ohne Zeilenende-Pflicht des Aufrufers; gap_us = -1 beim ersten Frame eines Geräts.
    static std::string format(const Row& r, int64_t gap_us) {
        std::string dev = r.device;
        for (char& c : dev) if (c == ',' || c == '\n' || c == '\r' || c == '"') c = '_';
        return std::to_string(r.ts_us) + ',' + dev + ',' + std::to_string(r.seq) + ',' + std::to_string(r.render_us) + ',' +
               std::to_string(r.encode_us) + ',' + std::to_string(r.send_us) + ',' + std::to_string(r.bytes) + ',' +
               std::to_string(gap_us) + ',' + (r.animating ? '1' : '0') + '\n';
    }

    void record(const Row& r) {
        if (!on_) return;
        std::lock_guard lk(mu_);
        auto [it, fresh] = last_.try_emplace(r.device, r.ts_us);
        const int64_t gap = fresh ? -1 : r.ts_us - it->second;
        it->second = r.ts_us;
        const std::string line = format(r, gap);
        std::fwrite(line.data(), 1, line.size(), file_);
        // Am Ende einer Animation und sonst alle 32 Zeilen schreiben, damit ein harter Abbruch wenig verliert.
        if (!r.animating || ++pending_ >= 32) { std::fflush(file_); pending_ = 0; }
    }

private:
    static std::string env_path() {
#ifdef _WIN32
        char* value = nullptr;
        size_t len = 0;
        std::string out;
        if (_dupenv_s(&value, &len, "SB_FRAME_TRACE") == 0 && value) out = value;
        std::free(value);
        return out;
#else
        const char* value = std::getenv("SB_FRAME_TRACE");
        return value ? value : "";
#endif
    }

    bool on_ = false;
    std::FILE* file_ = nullptr;
    std::mutex mu_;
    std::map<std::string, int64_t> last_;
    int pending_ = 0;
};

}  // namespace fl::companion
