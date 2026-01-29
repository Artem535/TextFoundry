//
// Created by artem.d on 28.01.2026.
//

#include "fragment.h"

namespace tf {

std::string Separator::toString() const {
    switch (type) {
        case SeparatorType::Newline:
            return "\n";
        case SeparatorType::Paragraph:
            return "\n\n";
        case SeparatorType::Hr:
            return "\n---\n";
    }
    return "";
}

// StaticText implementation
const std::string& StaticText::text() const noexcept {
    return content;
}

// Fragment implementation
Error Fragment::validate(bool isDraftContext) const {
    return std::visit([&](const auto& val) -> Error {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, BlockRef>) {
            return val.validate(isDraftContext);
        }
        // StaticText and Separator are always valid
        return Error::success();
    }, data_);
}

// FragmentJsonSerializer stub implementations
std::string FragmentJsonSerializer::serialize(const Fragment& fragment) {
    // TODO: Implement JSON serialization
    (void)fragment;
    return "{}";
}

std::string FragmentJsonSerializer::serialize(const std::vector<Fragment>& fragments) {
    // TODO: Implement JSON array serialization
    (void)fragments;
    return "[]";
}

Result<Fragment> FragmentJsonSerializer::deserialize(std::string_view json) {
    // TODO: Implement JSON deserialization
    (void)json;
    return Result<Fragment>(Fragment::makeStaticText(""));
}

Result<std::vector<Fragment>> FragmentJsonSerializer::deserializeList(std::string_view json) {
    // TODO: Implement JSON array deserialization
    (void)json;
    return Result<std::vector<Fragment>>(std::vector<Fragment>{});
}

} // namespace tf
