// © 2026 FuchsLabs
#include "plugins.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <set>
#include <stdexcept>

namespace fl::plugins {
namespace {
struct Invalid : std::runtime_error { using std::runtime_error::runtime_error; };
void require(bool ok, const char* message) { if (!ok) throw Invalid(message); }
bool lower(char c) { return c >= 'a' && c <= 'z'; }
bool digit(char c) { return c >= '0' && c <= '9'; }
bool alpha(char c) { return lower(c) || (c >= 'A' && c <= 'Z'); }
std::string lowercase(std::string s) { for (auto& c : s) if (c >= 'A' && c <= 'Z') c += 'a' - 'A'; return s; }
bool identifier(std::string_view s) {
    return !s.empty() && s.size() <= 40 && lower(s[0]) &&
        std::all_of(s.begin(), s.end(), [](char c) { return lower(c) || digit(c) || c == '_'; });
}
bool path_segment(std::string_view s) {
    if (s.empty()) return false;
    if (digit(s[0])) return std::all_of(s.begin(), s.end(), digit);
    return (alpha(s[0]) || s[0] == '_') && std::all_of(s.begin(), s.end(), [](char c) {
        return alpha(c) || digit(c) || c == '_';
    });
}
std::vector<std::string> split(std::string_view s) {
    std::vector<std::string> result;
    for (size_t i = 0;;) {
        auto end = s.find('.', i);
        result.emplace_back(s.substr(i, end == s.npos ? s.size() - i : end - i));
        if (end == s.npos) return result;
        i = end + 1;
    }
}
bool semver(const std::string& s) {
    auto parts = split(s);
    return parts.size() == 3 && std::all_of(parts.begin(), parts.end(), [](const auto& p) {
        return !p.empty() && p.size() <= 4 && std::all_of(p.begin(), p.end(), digit);
    });
}
bool clean(std::string_view s) {
    return std::none_of(s.begin(), s.end(), [](unsigned char c) { return c < 32 || c == 127; });
}
// Bidi controls and zero-width characters (U+200B-200F, U+2028-202E, U+2060, U+2066-2069, U+FEFF) allow
// spoofed or invisible text in the approval UI and on the display.
bool invisible_format(std::string_view s) {
    for (size_t i = 0; i + 2 < s.size(); ++i) {
        const auto a = static_cast<unsigned char>(s[i]), b = static_cast<unsigned char>(s[i + 1]),
                   c = static_cast<unsigned char>(s[i + 2]);
        if (a == 0xE2 && b == 0x80 && ((c >= 0x8B && c <= 0x8F) || (c >= 0xA8 && c <= 0xAE))) return true;
        if (a == 0xE2 && ((b == 0x81 && (c == 0xA0 || (c >= 0xA6 && c <= 0xA9))))) return true;
        if (a == 0xEF && b == 0xBB && c == 0xBF) return true;
    }
    return false;
}
bool display_clean(std::string_view s) { return clean(s) && !invisible_format(s); }
size_t characters(std::string_view s) {
    return static_cast<size_t>(std::count_if(s.begin(), s.end(), [](unsigned char c) { return (c & 0xc0) != 0x80; }));
}
std::string bounded(std::string s) {
    if (s.size() <= display_text_limit) return s;
    size_t n = display_text_limit;
    while (n && (static_cast<unsigned char>(s[n]) & 0xc0) == 0x80) --n;
    s.resize(n);
    return s;
}
void fields(const Json& j, std::initializer_list<const char*> allowed) {
    require(j.is_object(), "Expected an object.");
    for (auto it = j.begin(); it != j.end(); ++it)
        require(std::any_of(allowed.begin(), allowed.end(), [&](const char* x) { return it.key() == x; }),
                "Unknown or misplaced field.");
}
std::string str(const Json& j, const char* key, size_t min = 0, size_t max = manifest_limit) {
    require(j.contains(key) && j[key].is_string(), "Missing or invalid string field.");
    const auto s = j[key].get<std::string>();
    require(characters(s) >= min && characters(s) <= max, "String length outside allowed range.");
    return s;
}
int integer(const Json& j, const char* key, int min, int max) {
    require(j.contains(key) && j[key].is_number_integer(), "Missing or invalid integer field.");
    const auto n = j[key].get<double>();
    require(n >= min && n <= max, "Integer outside allowed range.");
    return static_cast<int>(n);
}
double number(const Json& j, const char* key) {
    require(j.contains(key) && j[key].is_number(), "Missing or invalid numeric field.");
    double n = j[key].get<double>();
    require(std::isfinite(n), "Number must be finite.");
    return n;
}
Json parse_bounded(std::string_view input, size_t limit) {
    require(!input.empty() && input.size() <= limit, "JSON size outside allowed range.");
    // Bound nesting before the recursive JSON parser sees the input.
    size_t depth = 0;
    bool quoted = false, escaped = false;
    for (char c : input) {
        if (quoted) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') quoted = false;
        } else if (c == '"') quoted = true;
        else if (c == '{' || c == '[') require(++depth <= depth_limit, "JSON nesting exceeds 32 levels.");
        else if (c == '}' || c == ']') { require(depth > 0, "Invalid JSON."); --depth; }
    }
    std::vector<std::set<std::string>> keys;
    auto callback = [&](int, Json::parse_event_t event, Json& value) {
        if (event == Json::parse_event_t::object_start) keys.emplace_back();
        else if (event == Json::parse_event_t::key)
            require(!keys.empty() && keys.back().insert(value.get<std::string>()).second, "Duplicate JSON key.");
        else if (event == Json::parse_event_t::object_end) keys.pop_back();
        return true;
    };
    return Json::parse(input.begin(), input.end(), callback, true, false);
}
const Setting* setting(const Manifest& m, const std::string& key) {
    for (const auto& s : m.settings) if (s.key == key) return &s;
    return nullptr;
}
bool source_exists(const Manifest& m, const std::string& id) {
    return std::any_of(m.sources.begin(), m.sources.end(), [&](const Source& s) { return s.id == id; });
}
enum class Usage { display, item, request };
void reference(const Manifest& m, const std::string& ref, Usage usage, std::set<std::string>* used) {
    auto path = split(ref);
    require(std::all_of(path.begin(), path.end(), [](const auto& p) { return path_segment(p); }), "Invalid template path.");
    if (path[0] == "settings") {
        require(path.size() == 2, "Setting reference must have exactly one key.");
        const auto* s = setting(m, path[1]);
        require(s != nullptr, "Unknown setting reference.");
        require(usage == Usage::request || s->type != "secret", "Secret settings cannot appear in display templates.");
        if (used) used->insert(s->key);
        return;
    }
    require(usage != Usage::request, "Requests may only reference settings.");
    if ((path[0] == "time" || path[0] == "date") && path.size() == 1) return;
    if (path[0] == "item" && usage == Usage::item) return;
    require(path.size() >= 2 && source_exists(m, path[0]), "Unknown source or invalid template reference.");
}
template<class F> std::string expand(std::string_view s, F resolve, bool cap) {
    std::string out;
    for (size_t i = 0; i < s.size();) {
        auto begin = s.find("{{", i), stray = s.find("}}", i);
        require(stray == s.npos || (begin != s.npos && stray > begin), "Unmatched template delimiter.");
        if (begin == s.npos) { out.append(s.substr(i)); break; }
        out.append(s.substr(i, begin - i));
        auto end = s.find("}}", begin + 2);
        require(end != s.npos, "Unmatched template delimiter.");
        out += resolve(std::string(s.substr(begin + 2, end - begin - 2)));
        i = end + 2;
        if (cap && out.size() > display_text_limit) return bounded(std::move(out));
        require(out.size() <= manifest_limit, "Expanded request exceeds size limit.");
    }
    require(cap || out.size() <= manifest_limit, "Expanded request exceeds size limit.");
    return cap ? bounded(std::move(out)) : out;
}
void validate_template(const Manifest& m, const std::string& s, Usage usage, std::set<std::string>* used = nullptr) {
    expand(s, [&](const auto& ref) { reference(m, ref, usage, used); return std::string(); }, false);
}
std::string url_host(const std::string& url) {
    require(url.starts_with("https://") && clean(url) && url.find('\\') == url.npos &&
            url.find('#') == url.npos && url.find(' ') == url.npos, "Source requires a fixed HTTPS URL.");
    auto end = url.find_first_of("/?", 8);
    auto authority = url.substr(8, end == url.npos ? url.size() - 8 : end - 8);
    if (authority.ends_with(":443")) authority.resize(authority.size() - 4);
    require(authority.size() <= 253 && authority.find('.') != authority.npos, "Source requires a public DNS hostname.");
    const auto host = lowercase(authority);
    const auto labels = split(host);
    for (const auto& label : labels) {
        require(!label.empty() && label.size() <= 63 && label.front() != '-' && label.back() != '-' &&
            std::all_of(label.begin(), label.end(), [](char c) { return lower(c) || digit(c) || c == '-'; }),
            "Invalid HTTPS hostname or port.");
    }
    require(std::all_of(labels.back().begin(), labels.back().end(), lower) && labels.back().size() >= 2,
            "IP literals and numeric hostnames are not allowed.");
    require(labels.back() != "localhost" && labels.back() != "local" && labels.back() != "internal" &&
            labels.back() != "lan" && labels.back() != "home" && host != "home.arpa" && !host.ends_with(".home.arpa"),
            "Local hostnames are not allowed.");
    return host;
}
bool header_name(const std::string& s) {
    static const std::string punctuation = "!#$%&'*+.^_`|~-";
    return !s.empty() && std::all_of(s.begin(), s.end(), [&](char c) { return alpha(c) || digit(c) || punctuation.find(c) != punctuation.npos; });
}
Json settings_values(const Manifest& m, const Json& supplied, bool secrets) {
    require(supplied.is_object(), "Settings must be an object.");
    Json values = Json::object();
    for (const auto& s : m.settings) {
        if (!secrets && s.type == "secret") continue;
        Json v = supplied.contains(s.key) ? supplied[s.key] : s.default_value;
        require(s.type == "number" ? v.is_number() && std::isfinite(v.get<double>()) : v.is_string(), "Invalid setting value type.");
        require(!v.is_string() || v.get_ref<const std::string&>().size() <= manifest_limit, "Setting value too large.");
        values[s.key] = std::move(v);
    }
    return values;
}
const Json* walk(const Json* value, const std::vector<std::string>& path, size_t i) {
    for (; value && i < path.size(); ++i) {
        if (value->is_object()) {
            auto it = value->find(path[i]);
            value = it == value->end() ? nullptr : &*it;
        } else if (value->is_array()) {
            size_t index = 0;
            auto [end, ec] = std::from_chars(path[i].data(), path[i].data() + path[i].size(), index);
            if (ec != std::errc() || end != path[i].data() + path[i].size() || index >= value->size()) return nullptr;
            value = &(*value)[index];
        } else return nullptr;
    }
    return value;
}
std::string scalar(const Json* value, bool& missing) {
    if (!value || value->is_null()) { missing = true; return "—"; }
    if (value->is_structured()) return "[Daten]";
    if (value->is_string()) return bounded(value->get<std::string>());
    if (value->is_number() && !std::isfinite(value->get<double>())) { missing = true; return "—"; }
    return value->dump();
}
std::string encode_component(std::string_view value) {
    // "." and ".." would become dot segments in a URL path (traversal after server/proxy normalization).
    require(value != "." && value != "..", "Setting value is not allowed as a URL path segment.");
    const char* hex = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : value) {
        if (alpha(static_cast<char>(c)) || digit(static_cast<char>(c)) || c == '-' || c == '.' || c == '_' || c == '~') out += c;
        else { out += '%'; out += hex[c >> 4]; out += hex[c & 15]; }
    }
    return out;
}
std::string checked_text(const Json& j, const char* key, size_t min = 0, size_t max = manifest_limit) {
    auto s = str(j, key, min, max);
    require(display_clean(s), "Control, bidirectional or zero-width characters are not allowed in display text.");
    return s;
}
} // namespace

