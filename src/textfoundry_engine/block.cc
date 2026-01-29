//
// Created by artem.d on 28.01.2026.
//

#include "block.h"

#include <regex>

namespace tf {

// Template implementation
std::vector<std::string> Template::extractParamNames() const {
    std::vector<std::string> names;
    // Regex to match {{paramName}} pattern
    std::regex pattern(R"(\{\{(\w+)\}\})");
    std::sregex_iterator begin(content_.begin(), content_.end(), pattern);
    std::sregex_iterator end;

    for (auto it = begin; it != end; ++it) {
        names.push_back((*it)[1].str());
    }
    return names;
}

Result<std::string> Template::expand(const Params& params) const {
    std::string result = content_;
    std::regex pattern(R"(\{\{(\w+)\}\})");
    std::smatch match;

    std::string::const_iterator searchStart(result.cbegin());
    while (std::regex_search(searchStart, result.cend(), match, pattern)) {
        std::string paramName = match[1].str();
        auto it = params.find(paramName);
        if (it == params.end()) {
            return Error::missingParam(paramName);
        }
        // Replace {{paramName}} with value
        size_t pos = match.position();
        size_t len = match.length();
        result.replace(pos, len, it->second);
        searchStart = result.cbegin() + pos + it->second.length();
    }

    return result;
}

// Block implementation
Result<ParamValue> Block::resolveParam(
    const std::string& paramName,
    const Params& localOverride,
    const Params& runtimeContext
) const {
    // 1. Check runtime context (highest priority)
    auto runtimeIt = runtimeContext.find(paramName);
    if (runtimeIt != runtimeContext.end()) {
        return runtimeIt->second;
    }

    // 2. Check local override
    auto localIt = localOverride.find(paramName);
    if (localIt != localOverride.end()) {
        return localIt->second;
    }

    // 3. Check block defaults (lowest priority)
    auto defaultIt = defaults_.find(paramName);
    if (defaultIt != defaults_.end()) {
        return defaultIt->second;
    }

    // Parameter not found
    return Error::missingParam(paramName);
}

Error Block::validateParams(
    const Params& localOverride,
    const Params& runtimeContext
) const {
    // Check all parameters in template have values
    auto paramNames = template_.extractParamNames();
    for (const auto& paramName : paramNames) {
        if (!canResolveParam(paramName, localOverride, runtimeContext)) {
            return Error::missingParam(paramName);
        }
    }
    return Error::success();
}

Error Block::publish(Version newVersion) {
    if (state_ != BlockState::Draft) {
        return Error{ErrorCode::InvalidStateTransition,
                     "Only Draft blocks can be published"};
    }
    if (id_.empty()) {
        return Error{ErrorCode::InvalidParamType, "Block must have an ID"};
    }
    if (template_.content().empty()) {
        return Error{ErrorCode::InvalidParamType, "Block must have a template"};
    }

    state_ = BlockState::Published;
    version_ = newVersion;
    return Error::success();
}

void Block::deprecate() {
    if (state_ == BlockState::Published) {
        state_ = BlockState::Deprecated;
    }
}

bool Block::canResolveParam(
    const std::string& paramName,
    const Params& localOverride,
    const Params& runtimeContext
) const {
    // Check runtime context
    if (runtimeContext.contains(paramName)) {
        return true;
    }

    // Check local override
    if (localOverride.contains(paramName)) {
        return true;
    }

    // Check block defaults
    if (defaults_.contains(paramName)) {
        return true;
    }

    return false;
}

} // namespace tf
