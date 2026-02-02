// Created by a.durynin on 02.02.2026.

#pragma once

#include <optional>
#include <string>

struct ApplicationSettings {
    std::string project_key{"default"};
    std::string data_path{"memory:objectbox"};
    bool dry_run{false};
    bool strict_mode{false};
    std::optional<std::string> target_language;
};
