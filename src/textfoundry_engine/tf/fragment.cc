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
const std::string& StaticText::text() const noexcept { return content; }

// Fragment implementation
Error Fragment::validate(bool isDraftContext) const {
  return std::visit(
      [&](const auto& val) -> Error {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, BlockRef>) {
          return val.validate(isDraftContext);
        }
        // StaticText and Separator are always valid
        return Error::success();
      },
      data_);
}
}  // namespace tf
