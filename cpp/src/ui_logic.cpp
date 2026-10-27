// © 2026 FuchsLabs
#include "ui_logic.h"

#include <algorithm>
#include <cctype>
#include <set>

#include "qrcodegen.hpp"
#include "render.h"
#include "util.h"

namespace fl::ui::logic {

std::optional<std::string> normalize_hex(const std::string& input) {
    std::string s = trim(input);
    if (!s.empty() && s[0] == '#') s.erase(0, 1);
    if (s.size() != 3 && s.size() != 6) return std::nullopt;
    for (char c : s)
        if (!std::isxdigit(static_cast<unsigned char>(c))) return std::nullopt;
    std::string out = "#";
    for (char c : s) {
        char up = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        out += up;
        if (s.size() == 3) out += up;
    }
    return out;
}

std::vector<std::string> lines_of(const std::string& text) {
    std::vector<std::string> out;
    std::string line;
    auto flush = [&] {
        std::string t = trim(line);
        if (!t.empty()) out.push_back(t);
        line.clear();
    };
    for (char c : text) {
        if (c == '\r' || c == '\n') flush();
        else line += c;
    }
    flush();
    return out;
}

std::string lines_join(const std::vector<std::string>& lines) {
    std::string out;
    for (const auto& l : lines) {
        if (!out.empty()) out += "\r\n";
        out += l;
    }
    return out;
}

std::vector<std::string> id_list(const std::string& text) {
    std::vector<std::string> out;
    std::string cur;
    auto flush = [&] {
        if (!cur.empty() && std::find(out.begin(), out.end(), cur) == out.end()) out.push_back(cur);
        cur.clear();
    };
    for (char c : text) {
        if (c >= '0' && c <= '9') cur += c;
        else flush();
    }
    flush();
    return out;
}

std::string set_id(const std::string& text, const std::string& id, bool on) {
    auto ids = id_list(text);
    auto it = std::find(ids.begin(), ids.end(), id);
    if (on && it == ids.end()) ids.push_back(id);
    if (!on && it != ids.end()) ids.erase(it);
    return lines_join(ids);
}

std::vector<std::string> link_accounts(std::vector<std::string> links, const Account& a, const Account& b) {
    const std::string av = a.platform + ":" + a.id, bv = b.platform + ":" + b.id;
    auto split = [](const std::string& line) {
        std::vector<std::string> parts;
        std::string cur;
        for (char c : line) {
            if (c == '|') { parts.push_back(cur); cur.clear(); }
            else cur += c;
        }
        parts.push_back(cur);
        return parts;
    };
    auto same = [](const std::string& x, const std::string& y) { return casefold(x) == casefold(y); };
    for (auto& line : links) {
        auto parts = split(line);
        bool hit = false;
        for (size_t i = 1; i < parts.size(); ++i)
            if (same(parts[i], av) || same(parts[i], bv)) hit = true;
        if (!hit) continue;
        for (const auto& v : {av, bv})
            if (std::none_of(parts.begin() + 1, parts.end(), [&](const std::string& p) { return same(p, v); })) parts.push_back(v);
        std::string joined;
        for (size_t i = 0; i < parts.size(); ++i) joined += (i ? "|" : "") + parts[i];
        line = joined;
        return links;
    }
    links.push_back(a.name + "|" + av + "|" + bv);
    return links;
}

std::string entity_id_of(const std::string& line) {
    std::string id = line.substr(0, line.find('|'));
    id = trim(id);
    std::transform(id.begin(), id.end(), id.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return id;
}

static bool valid_entity(const std::string& id) {
    size_t dot = id.find('.');
    if (dot == std::string::npos || dot == 0 || dot + 1 >= id.size() || id.find('.', dot + 1) != std::string::npos) return false;
    for (size_t i = 0; i < id.size(); ++i) {
        char c = id[i];
        if (i == dot) continue;
        if (!((c >= 'a' && c <= 'z') || c == '_' || (i > dot && c >= '0' && c <= '9'))) return false;
    }
    return true;
}

bool has_entity(const std::vector<std::string>& lines, const std::string& id) {
    return std::any_of(lines.begin(), lines.end(), [&](const std::string& l) { return entity_id_of(l) == id; });
}

std::string add_entity(std::vector<std::string>& lines, const std::string& id_in, size_t max) {
    std::string id = entity_id_of(id_in);
    if (!valid_entity(id)) return "invalid";
    if (has_entity(lines, id)) return {};
    if (lines.size() >= max) return "full";
    lines.push_back(id);
    return {};
}

void remove_entity(std::vector<std::string>& lines, const std::string& id) {
    lines.erase(std::remove_if(lines.begin(), lines.end(), [&](const std::string& l) { return entity_id_of(l) == id; }), lines.end());
}

std::vector<std::string> color_warnings(const std::map<std::string, std::string>& colors) {
    auto get = [&](const char* k) -> std::optional<Color> {
        auto it = colors.find(k);
        return it == colors.end() ? std::nullopt : render::parse_color(it->second);
    };
    std::vector<std::string> out;
    auto check = [&](const char* a, const char* b, double need, const char* id) {
        auto ca = get(a), cb = get(b);
        if (ca && cb && render::contrast(*ca, *cb) < need) out.push_back(id);
    };
    check("text", "bg", 4.5, "text_bg");
    check("text", "card", 4.5, "text_card");
    check("muted", "bg", 3, "muted_bg");
    check("accent", "bg", 3, "accent_bg");
    return out;
}

std::vector<size_t> filter_indices(const std::vector<std::string>& names, const std::vector<bool>& selected,
                                   const std::string& query, size_t cap) {
    const std::string q = casefold(trim(query));
    std::vector<size_t> on, off;
    for (size_t i = 0; i < names.size(); ++i) {
        bool picked = i < selected.size() && selected[i];
        if (!q.empty() && casefold(names[i]).find(q) == std::string::npos) continue;
        (picked ? on : off).push_back(i);
    }
    on.insert(on.end(), off.begin(), off.end());
    if (on.size() > cap) on.resize(cap);
    return on;
}

Image qr_image(const std::string& text, int module_px) {
    using qrcodegen::QrCode;
    QrCode qr = QrCode::encodeText(text.c_str(), QrCode::Ecc::MEDIUM);
    const int border = 4, modules = qr.getSize() + 2 * border, px = modules * std::max(1, module_px);
    Image img(px, px, Color{255, 255, 255});
    for (int y = 0; y < px; ++y)
        for (int x = 0; x < px; ++x) {
            int mx = x / module_px - border, my = y / module_px - border;
            if (mx >= 0 && my >= 0 && mx < qr.getSize() && my < qr.getSize() && qr.getModule(mx, my)) img.px[size_t(y) * px + x] = 0xFF000000u;
        }
    return img;
}

}  // namespace fl::ui::logic
