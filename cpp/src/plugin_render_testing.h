// © 2026 FuchsLabs – test entry point for fault injection into plugin block drawing.
// The hook code is compiled into every build that contains plugin_render.cpp (app, preview, tests), but it is
// only reachable through testing::draw_page(); the public draw_page() always passes no hooks (nullptr), and no
// production source includes this header or calls testing::draw_page().
#pragma once
#include <functional>
#include <string>
#include "plugin_render.h"

namespace fl::plugins::testing {
struct DrawHooks {
    // Called once per drawn block, inside that block's isolation, right after its card background was
    // painted. Throwing here simulates a block that fails half-way (key: block ID, v1 "#<index>"); the
    // canvas lets a test also leave clips open, as a faulty block could.
    std::function<void(Canvas& canvas, const std::string& block_key)> after_card;
    // Called once per drawn block, inside its isolation, after all of its content (texts, bar, list) was drawn.
    // Throwing here simulates a block that fails after it has drawn everything.
    std::function<void(Canvas& canvas, const std::string& block_key)> after_content;
};
// Caller holds render::mutex() and has applied the requested theme (as for draw_page).
Image draw_page(const Page& page, int width, int height, bool sample, const DrawHooks& hooks);
} // namespace fl::plugins::testing