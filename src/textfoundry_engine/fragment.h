//
// Created by artem.d on 28.01.2026.
//

#pragma once

#include "blockref.h"
#include "blocktype.hpp"
#include "error.h"

#include <memory>
#include <string>
#include <vector>
#include <variant>

namespace tf {

/**
 * Fragment types supported in Composition
 */
enum class FragmentType {
    BlockRef,   ///< Reference to a Block
    StaticText, ///< Raw text without parameters
    Separator   ///< Typed separator (newline, paragraph, hr)
};

/**
 * Static text fragment - no parameters, no versioning
 * Stored inline in Composition, doesn't create Block entities
 */
struct StaticText {
    std::string content;

    explicit StaticText(std::string text) : content(std::move(text)) {}

    [[nodiscard]] const std::string& text() const noexcept { return content; }
};

/**
 * Separator fragment - typed delimiter
 */
struct Separator {
    SeparatorType type;

    explicit Separator(SeparatorType sepType) : type(sepType) {}

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

    explicit Fragment(BlockRef blockRef)
        : data_(std::move(blockRef)) {}

    explicit Fragment(StaticText staticText)
        : data_(std::move(staticText)) {}

    explicit Fragment(Separator separator)
        : data_(std::move(separator)) {}

    // Factory methods
    [[nodiscard]] static Fragment makeBlockRef(BlockRef ref) {
        return Fragment(std::move(ref));
    }

    [[nodiscard]] static Fragment makeStaticText(std::string text) {
        return Fragment(StaticText(std::move(text)));
    }

    [[nodiscard]] static Fragment makeSeparator(SeparatorType type) {
        return Fragment(Separator(type));
    }

    // Type checking
    [[nodiscard]] FragmentType type() const noexcept {
        return std::visit([](const auto& val) -> FragmentType {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, BlockRef>) return FragmentType::BlockRef;
            if constexpr (std::is_same_v<T, StaticText>) return FragmentType::StaticText;
            if constexpr (std::is_same_v<T, Separator>) return FragmentType::Separator;
            return FragmentType::StaticText;  // default
        }, data_);
    }

    [[nodiscard]] bool isBlockRef() const noexcept {
        return std::holds_alternative<BlockRef>(data_);
    }

    [[nodiscard]] bool isStaticText() const noexcept {
        return std::holds_alternative<StaticText>(data_);
    }

    [[nodiscard]] bool isSeparator() const noexcept {
        return std::holds_alternative<Separator>(data_);
    }

    // Accessors (use only after checking type)
    [[nodiscard]] BlockRef& asBlockRef() & {
        return std::get<BlockRef>(data_);
    }

    [[nodiscard]] const BlockRef& asBlockRef() const& {
        return std::get<BlockRef>(data_);
    }

    [[nodiscard]] StaticText& asStaticText() & {
        return std::get<StaticText>(data_);
    }

    [[nodiscard]] const StaticText& asStaticText() const& {
        return std::get<StaticText>(data_);
    }

    [[nodiscard]] Separator& asSeparator() & {
        return std::get<Separator>(data_);
    }

    [[nodiscard]] const Separator& asSeparator() const& {
        return std::get<Separator>(data_);
    }

    // Safe accessors returning nullptr if wrong type
    [[nodiscard]] BlockRef* getBlockRef() noexcept {
        return std::get_if<BlockRef>(&data_);
    }

    [[nodiscard]] const BlockRef* getBlockRef() const noexcept {
        return std::get_if<BlockRef>(&data_);
    }

    [[nodiscard]] StaticText* getStaticText() noexcept {
        return std::get_if<StaticText>(&data_);
    }

    [[nodiscard]] const StaticText* getStaticText() const noexcept {
        return std::get_if<StaticText>(&data_);
    }

    [[nodiscard]] Separator* getSeparator() noexcept {
        return std::get_if<Separator>(&data_);
    }

    [[nodiscard]] const Separator* getSeparator() const noexcept {
        return std::get_if<Separator>(&data_);
    }

    // Validation
    [[nodiscard]] Error validate(bool isDraftContext = false) const {
        return std::visit([&](const auto& val) -> Error {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, BlockRef>) {
                return val.validate(isDraftContext);
            }
            // StaticText and Separator are always valid
            return Error::success();
        }, data_);
    }

private:
    std::variant<BlockRef, StaticText, Separator> data_;
};

/**
 * JSON serialization helper for fragments
 * Used for storing Composition.fragments_json
 */
class FragmentJsonSerializer {
public:
    /**
     * Serialize fragment to JSON string
     */
    [[nodiscard]] static std::string serialize(const Fragment& fragment);

    /**
     * Serialize list of fragments to JSON array
     */
    [[nodiscard]] static std::string serialize(const std::vector<Fragment>& fragments);

    /**
     * Deserialize fragment from JSON
     */
    [[nodiscard]] static Result<Fragment> deserialize(std::string_view json);

    /**
     * Deserialize list of fragments from JSON array
     */
    [[nodiscard]] static Result<std::vector<Fragment>> deserializeList(std::string_view json);
};

} // namespace tf
