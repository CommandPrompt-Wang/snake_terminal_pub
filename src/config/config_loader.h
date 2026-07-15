#pragma once
#include <string>

// simple text‑based config loader
// format: key = value  (one per line, '#' for comments)
bool load_config(const std::string& path);
void save_config(const std::string& path);
