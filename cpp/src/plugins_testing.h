// © 2026 FuchsLabs – test entry point for fault injection into plugin data evaluation.
// The hook code is compiled into every build that contains plugins.cpp (app, validator, tests), but it is only
// reachable through testing::render_page(); the public render_page() always passes no hooks (nullptr), and no
// production source includes this header or calls testing::render_page().
#pragma once
#include <functional>
#include <string>
#include "plugins.h"

namespace fl::plugins::testing {
struct DataHooks {
    // Called once per block, inside that block's isolation, before its data is evaluated. Throwing
    // here simulates a failing evaluation of exactly this block (key: block ID, v1 "#<index>").
    std::function<void(const std::string& block_key)> before_block;
    // Called once after the settings were read, outside every block's isolation. Throwing here simulates a
    // failure outside the blocks (outer error boundary of render_page).
    std::function<void()> outside_blocks;
};
Page render_page(const Manifest& manifest, const RenderContext& context, const DataHooks& hooks);
} // namespace fl::plugins::testing