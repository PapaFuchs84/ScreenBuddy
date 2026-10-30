// © 2026 FuchsLabs
// Verbindung: Companion-App (Android) – Grundwerte, Kopplung per QR-Code, Geräte, Status und eigene Seitenauswahl.
#include "ui_pages.h"

#include <algorithm>
#include <chrono>
#include <ctime>

#include "app.h"
#include "native_settings.h"
#include "output.h"
#include "ui_logic.h"
#include "util.h"

namespace fl::ui {
namespace {

using Clock = std::chrono::steady_clock;

std::wstring when(long long ts, bool english) {
    if (ts <= 0) return L"–";
    std::time_t t = static_cast<std::time_t>(ts);
    std::tm tm{};
    localtime_s(&tm, &t);
    wchar_t buf[40];
    wcsftime(buf, 40, english ? L"%Y-%m-%d %H:%M" : L"%d.%m.%Y %H:%M", &tm);
    return buf;
}

class CompanionPage : public Page {
public:
    PageInfo info() const override {
        return {"companion", Group::Connection, Text(L"Companion"),
                T(L"Android-App im Heimnetz: Ausgabe, Kopplung, Geräte und eigene Seitenauswahl",
                  L"Android app on your home network: output, pairing, devices and its own page selection")};
    }
    bool active(const ojson& cfg) const override {
        return flag(cfg, "companion", "enabled") || cfg.at("display").value("output", "") == "network";
    }
    void build(Host& host, Column& root) override {
        form_ = Form(host);
        host_ = &host;
        const ojson& companion = host.config().at("companion");
        Card* base = root.add<Card>(T(L"Companion-Ausgabe", L"Companion output"),
            T(L"Zusätzlich zum USB-Display bzw. Bildschirm. Änderungen gelten nach Speichern und Neustart.",
              L"In addition to the USB display or monitor. Changes apply after saving and restarting."));
        form_.toggle(*base, "companion", "enabled", T(L"Companion zusätzlich aktivieren", L"Enable additional Companion output"));
        form_.number(*base, "companion", "port", T(L"TLS-Port", L"TLS port"),
                     T(L"1024–65535; nie der Port der Einstellungen (8765)", L"1024–65535; never the settings port (8765)"), 1024, 65535);
        form_.toggle(*base, "companion", "sync", T(L"Geräte synchron anzeigen", L"Show the same on all devices"),
                     T(L"An: alle Handys zeigen dasselbe Bild und wechseln gemeinsam. Aus: jedes Gerät hat eigene Seite, Auflösung, Wischen/Tippen und optional eine eigene Seitenliste (unten). Wirkt nach Speichern und Neustart.",
                       L"On: all phones show the same picture and switch together. Off: every device has its own page, resolution, swipe/tap and optionally its own page list (below). Applies after saving and restarting."));
        form_.slider(*base, "companion", "page_seconds", T(L"Seitenwechsel", L"Page interval"), T(L"3–600 Sekunden", L"3–600 seconds"),
                     3, 600, Text(L"s"));

        Card* status = root.add<Card>(T(L"Status", L"Status"), T(L"Wird jede Sekunde aktualisiert.", L"Refreshed every second."));
        auto label = [&](Card* c, Text l) { return c->row<Label>(std::move(l), Text(), Text(L"…"), Font::Small, Label::Muted); };
        server_ = label(status, T(L"Server", L"Server"));
        mdns_ = label(status, T(L"Ankündigung im Netz (mDNS)", L"Network announcement (mDNS)"));
        clients_ = label(status, T(L"Verbunden", L"Connected"));
        print_ = label(status, T(L"Fingerabdruck des Zertifikats", L"Certificate fingerprint"));
        print_->width = 360;

        Card* pair = root.add<Card>(T(L"Gerät koppeln", L"Pair a device"),
            T(L"Code und QR-Code sind 2 Minuten gültig. In der App den QR-Code scannen oder Adresse und PIN eintippen und den Fingerabdruck vergleichen.",
              L"The code and QR code are valid for 2 minutes. Scan the QR code in the app or type address and PIN and compare the fingerprint."));
        auto buttons = pair->add<ButtonRow>();
        pair_ = buttons->add(T(L"Kopplungscode anzeigen", L"Show pairing code"), Button::Primary, [this] { start_pairing(); });
        cancel_ = buttons->add(T(L"Abbrechen", L"Cancel"), Button::Ghost, [this] { cancel_pairing(); });
        cancel_->visible = pairing_;
        pair_note_ = pair->add<Label>(Text(), Font::Small, Label::Muted);
        pair_note_->visible = false;
        qr_ = pair->add<ImageView>();
        qr_->fixed_w = 220;
        qr_->aspect = 1.f;
        qr_->crisp = true;
        qr_->framed = false;
        qr_->name = T(L"QR-Code zum Koppeln", L"Pairing QR code");
        qr_->image = pairing_qr_;
        qr_->visible = pairing_;
        pin_ = pair->add<Label>(pairing_pin_, Font::H1, Label::Normal);
        pin_->visible = pairing_;
        pin_info_ = pair->add<Label>(Text(widen(host_text_)), Font::Body, Label::Muted);
        pin_info_->visible = pairing_;
        pin_print_ = pair->add<Label>(pairing_print_, Font::Small, Label::Muted);
        pin_print_->visible = pairing_;

        Card* devices = root.add<Card>(T(L"Gekoppelte Geräte", L"Paired devices"));
        devices_ = devices->add<ItemList>(std::vector<ItemList::Item>{});
        devices_->name = T(L"Gekoppelte Geräte", L"Paired devices");
        devices_->checkable = false;
        devices_->reorder = false;
        devices_->track_dirty = false;
        devices_->empty_text = T(L"Noch kein Gerät gekoppelt.", L"No device paired yet.");
        remove_ = devices->add<ButtonRow>()->add(T(L"Ausgewähltes Gerät entfernen", L"Remove selected device"), Button::Danger, [this] { remove_device(); });
        signature_.clear();

        Card* own = root.add<Card>(T(L"Seiten für das ausgewählte Gerät", L"Pages for the selected device"),
            T(L"Nur bei ausgeschalteter Synchronisation wirksam. Eigene Auswahl und Reihenfolge für das oben gewählte Gerät; Haken = anzeigen. Wird sofort gespeichert.",
              L"Only effective with synchronisation off. Own selection and order for the device selected above; check = show. Saved immediately."));
        dev_note_ = own->add<Label>(Text(), Font::Small, Label::Muted);
        dev_list_ = own->add<ItemList>(std::vector<ItemList::Item>{});
        dev_list_->name = T(L"Seiten des Geräts", L"Device pages");
        dev_list_->track_dirty = false;
        dev_list_->empty_text = T(L"Oben ein Gerät auswählen.", L"Select a device above first.");
        auto own_buttons = own->add<ButtonRow>();
        dev_save_ = own_buttons->add(T(L"Eigene Liste speichern", L"Save own list"), Button::Primary, [this] { save_device_pages(true); });
        dev_reset_ = own_buttons->add(T(L"Gemeinsame Liste verwenden", L"Use shared list"), Button::Secondary, [this] { save_device_pages(false); });
        devices_->on_select = [this](int) { dev_items_for_.clear(); load_device_pages(); };

        Card* pages = root.add<Card>(T(L"Seiten für Companion", L"Pages for Companion"),
            T(L"Eigene Auswahl und Reihenfolge. Haken = anzeigen; ▲/▼ oder Alt+Pfeiltaste sortiert um.",
              L"Own selection and order. Check = show; ▲/▼ or Alt+arrow key reorders."));
        list_ = pages->add<ItemList>(page_items(companion.at("pages"), companion.at("pages")));
        list_->name = T(L"Companion-Seiten", L"Companion pages");
        list_->selected = 0;
        auto step = pages->add<ButtonRow>();
        prev_ = step->add(T(L"Vorige Seite zeigen", L"Show previous page"), Button::Secondary, [this] { step_page(-1); });
        next_ = step->add(T(L"Nächste Seite zeigen", L"Show next page"), Button::Secondary, [this] { step_page(1); });
        tick(host, host.status());
    }
    void collect(Host&, ojson& patch) override {
        form_.collect(patch);
        auto& pages = patch["companion"]["pages"] = ojson::array();
        for (const auto& item : list_->items) if (item.enabled) pages.push_back(item.key);
    }
    void tick(Host&, const Status& s) override {
        if (!server_) return;
        const bool en = host_->english();
        const ojson& c = s.companion;
        const bool on = c.is_object() && c.value("enabled", false);
        const bool running = on && c.value("running", false);
        auto set = [](Label* l, Text t, Label::Tone tone = Label::Muted) { l->text = std::move(t); l->tone = tone; };
        if (!on) {
            set(server_, T(L"Companion ist aus oder startet nach „Speichern & anwenden“.", L"Companion is off or starts after “Save & apply”."));
            set(mdns_, Text(L"–"));
            set(clients_, Text(L"–"));
            set(print_, Text(L"–"));
        } else {
            std::string addr;
            if (c.contains("addresses") && c["addresses"].is_array())
                for (const auto& a : c["addresses"]) addr += (addr.empty() ? "" : ", ") + a.get<std::string>() + ":" + std::to_string(c.value("port", 0));
            if (running) set(server_, Text(L"Läuft auf " + widen(addr), L"Running on " + widen(addr)), Label::Success);
            else {
                std::string err = c.value("error", "");
                set(server_, err.empty() ? T(L"Startet …", L"Starting …") : Text(widen(err)), err.empty() ? Label::Muted : Label::Danger);
            }
            set(mdns_, c.value("mdns", false) ? T(L"Die App findet den PC von selbst.", L"The app finds the PC by itself.")
                                              : T(L"Nicht verfügbar – in der App die IP-Adresse eingeben.", L"Not available – type the IP address in the app."));
            std::wstring names;
            if (c.contains("clients") && c["clients"].is_array())
                for (const auto& x : c["clients"]) names += (names.empty() ? L"" : L", ") + widen(x.value("name", "") + " (" + x.value("ip", "") + ")");
            set(clients_, names.empty() ? T(L"Gerade ist kein Gerät verbunden.", L"No device is connected right now.") : Text(names),
                names.empty() ? Label::Muted : Label::Success);
            set(print_, Text(widen(c.value("fingerprint", "–"))));
            if (c.value("locked_for", 0) > 0)
                set(server_, Text(L"Kopplung gesperrt (zu viele Fehlversuche): " + std::to_wstring((c.value("locked_for", 0) + 59) / 60) + L" min",
                                  L"Pairing locked (too many failed attempts): " + std::to_wstring((c.value("locked_for", 0) + 59) / 60) + L" min"), Label::Danger);
        }
        refresh_devices(c, en);
        // Kopplungscode: Restzeit; ist er verbraucht oder abgelaufen, ausblenden
        if (pairing_) {
            auto left = std::chrono::duration_cast<std::chrono::seconds>(deadline_ - Clock::now()).count();
            const bool server_active = on && c.contains("pairing") && c["pairing"].value("active", false);
            const bool grace = Clock::now() - started_ < std::chrono::seconds(4);
            if (left <= 0 || (!server_active && !grace)) end_pairing(left <= 0 ? T(L"Code abgelaufen. Bei Bedarf einen neuen anzeigen.", L"Code expired. Show a new one if needed.")
                                                                        : T(L"Code verwendet oder abgebrochen.", L"Code used or cancelled."));
            else {
                wchar_t t[16];
                swprintf(t, 16, L"%lld:%02lld", left / 60, left % 60);
                pin_info_->text = Text(widen(host_text_) + L" · noch " + t, widen(host_text_) + L" · " + t + L" left");
            }
        }
        const bool idle = !host_->busy();
        pair_->enabled = running && idle && !pairing_;
        remove_->enabled = on && idle && devices_ && !devices_->items.empty() && devices_->selected >= 0;
        prev_->enabled = next_->enabled = running && idle;
        const bool dev_sel = devices_ && devices_->selected >= 0 && devices_->selected < int(devices_->items.size());
        dev_save_->enabled = running && idle && dev_sel && !dev_list_->items.empty();
        dev_reset_->enabled = running && idle && dev_sel;
        devices_json_ = c.is_object() && c.contains("devices") ? c["devices"] : ojson::array();
        if (dev_sel && dev_loaded_for_ != devices_->items[size_t(devices_->selected)].key) load_device_pages();
    }

private:
    Form form_;
    Host* host_ = nullptr;
    Label *server_ = nullptr, *mdns_ = nullptr, *clients_ = nullptr, *print_ = nullptr;
    Label *pair_note_ = nullptr, *pin_ = nullptr, *pin_info_ = nullptr, *pin_print_ = nullptr;
    ImageView* qr_ = nullptr;
    Button *pair_ = nullptr, *cancel_ = nullptr, *remove_ = nullptr, *prev_ = nullptr, *next_ = nullptr;
    ItemList *devices_ = nullptr, *list_ = nullptr, *dev_list_ = nullptr;
    Label* dev_note_ = nullptr;
    Button *dev_save_ = nullptr, *dev_reset_ = nullptr;
    ojson devices_json_ = ojson::array();
    std::string dev_loaded_for_, dev_items_for_;   // angefordert / tatsächlich in der Liste
    bool pairing_ = false;
    // Die Kopplung ist eine sofortige Serveraktion und überlebt „Verwerfen“ der Formularwerte.
    std::optional<Image> pairing_qr_;
    Text pairing_pin_, pairing_print_;
    Clock::time_point deadline_, started_;
    std::string host_text_, signature_;

