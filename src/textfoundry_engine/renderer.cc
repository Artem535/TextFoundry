//
// Created by a.durynin on 29.01.2026.
//

#include "renderer.h"
#include "logger.h"

#include <sstream>

namespace tf {

// RenderResult implementation
bool RenderResult::isEmpty() const noexcept {
    return text.empty();
}

// Renderer implementation
Renderer::Renderer(std::unique_ptr<IBlockCache> cache) : blockCache_(std::move(cache)) {}

void Renderer::setBlockCache(std::unique_ptr<IBlockCache> cache) {
    blockCache_ = std::move(cache);
}

Result<RenderResult> Renderer::render(
    const Composition& composition,
    const RenderContext& context
) const {
    TF_LOG_DEBUG("Rendering composition [id={}, version={}.{}]",
                 composition.id(), composition.version().major, composition.version().minor);

    // Only Published compositions can be rendered
    if (composition.state() != BlockState::Published) {
        TF_LOG_ERROR("Cannot render composition: only Published compositions can be rendered [id={}, state={}]",
                     composition.id(), static_cast<int>(composition.state()));
        return Result<RenderResult>(Error{ErrorCode::PublishedRequired,
                     "Only Published compositions can be rendered"});
    }

    // Render all fragments
    std::vector<std::string> fragmentTexts;
    std::vector<std::pair<BlockId, Version>> blocksUsed;

    TF_LOG_TRACE("Rendering {} fragments", composition.fragments().size());
    for (const auto& fragment : composition.fragments()) {
        auto result = renderFragment(fragment, context, blocksUsed);
        if (result.hasError()) {
            TF_LOG_ERROR("Failed to render fragment: {}", result.error().message);
            return Result<RenderResult>(result.error());
        }
        fragmentTexts.push_back(std::move(result.value()));
    }

    // Apply structural style
    auto style = getEffectiveStyle(composition);
    std::string finalText = applyStructuralStyle(fragmentTexts, style);

    // Format output
    finalText = formatOutput(finalText, style.outputFormat);

    RenderResult result;
    result.text = std::move(finalText);
    result.compositionId = composition.id();
    result.compositionVersion = composition.version();
    result.blocksUsed = std::move(blocksUsed);
    result.format = style.outputFormat;

    TF_LOG_DEBUG("Composition rendered successfully [id={}, blocks_used={}]",
                 composition.id(), result.blocksUsed.size());

    return Result<RenderResult>(result);
}

Result<std::string> Renderer::renderBlock(
    const Block& block,
    const RenderContext& context
) const {
    // Merge defaults with context params (context has priority)
    Params merged = block.defaults();
    for (const auto& [key, value] : context.params) {
        merged[key] = value;
    }

    return block.templ().expand(merged);
}

std::string Renderer::applyStructuralStyle(
    const std::vector<std::string>& fragmentTexts,
    const StructuralStyle& style
) {
    std::ostringstream result;

    // Add preamble
    if (style.preamble.has_value()) {
        result << style.preamble.value();
    }

    // Join fragments with delimiter
    for (size_t i = 0; i < fragmentTexts.size(); ++i) {
        std::string text = fragmentTexts[i];

        // Apply block wrapper if present
        if (style.blockWrapper.has_value()) {
            std::string wrapped = style.blockWrapper.value();
            // Replace {{content}} placeholder
            constexpr std::string_view content = "{{content}}";
            size_t pos = wrapped.find(content);
            if (pos != std::string::npos) {
                wrapped.replace(pos, content.size(), text);
            }
            text = wrapped;
        }

        result << text;

        // Add delimiter between fragments (not after last)
        if (i < fragmentTexts.size() - 1 && style.delimiter.has_value()) {
            result << style.delimiter.value();
        }
    }

    // Add postamble
    if (style.postamble.has_value()) {
        result << style.postamble.value();
    }

    return result.str();
}

Result<std::string> Renderer::renderFragment(
    const Fragment& fragment,
    const RenderContext& context,
    std::vector<std::pair<BlockId, Version>>& blocksUsed
) const {
    switch (fragment.type()) {
        case FragmentType::BlockRef:
            return expandBlockRef(fragment.asBlockRef(), context, blocksUsed);

        case FragmentType::StaticText:
            return Result<std::string>(fragment.asStaticText().text());

        case FragmentType::Separator:
            return Result<std::string>(fragment.asSeparator().toString());
    }
    return Result<std::string>(Error{ErrorCode::InvalidParamType, "Unknown fragment type"});
}

Result<std::string> Renderer::expandBlockRef(
    const BlockRef& blockRef,
    const RenderContext& context,
    std::vector<std::pair<BlockId, Version>>& blocksUsed
) const {
    if (!blockCache_) {
        return Result<std::string>(Error{ErrorCode::StorageError, "No block cache available"});
    }

    const Block* block = nullptr;
    Version usedVersion;

    if (blockRef.useLatest()) {
        block = blockCache_->getLatestBlock(blockRef.blockId());
        if (block) {
            usedVersion = block->version();
        }
    } else {
        usedVersion = blockRef.version().value_or(Version{0, 0});
        block = blockCache_->getBlock(blockRef.blockId(), usedVersion);
    }

    if (!block) {
        return Result<std::string>(Error::blockNotFound(blockRef.blockId()));
    }

    // Track block usage
    blocksUsed.emplace_back(blockRef.blockId(), usedVersion);

    // Resolve parameters
    auto paramsResult = blockRef.resolveParams(*block, context.params);
    if (paramsResult.hasError()) {
        return Result<std::string>(paramsResult.error());
    }

    // Expand template
    return block->templ().expand(paramsResult.value());
}

StructuralStyle Renderer::getEffectiveStyle(const Composition& composition) const {
    if (composition.styleProfile().has_value()) {
        return composition.styleProfile()->structural;
    }
    return StructuralStyle::plain();
}

std::string Renderer::formatOutput(
    const std::string& text,
    StructuralStyle::OutputFormat format
) const {
    // For now, just return text as-is
    // Format-specific handling can be added later
    (void)format;
    return text;
}

} // namespace tf
