// Created by a.durynin on 02.02.2026.

#pragma once

#include <sago/platform_folders.h>

#include <optional>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

struct ApplicationSettings {
  std::string project_key{"default"};
  fs::path data_path{fs::path(sago::getConfigHome()) / "TextFoundry"};
  bool dry_run{false};
  bool strict_mode{false};
  std::optional<std::string> target_language;
};