    void show_pairing(bool shown) {
        qr_->visible = pin_->visible = pin_info_->visible = pin_print_->visible = shown;
        cancel_->visible = shown;
        host_->relayout();
    }
    void end_pairing(Text note) {
        pairing_ = false;
        show_pairing(false);
        pair_note_->text = std::move(note);
        pair_note_->visible = true;
        qr_->image.reset();
        pairing_qr_.reset();
        pairing_pin_ = pairing_print_ = Text();
        host_->relayout();
    }
    void start_pairing() {
        if (pairing_) return;
        Host* h = host_;
        h->run([this, h]() -> std::function<void()> {
            auto* c = h->app().companion();
            if (!c) throw std::runtime_error(h->english() ? "The Companion server is not running – enable Companion and save."
                                                          : "Der Companion-Server läuft nicht – Companion einschalten und speichern.");
            ojson r = c->start_pairing();
            if (!r.value("ok", false)) throw std::runtime_error(r.value("error", "?"));
            Image qr = logic::qr_image(r.value("uri", ""), 6);
            const std::string pin = r.value("pin", ""), host = r.value("host", "") + ":" + std::to_string(r.value("port", 0));
            const std::string print = r.value("fingerprint", "");
            const int seconds = r.value("expires_in", 120);
            return [this, qr = std::move(qr), pin, host, print, seconds]() mutable {
                pairing_qr_ = std::move(qr);
                qr_->image = pairing_qr_;
                ++qr_->version;
                pairing_pin_ = Text(widen(pin.size() == 6 ? pin.substr(0, 3) + " " + pin.substr(3) : pin));
                pin_->text = pairing_pin_;
                host_text_ = host;
                pairing_print_ = Text(L"SHA-256: " + widen(print));
                pin_print_->text = pairing_print_;
                pair_note_->visible = false;
                pairing_ = true;
                started_ = Clock::now();
                deadline_ = started_ + std::chrono::seconds(seconds);
                pin_info_->text = Text(widen(host));
                show_pairing(true);
            };
        });
    }
    void cancel_pairing() {
        Host* h = host_;
        h->run([h]() -> std::function<void()> {
            if (auto* c = h->app().companion()) c->cancel_pairing();
            return {};
        });
        end_pairing(T(L"Kopplung abgebrochen.", L"Pairing cancelled."));
    }
    void remove_device() {
        const int i = devices_->selected;
        if (i < 0 || i >= int(devices_->items.size())) return;
        const auto& item = devices_->items[size_t(i)];
        const std::string id = item.key;
        const bool en = host_->english();
        const std::wstring text = (en ? L"Remove pairing of “" : L"Kopplung von „") + item.title.get(en) +
                                  (en ? L"”? The device must be paired again afterwards." : L"“ entfernen? Das Gerät muss danach neu gekoppelt werden.");
        if (MessageBoxW(host_->window(), text.c_str(), en ? L"ScreenBuddy" : L"ScreenBuddy", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return;
        Host* h = host_;
        h->run([h, id]() -> std::function<void()> {
            auto* c = h->app().companion();
            const bool ok = c && c->remove_device(id);
            return [h, ok] {
                if (ok) h->toast(T(L"Gerät entfernt.", L"Device removed."), ToastKind::Success);
                else h->toast(T(L"Gerät nicht gefunden.", L"Device not found."), ToastKind::Error);
            };
        });
    }
    // Eigene Seitenliste des gewählten Geräts anzeigen (Plugins kommen aus der Pluginliste, Arbeits-Thread)
    void load_device_pages() {
        if (!devices_ || devices_->selected < 0 || devices_->selected >= int(devices_->items.size())) {
            dev_list_->items.clear(); dev_loaded_for_.clear(); return;
        }
        const std::string id = devices_->items[size_t(devices_->selected)].key;
        if (host_->busy()) { dev_loaded_for_.clear(); return; }   // beim nächsten Status erneut versuchen
        dev_loaded_for_ = id;
        ojson order = host_->config().at("companion").at("pages"), selected = order;
        bool custom = false;
        for (const auto& d : devices_json_)
            if (d.value("id", "") == id && d.contains("pages") && d["pages"].is_array()) { order = selected = d["pages"]; custom = true; }
        Host* h = host_;
        h->run([this, h, id, order, selected, custom]() -> std::function<void()> {
            std::vector<PluginPageInfo> plugins;
            if (h->plugins().list)
                for (const auto& e : h->plugins().list())
                    if (e.enabled) plugins.push_back({"plugin:" + e.id, Text(widen(e.name))});
            return [this, id, order, selected, custom, plugins = std::move(plugins)] {
                if (dev_loaded_for_ != id) return;
                auto items = page_items(order, selected, plugins);
                if (!custom)   // gemeinsame Liste: Plugins sind dort wie bisher sichtbar
                    for (auto& i : items) if (i.key.rfind("plugin:", 0) == 0) i.enabled = true;
                dev_items_for_ = id;
                dev_list_->items = items;
                dev_list_->original = items;
                dev_list_->selected = items.empty() ? -1 : 0;
                dev_note_->text = custom ? T(L"Dieses Gerät hat eine eigene Liste.", L"This device has its own list.")
                                         : T(L"Dieses Gerät nutzt die gemeinsame Liste (unten).", L"This device uses the shared list (below).");
                host_->relayout();
            };
        });
    }
    void save_device_pages(bool own) {
        if (!devices_ || devices_->selected < 0 || devices_->selected >= int(devices_->items.size())) return;
        const std::string id = devices_->items[size_t(devices_->selected)].key;
        if (dev_items_for_ != id) {   // Liste gehört noch zu einem anderen Gerät: nie fremde Auswahl speichern
            host_->toast(T(L"Die Liste dieses Geräts wird noch geladen. Bitte gleich erneut versuchen.", L"This device's list is still loading. Please try again in a moment."), ToastKind::Info);
            return;
        }
        std::vector<std::string> pages;
        for (const auto& item : dev_list_->items) if (item.enabled) pages.push_back(item.key);
        Host* h = host_;
        h->run([this, h, id, own, pages]() -> std::function<void()> {
            auto* c = h->app().companion();
            const bool ok = c && c->set_device_pages(id, own ? std::optional<std::vector<std::string>>(pages) : std::nullopt);
            return [this, h, ok, own] {
                dev_loaded_for_.clear();   // beim nächsten Status neu laden
                if (ok) h->toast(own ? T(L"Eigene Liste gespeichert.", L"Own list saved.") : T(L"Gerät nutzt wieder die gemeinsame Liste.", L"Device uses the shared list again."), ToastKind::Success);
                else h->toast(T(L"Gerät nicht gefunden.", L"Device not found."), ToastKind::Error);
            };
        });
    }
    void step_page(int delta) {
        Host* h = host_;
        h->run([h, delta]() -> std::function<void()> {
            if (!h->app().companion()) throw std::runtime_error(h->english() ? "Companion is not running." : "Companion läuft nicht.");
            h->app().step_page(delta, true);
            return {};
        });
    }
    // Geräteliste nur neu aufbauen, wenn sich etwas geändert hat (Auswahl bleibt erhalten).
    void refresh_devices(const ojson& c, bool en) {
        if (!devices_) return;
        std::set<std::string> online;
        if (c.is_object() && c.contains("clients") && c["clients"].is_array())
            for (const auto& x : c["clients"]) online.insert(x.value("id", ""));
        std::string sig = en ? "en" : "de";
        std::vector<ItemList::Item> items;
        if (c.is_object() && c.contains("devices") && c["devices"].is_array())
            for (const auto& d : c["devices"]) {
                const std::string id = d.value("id", "");
                const bool is_on = online.count(id) > 0;
                const long long added = d.value("added", 0LL), seen = d.value("last_seen", 0LL);
                Text detail(L"Gekoppelt am " + when(added, false) + (is_on ? L" · verbunden" : L" · zuletzt " + when(seen, false)),
                            L"Paired " + when(added, true) + (is_on ? L" · connected" : L" · last seen " + when(seen, true)));
                items.push_back({id, Text(widen(d.value("name", id))), detail, false});
                sig += "|" + id + (is_on ? "1" : "0") + std::to_string(seen) + d.value("name", "");
            }
        if (sig == signature_) return;
        signature_ = sig;
        std::string keep = devices_->selected >= 0 && devices_->selected < int(devices_->items.size()) ? devices_->items[size_t(devices_->selected)].key : "";
        devices_->items = items;
        devices_->original = items;
        devices_->selected = items.empty() ? -1 : 0;
        for (size_t i = 0; i < items.size(); ++i) if (items[i].key == keep) devices_->selected = int(i);
        host_->relayout();
    }
};

}  // namespace
std::unique_ptr<Page> make_companion_page() { return std::make_unique<CompanionPage>(); }
}  // namespace fl::ui
