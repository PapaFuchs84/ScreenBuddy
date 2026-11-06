// © 2026 FuchsLabs
#include "page_frame.h"
#include "plugin_isolation.h"
#include "util.h"
#include <mutex>

namespace fl::render {
Image draw_framed(const Rahmen& frame, const std::vector<Platzierung>& blocks) {
    Canvas d(frame.width, frame.height, frame.scale, frame.background);
    frame.header(d);
    // Wie bisher: kein Clip und keine Fehlerisolierung für eingebaute Seiten.
    for (const auto& entry : blocks)
        if (entry.placed.visible()) entry.block->draw(d, entry.placed.area, *entry.placed.density);
    frame.footer(d);
    return d.finish();
}
Image draw_arranged(const Rahmen& frame,const std::vector<Platzierung>& blocks,std::vector<std::string>* failed) {
    Canvas d(frame.width,frame.height,frame.scale,frame.background);
    frame.header(d);
    for(const auto& entry:blocks) if(entry.placed.visible()) {
        const auto& a=entry.placed.area; const int depth=d.clip_depth();
        try {
            d.clip_push(a.x,a.y,a.x+a.width-1/frame.scale,a.y+a.height-1/frame.scale);
            entry.block->draw(d,a,*entry.placed.density);
        } catch(...) {
            // Teilbilder eines fehlgeschlagenen Blocks entfernen; Nachbarblöcke bleiben erhalten.
            while(d.clip_depth()>depth) d.clip_pop();
            d.clip_push(a.x,a.y,a.x+a.width-1/frame.scale,a.y+a.height-1/frame.scale);
            d.rectangle(a.x,a.y,a.x+a.width-1/frame.scale,a.y+a.height-1/frame.scale,frame.background);
            if(failed) failed->push_back(entry.block->id());
            static std::mutex mutex;
            static plugins::isolation::RateLimiter limiter;
            std::lock_guard lock(mutex);
            const auto id=plugins::isolation::safe_id(entry.block->id());
            if(limiter.allow(id,plugins::isolation::RateLimiter::Clock::now()))
                LOGW("layout","Eingebauter Block %s ausgeblendet (Fehler beim Zeichnen)",id.c_str());
        }
        while(d.clip_depth()>depth) d.clip_pop();
    }
    frame.footer(d); return d.finish();
}
} // namespace fl::render
