//
// Created by a.durynin on 29.01.2026.
//

#pragma once

#include <cstdint>
#include <compare>
#include <format>
#include <string>

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
    [[nodiscard]] std::string toString() const {
        return std::format("{}.{}", major, minor);
    }
};

} // namespace tf

/**
 * Formatter for Version
 */
template<>
struct std::formatter<tf::Version> : std::formatter<std::string> {
    template<typename FormatContext>
    auto format(const tf::Version& v, FormatContext& ctx) const {
        return std::formatter<std::string>::format(v.toString(), ctx);
    }
};