bool reserved_device_name(std::string_view id) {
    const auto name = lowercase(std::string(id));
    if (name == "con" || name == "prn" || name == "aux" || name == "nul") return true;
    return name.size() == 4 && (name.starts_with("com") || name.starts_with("lpt")) && name[3] >= '1' && name[3] <= '9';
}
std::string source_fingerprint(const Manifest& manifest) {
    Json sources = Json::array();
    for (const auto& s : manifest.sources)
        sources.push_back({{"id", s.id}, {"url", s.url}, {"host", s.host}, {"headers", s.headers}});
    return sources.dump(); // std::map keeps headers in a stable order
}

std::optional<Json> parse_strict_json(std::string_view utf8, size_t limit) {
    try { return parse_bounded(utf8, limit); }
    catch (const Invalid&) { return std::nullopt; }
    catch (const Json::exception&) { return std::nullopt; }
}

ParseResult parse_manifest(std::string_view utf8) {
    try {
        Json j = parse_bounded(utf8, manifest_limit);
        fields(j, {"schema_version", "id", "name", "description", "version", "license", "author", "category", "min_app_version",
                   "refresh_seconds", "settings", "sources", "widgets"});
        require(integer(j, "schema_version", 1, 1) == 1, "Unsupported schema version.");
        Manifest m;
        m.id = str(j, "id", 1, 48);
        require(lower(m.id[0]) && m.id.back() != '-' && m.id.find("--") == m.id.npos &&
                std::all_of(m.id.begin(), m.id.end(), [](char c) { return lower(c) || digit(c) || c == '-'; }) &&
                !reserved_device_name(m.id), "Invalid plugin ID.");
        m.name = checked_text(j, "name", 1, 80);
        m.description = j.contains("description") ? checked_text(j, "description", 0, 500) : "";
        m.version = str(j, "version", 5, 14);
        require(semver(m.version), "Version must be X.Y.Z.");
        if (j.contains("license")) {
            m.license = str(j, "license", 1, 32);
            require(std::any_of(std::begin(store_licenses), std::end(store_licenses), [&](const char* x) { return m.license == x; }),
                    "License must be MIT or CC0-1.0.");
        }
        if (j.contains("author")) m.author = checked_text(j, "author", 1, 80);
        if (j.contains("category")) {
            m.category = str(j, "category", 1, 32);
            require(std::any_of(std::begin(store_categories), std::end(store_categories), [&](const char* x) { return m.category == x; }),
                    "Unknown category.");
        }
        if (j.contains("min_app_version")) {
            m.min_app_version = str(j, "min_app_version", 5, 14);
            require(semver(m.min_app_version), "min_app_version must be X.Y.Z.");
        }
        if (j.contains("refresh_seconds")) m.refresh_seconds = integer(j, "refresh_seconds", 15, 3600);
        for (const auto* key : {"settings", "sources"}) if (!j.contains(key)) j[key] = Json::array();
        require(j["settings"].is_array() && j["settings"].size() <= 12, "At most 12 settings allowed.");
        std::set<std::string> setting_ids, source_ids;
        for (const auto& v : j["settings"]) {
            fields(v, {"key", "label", "type", "default", "required"});
            Setting s;
            s.key = str(v, "key");
            require(identifier(s.key) && setting_ids.insert(s.key).second, "Invalid or duplicate setting key.");
            s.label = checked_text(v, "label", 1, 80);
            s.type = str(v, "type");
            require(s.type == "text" || s.type == "number" || s.type == "secret", "Unsupported setting type.");
            if (v.contains("required")) { require(v["required"].is_boolean(), "Required must be boolean."); s.required = v["required"].get<bool>(); }
            s.default_value = v.contains("default") ? v["default"] : (s.type == "number" ? Json(0) : Json(""));
            require(s.type == "number" ? s.default_value.is_number() && std::isfinite(s.default_value.get<double>()) : s.default_value.is_string(), "Invalid default value type.");
            if (s.type != "number") {
                const auto& default_text = s.default_value.get_ref<const std::string&>();
                require(default_text.size() <= setting_value_limit && clean(default_text), "Invalid setting default.");
            }
            require(s.type != "secret" || s.default_value == "", "Secret defaults must be empty.");
            m.settings.push_back(std::move(s));
        }
        require(j["sources"].is_array() && j["sources"].size() <= 4, "At most 4 sources allowed.");
        for (const auto& v : j["sources"]) {
            fields(v, {"id", "url", "format", "headers", "sample"});
            Source s;
            s.id = str(v, "id");
            require(identifier(s.id) && source_ids.insert(s.id).second && s.id != "settings" && s.id != "time" &&
                    s.id != "date" && s.id != "item", "Invalid, reserved or duplicate source ID.");
            s.url = str(v, "url");
            s.host = url_host(s.url);
            std::set<std::string> used, headers;
            validate_template(m, s.url, Usage::request, &used);
            if (v.contains("format")) require(str(v, "format") == "json", "Only JSON sources are supported.");
            if (v.contains("sample")) { require(v["sample"].is_object(), "Sample must be an object."); s.sample = v["sample"]; }
            if (v.contains("headers")) {
                require(v["headers"].is_object() && v["headers"].size() <= 16, "Headers must be an object with at most 16 entries.");
                for (auto it = v["headers"].begin(); it != v["headers"].end(); ++it) {
                    auto name = lowercase(it.key());
                    require(header_name(name) && (name == "accept" || name == "authorization" || name.starts_with("x-")) &&
                            headers.insert(name).second, "Unsupported or duplicate header.");
                    require(it.value().is_string(), "Header value must be a string.");
                    auto value = it.value().get<std::string>();
                    require(clean(value), "Control characters are not allowed in headers.");
                    validate_template(m, value, Usage::request, &used);
                    s.headers.emplace(it.key(), std::move(value));
                }
            }
            s.setting_keys.assign(used.begin(), used.end());
            m.sources.push_back(std::move(s));
        }
        require(j.contains("widgets") && j["widgets"].is_array() && !j["widgets"].empty() && j["widgets"].size() <= 6,
                "A plugin needs 1 to 6 widgets.");
        for (const auto& v : j["widgets"]) {
            Widget w;
            w.type = str(v, "type");
            if (w.type == "text") { fields(v, {"type", "label", "text"}); w.text = checked_text(v, "text"); }
            else if (w.type == "metric") { fields(v, {"type", "label", "value", "unit"}); w.value = checked_text(v, "value"); }
            else if (w.type == "bar") {
                fields(v, {"type", "label", "value", "unit", "min", "max"});
                w.value = checked_text(v, "value"); w.min = number(v, "min"); w.max = number(v, "max");
                require(w.max > w.min, "Bar maximum must exceed minimum.");
            } else if (w.type == "list") {
                fields(v, {"type", "label", "items", "item_template", "max_items"});
                w.items = str(v, "items"); w.item_template = checked_text(v, "item_template");
                w.max_items = integer(v, "max_items", 1, 5);
                auto path = split(w.items);
                require(path.size() >= 2 && source_exists(m, path[0]), "List path must start with a source.");
                reference(m, w.items, Usage::display, nullptr);
                validate_template(m, w.item_template, Usage::item);
            } else if (w.type == "clock") {
                fields(v, {"type", "label", "format"}); w.format = str(v, "format");
                require(w.format == "time" || w.format == "date" || w.format == "datetime", "Unsupported clock format.");
            } else throw Invalid("Unsupported widget type.");
            if (v.contains("label")) w.label = checked_text(v, "label");
            if (v.contains("unit")) w.unit = checked_text(v, "unit");
            for (const auto* text : {&w.label, &w.unit, &w.text, &w.value}) validate_template(m, *text, Usage::display);
            m.widgets.push_back(std::move(w));
        }
        j["description"] = m.description; j["refresh_seconds"] = m.refresh_seconds;
        m.definition = std::move(j);
        return {std::move(m), {}};
    } catch (const Invalid& e) { return {std::nullopt, e.what()}; }
      catch (const Json::exception&) { return {std::nullopt, "Invalid JSON or JSON value."}; }
}

