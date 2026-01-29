//
// Created by a.durynin on 29.01.2026.
//

#pragma once

#include <stdexcept>
#include <string>
#include <variant>

namespace tf {

/**
 * Error types for TextFoundry operations.
 * All errors follow the "no magic" principle - deterministic and testable.
 */
enum class ErrorCode {
    // Parameter errors
    MissingParam,           ///< Required parameter not provided
    InvalidParamType,       ///< Parameter type mismatch
    UnknownParam,           ///< Parameter not defined in schema

    // Version errors
    VersionRequired,        ///< BlockRef must specify version
    VersionNotFound,        ///< Requested version does not exist
    InvalidVersion,         ///< Malformed version string

    // Entity errors
    BlockNotFound,          ///< Block with given ID not found
    CompositionNotFound,    ///< Composition with given ID not found
    DuplicateId,            ///< ID already exists

    // State errors
    InvalidStateTransition, ///< Cannot transition from current state
    DraftRequired,          ///< Operation requires Draft state
    PublishedRequired,      ///< Operation requires Published state

    // Rendering errors
    TemplateSyntaxError,    ///< Invalid template syntax
    CircularReference,      ///< Circular dependency in blocks

    // Storage errors
    StorageError,           ///< Underlying storage failure

    // Success
    Success                 ///< No error
};

/**
 * Error structure containing code and message
 */
struct Error {
    ErrorCode code;
    std::string message;

    [[nodiscard]] bool isError() const noexcept {
        return code != ErrorCode::Success;
    }

    [[nodiscard]] bool isSuccess() const noexcept {
        return code == ErrorCode::Success;
    }

    // Factory methods for common errors
    [[nodiscard]] static Error missingParam(const std::string& paramName) {
        return Error{ErrorCode::MissingParam, "Missing required parameter: " + paramName};
    }

    [[nodiscard]] static Error versionRequired() {
        return Error{ErrorCode::VersionRequired, "BlockRef must specify version (use_latest only allowed in Draft)"};
    }

    [[nodiscard]] static Error blockNotFound(const std::string& blockId) {
        return Error{ErrorCode::BlockNotFound, "Block not found: " + blockId};
    }

    [[nodiscard]] static Error compositionNotFound(const std::string& compId) {
        return Error{ErrorCode::CompositionNotFound, "Composition not found: " + compId};
    }

    [[nodiscard]] static Error success() {
        return Error{ErrorCode::Success, ""};
    }
};

/**
 * Result type - either value or error
 * Similar to std::expected (C++23) for compatibility
 */
template<typename T>
class Result {
public:
    Result(T value) : data_(std::move(value)) {}
    Result(Error error) : data_(std::move(error)) {}

    [[nodiscard]] bool hasValue() const noexcept {
        return std::holds_alternative<T>(data_);
    }

    [[nodiscard]] bool hasError() const noexcept {
        return std::holds_alternative<Error>(data_);
    }

    [[nodiscard]] T& value() & {
        return std::get<T>(data_);
    }

    [[nodiscard]] const T& value() const& {
        return std::get<T>(data_);
    }

    [[nodiscard]] T&& value() && {
        return std::get<T>(std::move(data_));
    }

    [[nodiscard]] Error& error() & {
        return std::get<Error>(data_);
    }

    [[nodiscard]] const Error& error() const& {
        return std::get<Error>(data_);
    }

    [[nodiscard]] T* operator->() {
        return &std::get<T>(data_);
    }

    [[nodiscard]] const T* operator->() const {
        return &std::get<T>(data_);
    }

    [[nodiscard]] T& operator*() & {
        return std::get<T>(data_);
    }

    [[nodiscard]] const T& operator*() const& {
        return std::get<T>(data_);
    }

private:
    std::variant<T, Error> data_;
};

/**
 * Exception thrown for unrecoverable engine errors
 */
class EngineException : public std::runtime_error {
public:
    explicit EngineException(const std::string& message)
        : std::runtime_error(message) {}
};

} // namespace tf
