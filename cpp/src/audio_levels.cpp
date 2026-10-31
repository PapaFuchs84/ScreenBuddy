// (c) 2026 FuchsLabs - Pegel-/Spektrumanzeige, siehe audio_levels.h
#include "audio_levels.h"
#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <ks.h>
#include <ksmedia.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <complex>
#include "amp_logic.h"
#include "util.h"

namespace fl::audio {
namespace {
constexpr int FftN = 1024;
constexpr double Pi = 3.14159265358979323846;

void fft(std::vector<std::complex<float>>& a) {
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1) {
        const float ang = float(-2 * Pi / double(len));
        const std::complex<float> wl(std::cos(ang), std::sin(ang));
        for (size_t i = 0; i < n; i += len) {
            std::complex<float> w(1, 0);
            for (size_t k = 0; k < len / 2; ++k) {
                auto u = a[i + k], v = a[i + k + len / 2] * w;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
                w *= wl;
            }
        }
    }
}

long long now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

template <class T> void release(T*& p) { if (p) { p->Release(); p = nullptr; } }
}  // namespace

void spectrum_bands(const float* mono, int n, int sample_rate, float* out, int bands) {
    for (int b = 0; b < bands; ++b) out[b] = 0;
    if (n < FftN || sample_rate <= 0 || bands <= 0) return;
    std::vector<std::complex<float>> a(FftN);
    double wsum = 0;
    const float* x = mono + (n - FftN);   // jüngste Samples
    for (int i = 0; i < FftN; ++i) {
        float w = 0.5f - 0.5f * std::cos(float(2 * Pi * i / (FftN - 1)));
        a[size_t(i)] = {x[i] * w, 0};
        wsum += w;
    }
    fft(a);
    const double fmin = 60, fmax = std::min(14000.0, sample_rate / 2.0 - 1);
    for (int b = 0; b < bands; ++b) {
        const double lo = fmin * std::pow(fmax / fmin, double(b) / bands), hi = fmin * std::pow(fmax / fmin, double(b + 1) / bands);
        int k0 = std::max(1, int(std::floor(lo * FftN / sample_rate))), k1 = std::max(k0, int(std::ceil(hi * FftN / sample_rate)) - 1);
        k1 = std::min(k1, FftN / 2 - 1);
        double peak = 0;
        for (int k = k0; k <= k1; ++k) peak = std::max(peak, double(std::abs(a[size_t(k)])));
        const double amp = peak * 2.0 / wsum;                     // Amplitude relativ zur Vollaussteuerung
        double db = 20 * std::log10(amp + 1e-9);
        db += 3.0 * std::log2((lo + hi) / 2 / 1000.0 + 1.0);      // leichte Höhenanhebung (Musik fällt zu hohen Tönen ab)
        out[b] = float(std::clamp((db + 62.0) / 56.0, 0.0, 1.0));  // -62 dB … -6 dB
    }
}

bool smooth_levels(float* current, const float* target, int count) {
    bool changed = false;
    for (int b = 0; b < count; ++b) {
        float v = target[b] > current[b] ? current[b] + (target[b] - current[b]) * 0.75f : current[b] * 0.80f;
        if (v < 0.01f) v = 0;
        if (v != current[b]) changed = true;   // auch den letzten Abfall auf exakt null veröffentlichen
        current[b] = v;
    }
    return changed;
}

LevelMeter::LevelMeter(std::function<void()> on_update) : on_update_(std::move(on_update)) {}
LevelMeter::~LevelMeter() { stop(); }

void LevelMeter::start() {
    if (thread_.joinable()) return;
    stop_ = false;
    thread_ = std::thread([this] { run(); });
}

void LevelMeter::stop() {
    stop_ = true;
    if (thread_.joinable()) thread_.join();
    available_ = false;
    simulated_ = false;
}

void LevelMeter::touch(uint32_t seed) { touched_ = now_ms(); seed_ = seed; }
bool LevelMeter::active() const { return now_ms() - touched_.load() < 3000; }

std::vector<float> LevelMeter::levels() {
    if (!available_ && !simulated_) return {};
    std::lock_guard lk(mu_);
    return std::vector<float>(levels_, levels_ + Bands);
}

// Simulierte Balken für Remote-Wiedergabe (Handy, Lautsprecher, Connect): nur im Meter-Thread aufrufen.
void LevelMeter::sim_tick(float* cur, long long t, long long& last_calc) {
    if (t - last_calc < 70) return;
    last_calc = t;
    float next[Bands]{};
    simulate_levels(double(t) / 1000.0, seed_.load(), next, Bands);
    smooth_levels(cur, next, Bands);
    { std::lock_guard lk(mu_); std::copy(cur, cur + Bands, levels_); }
    simulated_ = true;
    if (on_update_) on_update_();
}

