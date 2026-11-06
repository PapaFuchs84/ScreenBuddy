#include "plugin_schedule.h"
#include <algorithm>
#include <cmath>
#include <set>
namespace fl::plugins::schedule {
size_t serial_bytes(int rows, int width) { return rows > 0 && width > 0 ? size_t((rows+7)/8)*8*width*2 : 0; }
Budget budget_for(OutputKind kind, std::optional<double> rate, double last, double ms, int rows, int width, size_t clients) {
    Budget b; b.min_entry_seconds = 4*last;
    if (kind == OutputKind::monitor && ms <= 100) { b.shape = Shape::fade; b.steps = ms <= 50 ? 4 : 2; b.step_seconds = .4/b.steps; }
    if (kind == OutputKind::companion && clients) { b.skip_late = true; b.shape = Shape::fade; b.steps = 2; b.step_seconds = .2; if (ms > 100) { b.shape = Shape::cut; b.steps = 0; } }
    if (kind == OutputKind::serial && rate && *rate > 0 && serial_bytes(rows,width)/ *rate <= .6) {
        b.shape = Shape::reveal; b.steps = 4;
        // Voller realer Zyklus: Bandübertragung, Renderzeit und Ruhe im display_loop.
        b.step_seconds = serial_bytes(rows,width)/ *rate/4 + ms/1000 + .05;
        if (4*b.step_seconds > .6) {
            b.steps = 2; b.step_seconds = serial_bytes(rows,width)/ *rate/2 + ms/1000 + .05;
        }
        if (2*b.step_seconds > .6) { b.shape = Shape::cut; b.steps = 0; }
    }
    return b;
}
bool should_render(bool dirty, bool due, bool live, int step, bool size, bool minute, double now, double deadline) {
    return dirty || due || live || step || size || minute || now >= deadline;
}
Wake wake_at(double now, const std::vector<double>& deadlines, bool animating, bool meter) {
    double gap = animating ? .05 : meter ? .1 : .5;
    double until = now + 1;
    for (double d : deadlines) until = std::min(until,d);
    return {gap,until};
}
void reset(std::map<std::string,ZoneState>& state, bool generation_changed, bool size_changed) {
    if (generation_changed) state.clear();
    else if (size_changed) for (auto& [zone,s] : state) s.fade.reset();
}
Result advance(const std::vector<ZoneInput>& zones, std::map<std::string,ZoneState>& state, double now, const Budget& budget, const std::vector<std::string>& requests) {
    Result result; std::set<std::string> visible;
    for (const auto& z : zones) visible.insert(z.zone);
    std::erase_if(state, [&](const auto& item) { return !visible.contains(item.first); });
    for (const auto& z : zones) {
        auto& s = state[z.zone]; bool waiting = false; ZoneChoice c; c.zone = z.zone;
        std::vector<size_t> available;
        for (size_t i=0;i<z.entries.size();++i) if (z.entries[i].available) available.push_back(i);
        auto valid = [&](size_t i) { return i < z.entries.size() && z.entries[i].available; };
        if (available.empty()) { s = {}; result.zones.push_back(c); continue; }
        if (!s.shown || !valid(*s.shown)) { auto next = s.shown && z.mode == Mode::rotate
                ? std::upper_bound(available.begin(),available.end(),*s.shown) : available.begin();
            s.shown = next == available.end() ? available.front() : *next; s.since = now; s.fade.reset(); }
        if (s.fade) {
            auto f = *s.fade;
            int k = std::min(f.steps,int(std::max(0.0,now-f.start+1e-9)/f.step_seconds));
            // Nur Companion verwirft verspätete Schritte; seriell jedes Band genau einmal.
            if (now != s.last_now && k > s.last_step+1 && f.skip_late) k = f.steps;
            if (f.shape == Shape::reveal && now != s.last_now && k > s.last_step) {
                k = std::min(k,s.last_step+1);
                s.fade->start = f.start = now-k*f.step_seconds;
            }
            if (k >= f.steps || !valid(f.from) || !valid(f.to)) s.fade.reset();
            else { c.from_block = z.entries[f.from].block; c.shape = f.shape; c.mix = float(k)/f.steps;
                result.animating = true; result.next_deadline = std::min(result.next_deadline,f.start+(k+1)*f.step_seconds); }
            s.last_step = k;
        }
        if (!s.fade) {
            size_t target = *s.shown;
            double end = s.since + std::max(double(z.entries[target].seconds),budget.min_entry_seconds);
            if (z.mode == Mode::first_available) target = available.front();
            else if (now >= end || (now != s.last_now && std::find(requests.begin(),requests.end(),z.zone) != requests.end())) {
                auto i = std::find(available.begin(),available.end(),target);
                target = available[(size_t(i-available.begin())+1)%available.size()];
            }
            if (target != *s.shown) {
                double busy_until = now;
                for (const auto& [key,other] : state) if (key != z.zone && other.fade)
                    busy_until = std::max(busy_until,other.fade->start + std::min(.6,other.fade->steps*other.fade->step_seconds));
                if (busy_until > now) { waiting = true; result.next_deadline = std::min(result.next_deadline,busy_until); }
                else {
                    if (z.animate && budget.steps >= 2 && budget.step_seconds > 0) {
                        s.fade = ZoneState::Fade{*s.shown,target,now,budget.steps,budget.step_seconds,budget.shape,budget.skip_late};
                        c.from_block = z.entries[*s.shown].block; c.shape = budget.shape; c.mix = 0;
                        s.last_step = 0; result.animating = true; result.next_deadline = std::min(result.next_deadline,now+budget.step_seconds);
                    }
                    s.shown = target; s.since = now;
                }
            }
        }
        c.block = z.entries[*s.shown].block;
        if (!s.fade && !waiting && z.mode == Mode::rotate && available.size()>1)
            result.next_deadline = std::min(result.next_deadline,s.since+std::max(double(z.entries[*s.shown].seconds),budget.min_entry_seconds));
        s.last_now = now; result.zones.push_back(c);
    }
    return result;
}
}