DataResult parse_source_data(std::string_view utf8) {
    try {
        auto j = parse_bounded(utf8, response_limit);
        require(j.is_object(), "Source response must be an object.");
        return {std::move(j), {}};
    } catch (const Invalid& e) { return {std::nullopt, e.what()}; }
      catch (const Json::exception&) { return {std::nullopt, "Invalid source JSON."}; }
}
SourceData sample_data(const Manifest& manifest) {
    SourceData result;
    for (const auto& source : manifest.sources) result.emplace(source.id, source.sample);
    return result;
}
Page render_page(const Manifest& m, const RenderContext& context) {
    Page page{m.id, bounded(m.name), {}, false};
    Json values;
    try { values = settings_values(m, context.settings, false); }
    catch (const Invalid&) { values = Json::object(); page.missing_data = true; }
    auto data = [&](const std::string& ref, const Json* item) -> const Json* {
        auto path = split(ref);
        if (path[0] == "settings") return walk(&values, path, 1);
        if (path[0] == "item") return walk(item, path, 1);
        auto it = context.sources.find(path[0]);
        return it == context.sources.end() ? nullptr : walk(&it->second, path, 1);
    };
    auto text = [&](const std::string& source, const Json* item = nullptr) {
        return expand(source, [&](const std::string& ref) {
            if (ref == "time") return bounded(context.time);
            if (ref == "date") return bounded(context.date);
            return scalar(data(ref, item), page.missing_data);
        }, true);
    };
    for (const auto& w : m.widgets) {
        RenderedWidget rendered;
        rendered.type = w.type; rendered.label = text(w.label); rendered.unit = text(w.unit);
        if (w.type == "text") rendered.text = text(w.text);
        else if (w.type == "clock") rendered.text = bounded(w.format == "time" ? context.time :
            w.format == "date" ? context.date : context.date + " " + context.time);
        else if (w.type == "list") {
            const Json* array = data(w.items, nullptr);
            if (!array || !array->is_array()) { rendered.items.push_back("—"); page.missing_data = true; }
            else for (size_t i = 0; i < array->size() && i < static_cast<size_t>(w.max_items); ++i)
                rendered.items.push_back(text(w.item_template, &(*array)[i]));
        } else {
            rendered.text = text(w.value);
            if (w.type == "bar") {
                double n = 0;
                const auto& value = rendered.text;
                auto [end, ec] = std::from_chars(value.data(), value.data() + value.size(), n);
                if (ec == std::errc() && end == value.data() + value.size() && std::isfinite(n)) {
                    // Scale before subtraction, also on MSVC where long double == double.
                    double scale = std::max({std::abs(w.min), std::abs(w.max), 1.0});
                    rendered.bar_fraction = std::clamp((std::clamp(n, w.min, w.max) / scale - w.min / scale) /
                        (w.max / scale - w.min / scale), 0.0, 1.0);
                } else page.missing_data = true;
            }
        }
        page.widgets.push_back(std::move(rendered));
    }
    return page;
}
RequestResult prepare_request(const Manifest& m, const Source& source, const Json& supplied) {
    try {
        const auto values = settings_values(m, supplied, true);
        for (const auto& key : source.setting_keys) {
            const auto* s = setting(m, key);
            require(s && (!s->required || !values[key].is_string() || !values[key].get_ref<const std::string&>().empty()),
                    "A required source setting is missing.");
        }
        auto resolve = [&](const std::string& ref, bool encoded) {
            reference(m, ref, Usage::request, nullptr);
            auto path = split(ref);
            const auto& value = values.at(path[1]);
            const auto raw = value.is_string() ? value.get<std::string>() : value.dump();
            return encoded ? encode_component(raw) : raw;
        };
        Request request;
        request.host = source.host;
        request.url = expand(source.url, [&](const auto& ref) { return resolve(ref, true); }, false);
        require(request.url.size() <= manifest_limit && url_host(request.url) == source.host, "Expanded URL is invalid.");
        for (const auto& [name, value] : source.headers) {
            auto expanded = expand(value, [&](const auto& ref) { return resolve(ref, false); }, false);
            require(clean(expanded), "Control characters are not allowed in expanded headers.");
            request.headers.emplace(name, std::move(expanded));
        }
        return {std::move(request), {}};
    } catch (const Invalid& e) { return {std::nullopt, e.what()}; }
      catch (const Json::exception&) { return {std::nullopt, "Invalid source settings."}; }
}
bool app_compatible(const Manifest& manifest, std::string_view app_version) {
    if (manifest.min_app_version.empty()) return true;
    const std::string app(app_version);
    if (!semver(app)) return true;
    auto number = [](const std::string& v) {
        std::vector<int> n;
        for (const auto& p : split(v)) n.push_back(std::stoi(p));
        return n;
    };
    return number(app) >= number(manifest.min_app_version);
}

std::vector<std::string> store_problems(const Manifest& m) {
    std::vector<std::string> p;
    if (characters(m.description) < 10) p.push_back("Store: description (at least 10 characters) is required.");
    if (m.author.empty()) p.push_back("Store: author is required.");
    if (m.license.empty()) p.push_back("Store: license (MIT or CC0-1.0) is required.");
    if (m.category.empty()) p.push_back("Store: category is required.");
    if (m.min_app_version.empty()) p.push_back("Store: min_app_version is required.");
    if (!m.sources.empty() && m.refresh_seconds < 60) p.push_back("Store: plugins with sources need refresh_seconds of at least 60.");
    for (const auto& s : m.sources)
        if (s.sample.empty()) { p.push_back("Store: every source needs a sample for the preview."); break; }
    return p;
}

} // namespace fl::plugins
