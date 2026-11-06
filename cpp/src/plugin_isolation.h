// © 2026 FuchsLabs – Fehlerisolierung der Plugin-Blöcke: Ratenbegrenzung und feste Log-Texte (Phase 3).
// Header-only und rein (kein Logging, keine Uhr, kein Windows-API), damit es ohne Grafik getestet werden kann.
// Log-Zeilen enthalten nie Daten, Einstellungswerte, Secrets oder Ausnahmetexte (what()): nur Plugin-ID,
// Block-Schlüssel (v2: Block-ID, v1: "#<Index>") und die Stufe.
#pragma once
#include <chrono>
#include <cstddef>
#include <map>
#include <string>
#include <string_view>

namespace fl::plugins::isolation {
enum class Stage { data, draw };

// Höchstens ein Eintrag je Schlüssel und Intervall; höchstens `max_keys` Schlüssel werden verfolgt (abgelaufene
// werden entfernt, ist danach kein Platz, wird verworfen statt zu wachsen). Nicht threadsicher: der Aufrufer sperrt.
class RateLimiter {
public:
    using Clock = std::chrono::steady_clock;
    explicit RateLimiter(Clock::duration interval = std::chrono::seconds(60), std::size_t max_keys = 64)
        : interval_(interval), max_keys_(max_keys) {}
    bool allow(const std::string& key, Clock::time_point now) {
        auto it = last_.find(key);
        if (it != last_.end()) {
            if (now - it->second < interval_) return false;
            it->second = now;
            return true;
        }
        if (last_.size() >= max_keys_)
            for (auto old = last_.begin(); old != last_.end();) old = now - old->second >= interval_ ? last_.erase(old) : std::next(old);
        if (last_.size() >= max_keys_) return false;
        last_.emplace(key, now);
        return true;
    }
    std::size_t tracked() const { return last_.size(); }
private:
    Clock::duration interval_;
    std::size_t max_keys_;
    std::map<std::string, Clock::time_point> last_;
};

// Nur [a-z0-9_#-], höchstens `limit` Zeichen; alles andere wird zu '?'. Plugin- und Block-IDs sind vom Parser
// bereits so eingeschränkt; das hier schützt den Log auch bei von Hand gebauten Seiten (Tests, Vorschau).
inline std::string safe_id(std::string_view raw, std::size_t limit = 48) {
    std::string out;
    for (char c : raw.substr(0, limit))
        out += (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '#' ? c : '?';
    return out.empty() ? "?" : out;
}
inline std::string limiter_key(std::string_view plugin, std::string_view block, Stage stage) {
    return safe_id(plugin) + "|" + safe_id(block) + (stage == Stage::data ? "|data" : "|draw");
}
inline std::string failure_message(std::string_view plugin, std::string_view block, Stage stage) {
    return "Plugin " + safe_id(plugin) + ": Block " + safe_id(block) +
           (stage == Stage::data ? " ausgeblendet (Fehler beim Auswerten der Daten)" : " ausgeblendet (Fehler beim Zeichnen)");
}
} // namespace fl::plugins::isolation
