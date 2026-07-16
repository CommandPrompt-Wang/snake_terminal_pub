#pragma once
#include <filesystem>
#include <string>

// 根据 app 目录（只读 AppImage 挂载点亦可）解析可写的 snake.cfg 路径
void init_config_path(const std::filesystem::path& app_dir);
void log_config_startup();
const std::filesystem::path& config_path();

bool load_config();
void save_config();
