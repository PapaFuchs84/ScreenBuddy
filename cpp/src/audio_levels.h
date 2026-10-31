// (c) 2026 FuchsLabs - Pegel-/Spektrumanzeige (Amplifier) für die Spotify-Seite.
// Liest die Systemausgabe per WASAPI-Loopback (nur lokal, es wird nichts aufgezeichnet oder gespeichert:
// die Samples liegen kurz in einem kleinen Ringpuffer und werden sofort zu 16 Balken verrechnet).
#pragma once
#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace fl::audio {

constexpr int Bands = 16;
constexpr long long SilenceMs = 4000;   // so lange still bei laufender Wiedergabe → Wiedergabe ist woanders → simulieren

// Reine Logik (testbar): Spektrum von n Mono-Samples in `bands` logarithmische Bänder (≈ 60 Hz … 14 kHz), Werte 0..1.
void spectrum_bands(const float* mono, int n, int sample_rate, float* out, int bands);

// Schneller Anstieg, weicher Abfall bis exakt null.
bool smooth_levels(float* current, const float* target, int count);

class LevelMeter {
public:
    explicit LevelMeter(std::function<void()> on_update);
    ~LevelMeter();
    void start();
    void stop();
    void touch(uint32_t seed = 0);  // Die Seite ist gerade sichtbar: Analyse aktiv halten (läuft sonst nicht); seed = Titel für die Simulation
    bool simulated() const { return simulated_; }   // Balken sind simuliert (Wiedergabe läuft woanders, Systemausgabe bleibt still)
    bool active() const;          // zuletzt vor < 3 s berührt
    bool available() const { return available_; }   // Ausgabegerät lesbar
    std::vector<float> levels();  // Bands Werte 0..1 (leer, wenn nicht verfügbar)

private:
    void run();
    std::function<void()> on_update_;
    void sim_tick(float* cur, long long t, long long& last_calc);
    std::atomic<bool> stop_{false}, available_{false}, simulated_{false}, device_changed_{false};
    std::atomic<long long> touched_{0};
    std::atomic<uint32_t> seed_{0};
    long long last_signal_ = 0;               // nur im Meter-Thread
    std::mutex mu_;
    float levels_[Bands]{};
    std::thread thread_;
};

}  // namespace fl::audio