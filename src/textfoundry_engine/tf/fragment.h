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
  [[nodiscard]] static Fragment MakeBlockRef(BlockRef ref) {
    return Fragment(std::move(ref));
  }

  [[nodiscard]] static Fragment MakeStaticText(std::string text) {
    return Fragment(StaticText(std::move(text)));
  }

  [[nodiscard]] static Fragment MakeSeparator(SeparatorType type) {
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

  [[nodiscard]] bool IsBlockRef() const noexcept {
    return std::holds_alternative<BlockRef>(data_);
  }

  [[nodiscard]] bool IsStaticText() const noexcept {
    return std::holds_alternative<StaticText>(data_);
  }

  [[nodiscard]] bool IsSeparator() const noexcept {
    return std::holds_alternative<Separator>(data_);
  }

  // Accessors (use only after checking type)
  [[nodiscard]] BlockRef& AsBlockRef() & { return std::get<BlockRef>(data_); }

  [[nodiscard]] const BlockRef& AsBlockRef() const& {
    return std::get<BlockRef>(data_);
  }

  [[nodiscard]] StaticText& AsStaticText() & {
    return std::get<StaticText>(data_);
  }

  [[nodiscard]] const StaticText& AsStaticText() const& {
    return std::get<StaticText>(data_);
  }

  [[nodiscard]] Separator& AsSeparator() & {
    return std::get<Separator>(data_);
  }

  [[nodiscard]] const Separator& AsSeparator() const& {
    return std::get<Separator>(data_);
  }

  // Safe accessors returning nullptr if wrong type
  [[nodiscard]] BlockRef* GetBlockRef() noexcept {
    return std::get_if<BlockRef>(&data_);
  }

  [[nodiscard]] const BlockRef* GetBlockRef() const noexcept {
    return std::get_if<BlockRef>(&data_);
  }

  [[nodiscard]] StaticText* GetStaticText() noexcept {
    return std::get_if<StaticText>(&data_);
  }

  [[nodiscard]] const StaticText* GetStaticText() const noexcept {
    return std::get_if<StaticText>(&data_);
  }

  [[nodiscard]] Separator* GetSeparator() noexcept {
    return std::get_if<Separator>(&data_);
  }

  [[nodiscard]] const Separator* GetSeparator() const noexcept {
    return std::get_if<Separator>(&data_);
  }

  // Validation
  [[nodiscard]] Error validate(bool isDraftContext = false) const;

 private:
  std::variant<BlockRef, StaticText, Separator> data_;
};
}  // namespace tf