void LevelMeter::run() {
    const bool com = SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
    std::vector<float> ring;                   // Mono, nur die letzten FftN*2 Samples
    ring.reserve(FftN * 4);
    while (!stop_) {
        IMMDeviceEnumerator* en = nullptr;
        IMMDevice* dev = nullptr;
        IAudioClient* client = nullptr;
        IAudioCaptureClient* cap = nullptr;
        WAVEFORMATEX* fmt = nullptr;
        bool ok = false;
        if (SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&en))) &&
            SUCCEEDED(en->GetDefaultAudioEndpoint(eRender, eConsole, &dev)) &&
            SUCCEEDED(dev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&client))) &&
            SUCCEEDED(client->GetMixFormat(&fmt)) &&
            SUCCEEDED(client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 2000000, 0, fmt, nullptr)) &&
            SUCCEEDED(client->GetService(IID_PPV_ARGS(&cap))) && SUCCEEDED(client->Start()))
            ok = true;
        bool is_float = false, pcm16 = false, pcm32 = false;
        int ch = 0, sr = 0;
        if (ok) {
            ch = std::max<int>(1, fmt->nChannels);
            sr = int(fmt->nSamplesPerSec);
            if (fmt->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) is_float = true;
            else if (fmt->wFormatTag == WAVE_FORMAT_PCM) { pcm16 = fmt->wBitsPerSample == 16; pcm32 = fmt->wBitsPerSample == 32; }
            else if (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
                auto* ex = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(fmt);
                if (ex->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) is_float = true;
                else if (ex->SubFormat == KSDATAFORMAT_SUBTYPE_PCM) { pcm16 = fmt->wBitsPerSample == 16; pcm32 = fmt->wBitsPerSample == 32; }
            }
            if (!is_float && !pcm16 && !pcm32) ok = false;
        }
        if (!ok) {
            LOGW("audio", "Amplifier: Ausgabegerät nicht lesbar – neuer Versuch in 5 s");
            available_ = false;
        } else {
            LOGI("audio", "Amplifier: Systemausgabe wird gelesen (%d Hz, %d Kanäle)", sr, ch);
            available_ = true;
            long long last_data = now_ms(), last_calc = 0;
            last_signal_ = now_ms();
            bool was_active = false;
            float cur[Bands]{};
            ring.clear();
            while (!stop_) {
                Sleep(20);
                UINT32 packet = 0;
                if (FAILED(cap->GetNextPacketSize(&packet))) break;   // Gerät gewechselt/entfernt: neu aufbauen
                while (packet && !stop_) {
                    BYTE* data = nullptr;
                    UINT32 frames = 0;
                    DWORD flags = 0;
                    if (FAILED(cap->GetBuffer(&data, &frames, &flags, nullptr, nullptr))) { packet = 0; break; }
                    for (UINT32 i = 0; i < frames; ++i) {
                        float v = 0;
                        if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT)) {
                            for (int c = 0; c < ch; ++c) {
                                if (is_float) v += reinterpret_cast<const float*>(data)[size_t(i) * ch + c];
                                else if (pcm16) v += reinterpret_cast<const int16_t*>(data)[size_t(i) * ch + c] / 32768.f;
                                else v += reinterpret_cast<const int32_t*>(data)[size_t(i) * ch + c] / 2147483648.f;
                            }
                            v /= float(ch);
                        }
                        ring.push_back(v);
                    }
                    cap->ReleaseBuffer(frames);
                    last_data = now_ms();
                    if (FAILED(cap->GetNextPacketSize(&packet))) break;
                }
                if (ring.size() > size_t(FftN) * 2) ring.erase(ring.begin(), ring.end() - FftN * 2);
                const long long t = now_ms();
                if (!active()) { ring.clear(); was_active = false; simulated_ = false; continue; }   // Seite nicht sichtbar: nichts rechnen
                if (!was_active) { was_active = true; last_signal_ = t; }
                if (t - last_calc < 70) continue;                // ≈ 14 Aktualisierungen pro Sekunde
                float next[Bands]{};
                if (t - last_data < 250 && ring.size() >= size_t(FftN)) spectrum_bands(ring.data(), int(ring.size()), sr, next, Bands);
                float peak = 0;
                for (float v : next) peak = std::max(peak, v);
                if (peak > 0.03f) { last_signal_ = t; simulated_ = false; }
                if (t - last_signal_ >= SilenceMs) { sim_tick(cur, t, last_calc); continue; }   // Systemausgabe still, Wiedergabe läuft woanders
                last_calc = t;
                if (smooth_levels(cur, next, Bands)) {
                    { std::lock_guard lk(mu_); std::copy(cur, cur + Bands, levels_); }
                    if (on_update_) on_update_();
                }
            }
        }
        if (client) client->Stop();
        if (fmt) CoTaskMemFree(fmt);
        release(cap); release(client); release(dev); release(en);
        available_ = false;
        // bei Fehler 5 s warten; ohne lesbares Ausgabegerät läuft die Wiedergabe sicher woanders → simulieren
        {
            float sim_cur[Bands]{};
            long long sim_calc = 0, quiet_since = now_ms();
            for (int i = 0; i < 100 && !stop_ && !ok; ++i) {
                Sleep(50);
                if (active() && now_ms() - quiet_since >= SilenceMs) sim_tick(sim_cur, now_ms(), sim_calc);
                else if (!active()) { quiet_since = now_ms(); simulated_ = false; }
            }
        }
        if (ok && !stop_) Sleep(500);
    }
    if (com) CoUninitialize();
}

}  // namespace fl::audio