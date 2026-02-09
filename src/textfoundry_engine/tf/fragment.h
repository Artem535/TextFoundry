//
// Created by artem.d on 28.01.2026.
//

#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "blockref.h"
#include "blocktype.hpp"
#include "error.h"

namespace tf {

/**
 * Fragment types supported in Composition
 */
enum class FragmentType {
  BlockRef,    ///< Reference to a Block
  StaticText,  ///< Raw text without parameters
  Separator    ///< Typed separator (newline, paragraph, hr)
};

/**
 * Static text fragment - no parameters, no versioning
 * Stored inline in Composition, doesn't create Block entities
 */
struct StaticText {
  std::string content;

  explicit StaticText(std::string text) : content(std::move(text)) {}

  [[nodiscard]] const std::string& text() const noexcept;
};

/**
 * Separator fragment - typed delimiter
 */
struct Separator {
  SeparatorType type;

  explicit Separator(const SeparatorType& sepType) : type(sepType) {}

  /**
   * Get string representation of separator
   */
  [[nodiscard]] std::string toString() const;
};

/**
 * Fragment - single element of Composition
 * Can be BlockRef, StaticText, or Separator
 */
class Fragment {
 public:
  // Constructors for each fragment type
  Fragment() : data_(StaticText("")) {}

  explicit Fragment(BlockRef blockRef) : data_(std::move(blockRef)) {}

  explicit Fragment(StaticText staticText) : data_(std::move(staticText)) {}

  explicit Fragment(Separator separator) : data_(separator) {}

  // Factory methods
  [[nodiscard]] static Fragment make_block_ref(BlockRef ref) {
    return Fragment(std::move(ref));
  }

  [[nodiscard]] static Fragment make_static_text(std::string text) {
    return Fragment(StaticText(std::move(text)));
  }

  [[nodiscard]] static Fragment make_separator(SeparatorType type) {
    return Fragment(Separator(type));
  }

  // Type checking
  [[nodiscard]] FragmentType type() const noexcept {
    return std::visit(
        [](const auto& val) -> FragmentType {
          using T = std::decay_t<decltype(val)>;
          if constexpr (std::is_same_v<T, BlockRef>)
            return FragmentType::BlockRef;
          if constexpr (std::is_same_v<T, StaticText>)
            return FragmentType::StaticText;
          if constexpr (std::is_same_v<T, Separator>)
            return FragmentType::Separator;
          return FragmentType::StaticText;  // default
        },
        data_);
  }

  [[nodiscard]] bool is_block_ref() const noexcept {
    return std::holds_alternative<BlockRef>(data_);
  }

  [[nodiscard]] bool is_static_text() const noexcept {
    return std::holds_alternative<StaticText>(data_);
  }

  [[nodiscard]] bool is_separator() const noexcept {
    return std::holds_alternative<Separator>(data_);
  }

  // Accessors (use only after checking type)
  [[nodiscard]] BlockRef& as_block_ref() & { return std::get<BlockRef>(data_); }

  [[nodiscard]] const BlockRef& as_block_ref() const& {
    return std::get<BlockRef>(data_);
  }

  [[nodiscard]] StaticText& as_static_text() & {
    return std::get<StaticText>(data_);
  }

  [[nodiscard]] const StaticText& as_static_text() const& {
    return std::get<StaticText>(data_);
  }

  [[nodiscard]] Separator& as_separator() & {
    return std::get<Separator>(data_);
  }

  [[nodiscard]] const Separator& as_separator() const& {
    return std::get<Separator>(data_);
  }

  // Safe accessors returning nullptr if wrong type
  [[nodiscard]] BlockRef* get_block_ref() noexcept {
    return std::get_if<BlockRef>(&data_);
  }

  [[nodiscard]] const BlockRef* get_block_ref() const noexcept {
    return std::get_if<BlockRef>(&data_);
  }

  [[nodiscard]] StaticText* get_static_text() noexcept {
    return std::get_if<StaticText>(&data_);
  }

  [[nodiscard]] const StaticText* get_static_text() const noexcept {
    return std::get_if<StaticText>(&data_);
  }

  [[nodiscard]] Separator* get_separator() noexcept {
    return std::get_if<Separator>(&data_);
  }

  [[nodiscard]] const Separator* get_separator() const noexcept {
    return std::get_if<Separator>(&data_);
  }

  // Validation
  [[nodiscard]] Error validate(bool isDraftContext = false) const;

 private:
  std::variant<BlockRef, StaticText, Separator> data_;
};
}  // namespace tf
