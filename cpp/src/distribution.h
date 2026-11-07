#pragma once
#include <filesystem>
namespace fl::distribution {
bool packaged();
std::filesystem::path data_dir();
bool startup_enabled();
void set_startup(bool enabled);
void open_updates();
}
