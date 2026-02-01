//
// Created by a.durynin on 29.01.2026.
//

#pragma once

#include <cstdint>
#include <format>
#include <string>
#include <fmt/base.h>

namespace tf {

/**
 * Version of a block or composition.
 * Follows semantic versioning principles (major.minor).
 * New versions are created for any change to Published entity.
 */
struct Version {
    uint16_t major = 0;  ///< Major version - breaking changes
    uint16_t minor = 0;  ///< Minor version - backward compatible additions

    [[nodiscard]] constexpr auto operator<=>(const Version&) const = default;
    [[nodiscard]] constexpr bool operator==(const Version&) const = default;

    /**
     * Convert version to string format "major.minor"
     */
    [[nodiscard]] std::string to_string() const {
        return std::format("{}.{}", major, minor);
    }
};

} // namespace tf


// fmt support for spdlog (corrected for fmt v9+)
template<>
struct fmt::formatter<tf::Version> {
    template<typename ParseContext>
    constexpr auto parse(ParseContext& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const tf::Version& v, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}.{}", v.major, v.minor);
    }
};

// C++23 std::format support
template<>
struct std::formatter<tf::Version> : std::formatter<std::string> {
    template<typename FormatContext>
    auto format(const tf::Version& v, FormatContext& ctx) const {
        return std::formatter<std::string>::format(v.to_string(), ctx);
    }
};
