// © 2026 FuchsLabs – Zonen-Layout für Plugin-Seiten (Manifest schema_version 2), Phase 2a.
// Reine Daten und Geometrie: keine Grafik, keine Dateien, kein Netz. Die Strukturen hier sind header-only,
// damit plugins.cpp (Parser) ohne plugin_zones.cpp gebaut werden kann; nur plan()/DensityTable/describe()
// stehen in plugin_zones.cpp.
//
// Begriffe (eigene ScreenBuddy-Begriffe, siehe docs/PLUGIN-SYSTEM.md):
//   Zone       – rechteckiger Bereich eines flachen Rasters (Spalte, Zeile, Spannweite). Keine Verschachtelung.
//   Block      – ein Baustein (Plugin-Widget) in einer Zone; Blöcke einer Zone werden untereinander gestapelt.
//   Dichtestufe – kompakt, normal oder ausführlich; gewählt aus der Fläche und den Schwellen des Bausteintyps.
#pragma once
#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace fl { class Canvas; } // nur für Block::draw; dieses Modul zeichnet nie selbst (Blöcke: plugin_render.cpp).

namespace fl::plugins::zones {
// Grenzen des Manifests v2 (vom Parser in plugins.cpp erzwungen, hier für Doku und Tests zentral).
inline constexpr int max_columns = 6;         // Spalten des Rasters
inline constexpr int max_rows = 6;            // Zeilen des Rasters
inline constexpr int max_weight = 12;         // Anteil einer Spalte/Zeile: ganze Zahl 1..12
inline constexpr int max_zones = 8;           // Zonen pro Seite
inline constexpr int max_blocks_per_zone = 4; // Blöcke pro Zone
inline constexpr int max_blocks = 12;         // Blöcke pro v2-Seite mit layout; ohne layout (v1 und v2) höchstens 6
inline constexpr float max_area = 16384.f;    // größere/ungültige Flächen ergeben einen leeren Plan

struct Zone {
    std::string id;
    int column = 0, row = 0;          // 0-basiert
    int column_span = 1, row_span = 1;
};
// Raster in Bruchteilen: Spalte i erhält columns[i] / Summe(columns) der freien Breite (Zeilen analog).
struct Layout {
    std::vector<int> columns, rows;
    std::vector<Zone> zones;
};

struct Rect { float x = 0, y = 0, width = 0, height = 0; };
struct Size { float width = 0, height = 0; };

enum class Density { compact = 0, regular = 1, detailed = 2 };
inline constexpr std::size_t density_count = 3;
// Mindestfläche je Dichtestufe (logische Pixel), Index = Density. Gültig nur aufsteigend (siehe valid()).
struct Thresholds {
    std::array<Size, density_count> min{};
};
bool valid(const Thresholds& thresholds);
// Höchste Dichtestufe, deren Mindestfläche in `area` passt; nullopt = Block passt nicht (ausblenden).
std::optional<Density> choose(const Thresholds& thresholds, Size area);

// Zentrale, app-seitige Schwellen je Bausteintyp (text, metric, bar, list, clock). Nicht im Manifest.
class DensityTable {
public:
    DensityTable(); // Standardwerte
    static const std::vector<std::string>& types();
    const Thresholds* find(std::string_view type) const; // nullptr = unbekannter Typ
    // false bei unbekanntem Typ oder ungültigen Schwellen; der alte Wert bleibt dann erhalten.
    bool set(std::string_view type, const Thresholds& thresholds);
    void reset(); // zurück auf Standardwerte
private:
    std::vector<std::pair<std::string, Thresholds>> entries_;
};

// Block-Schnittstelle für Plugin-Widgets (und später eingebaute Bausteine).
class Block {
public:
    virtual ~Block() = default;
    virtual const std::string& id() const = 0; // stabile Block-ID (Manifest v2: widgets[].id)
    // Kleinste Fläche, in der der Block bei dieser Dichtestufe vollständig zeichnen kann.
    virtual Size min_size(Density density) const = 0;
    // Zeichnet ausschließlich innerhalb von `area`; der Aufrufer setzt den Clip auf `area`.
    // Wird nie mit einer Fläche kleiner als min_size(density) aufgerufen. Umsetzung: plugin_render.cpp (Phase 2b).
    virtual void draw(Canvas& canvas, const Rect& area, Density density) const = 0;
};
// Block, dessen Mindestgrößen aus der zentralen DensityTable kommen (Plugin-Widgets nach Typ).
class TableBlock : public Block {
public:
    TableBlock(std::string id, std::string type, const DensityTable& table)
        : id_(std::move(id)), type_(std::move(type)), table_(&table) {}
    // Der Block hält nur die Adresse der Tabelle; eine temporäre Tabelle wäre sofort ein hängender Zeiger.
    // Die Tabelle muss den Block überleben (z. B. eine statische Tabelle der App).
    TableBlock(std::string, std::string, const DensityTable&&) = delete;
    const std::string& id() const override { return id_; }
    const std::string& type() const { return type_; }
    Size min_size(Density density) const override;
private:
    std::string id_, type_;
    const DensityTable* table_;
};

// Eingabe der Geometrie: reine Daten.
struct BlockInput {
    std::string id, zone;
    Thresholds min;
};
// Liest min_size() für alle drei Stufen; ungültige Angaben ergeben einen Block, der nie passt.
BlockInput describe(const Block& block, std::string zone);
// Dasselbe ohne Block-Objekt, direkt aus der Tabelle (z. B. für Tap-Ziele oder Tests ohne Zeichencode).
BlockInput describe(std::string id, std::string_view type, const DensityTable& table, std::string zone);

struct PlacedZone {
    std::string id;
    Rect area;
};
struct PlacedBlock {
    std::string id, zone;
    Rect area;                      // innerhalb der Zone; bei ausgeblendeten Blöcken leer
    std::optional<Density> density; // nullopt = ausgeblendet (zu wenig Fläche, ungültige Zone/Fläche)
    bool visible() const { return density.has_value(); }
};
struct Plan {
    std::vector<PlacedZone> zones;   // Reihenfolge wie layout.zones
    std::vector<PlacedBlock> blocks; // Reihenfolge wie blocks
};
// Reine Funktion: Fläche + Layout → Rechtecke. `gap` = Abstand zwischen Spalten, Zeilen und gestapelten
// Blöcken. Kanten werden aus kumulierten Anteilen gerundet, Nachbarn teilen sich exakt die Kante (+gap).
// Passt ein Block nicht, wird er ausgeblendet statt zu überlappen (zuerst der letzte nicht passende).
// Ungültige Eingaben (nicht endlich, <= 0, > max_area, Layout außerhalb der Grenzen) → alle Blöcke aus.
Plan plan(const Rect& area, const Layout& layout, const std::vector<BlockInput>& blocks, float gap);
// Prüft dieselben Strukturregeln wie der Parser (Grenzen, Raster, keine Überlappung, eindeutige IDs).
bool valid(const Layout& layout);
} // namespace fl::plugins::zones
