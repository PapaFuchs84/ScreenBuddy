// (c) 2026 FuchsLabs - Pegel-/Spektrumanzeige (Amplifier) für die Spotify-Seite.
// Liest die Systemausgabe per WASAPI-Loopback (nur lokal, es wird nichts aufgezeichnet oder gespeichert:
// die Samples liegen kurz in einem kleinen Ringpuffer und werden sofort zu 16 Balken verrechnet).
#pragma once
#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace fl::audio {

constexpr int Bands = 16;

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
    void touch();                 // Die Seite ist gerade sichtbar: Analyse aktiv halten (läuft sonst nicht)
    bool active() const;          // zuletzt vor < 3 s berührt
    bool available() const { return available_; }   // Ausgabegerät lesbar
    std::vector<float> levels();  // Bands Werte 0..1 (leer, wenn nicht verfügbar)

private:
    void run();
    std::function<void()> on_update_;
    std::atomic<bool> stop_{false}, available_{false};
    std::atomic<long long> touched_{0};
    std::mutex mu_;
    float levels_[Bands]{};
    std::thread thread_;
};

}  // namespace fl::audio