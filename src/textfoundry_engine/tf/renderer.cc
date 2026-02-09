//
// Created by a.durynin on 29.01.2026.
//

#include "renderer.h"

#include <sstream>

#include "logger.h"

namespace tf {

// RenderResult implementation
bool RenderResult::isEmpty() const noexcept { return text.empty(); }

// Renderer implementation
Renderer::Renderer(std::unique_ptr<IBlockCache> cache)
    : blockCache_(std::move(cache)) {}

void Renderer::set_block_cache(std::unique_ptr<IBlockCache> cache) {
  blockCache_ = std::move(cache);
}

void Renderer::clear_cache() {
  if (blockCache_) {
    blockCache_->clear();
  }
}

Result<RenderResult> Renderer::render(const Composition& composition,
                                      const RenderContext& context) const {
  TF_LOG_DEBUG("Rendering composition [id={}, version={}.{}]", composition.id(),
               composition.version().major, composition.version().minor);

  // Only Published compositions can be rendered
  if (composition.state() != BlockState::Published) {
    TF_LOG_ERROR(
        "Cannot render composition: only Published compositions can be "
        "rendered [id={}, state={}]",
        composition.id(), static_cast<int>(composition.state()));
    return Result<RenderResult>(
        Error{ErrorCode::PublishedRequired,
              "Only Published compositions can be rendered"});
  }

  // Render all fragments
  std::vector<std::string> fragmentTexts;
  std::vector<std::pair<BlockId, Version>> blocksUsed;

  TF_LOG_TRACE("Rendering {} fragments", composition.fragments().size());
  for (const auto& fragment : composition.fragments()) {
    auto result = render_fragment(fragment, context, blocksUsed);
    if (result.has_error()) {
      TF_LOG_ERROR("Failed to render fragment: {}", result.error().message);
      return Result<RenderResult>(result.error());
    }
    fragmentTexts.push_back(std::move(result.value()));
  }

  // Apply structural style
  auto style = get_effective_style(composition);
  std::string finalText = apply_structural_style(fragmentTexts, style);

  RenderResult result;
  result.text = std::move(finalText);
  result.compositionId = composition.id();
  result.compositionVersion = composition.version();
  result.blocksUsed = std::move(blocksUsed);

  TF_LOG_DEBUG("Composition rendered successfully [id={}, blocks_used={}]",
               composition.id(), result.blocksUsed.size());

  return Result<RenderResult>(result);
}

Result<std::string> Renderer::render_block(const Block& block,
                                           const RenderContext& context) {
  // Merge defaults with context params (context has priority)
  Params merged = block.defaults();
  for (const auto& [key, value] : context.params) {
    merged[key] = value;
  }

  return block.templ().expand(merged);
}

std::string Renderer::apply_structural_style(
    const std::vector<std::string>& fragmentTexts,
    const StructuralStyle& style) {
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
      if (const size_t pos = wrapped.find(content); pos != std::string::npos) {
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

Result<std::string> Renderer::render_fragment(
    const Fragment& fragment, const RenderContext& context,
    std::vector<std::pair<BlockId, Version>>& blocksUsed) const {
  switch (fragment.type()) {
    case FragmentType::BlockRef:
      return expand_block_ref(fragment.as_block_ref(), context, blocksUsed);

    case FragmentType::StaticText:
      return Result<std::string>(fragment.as_static_text().text());

    case FragmentType::Separator:
      return Result<std::string>(fragment.as_separator().toString());
  }
  return Result<std::string>(
      Error{ErrorCode::InvalidParamType, "Unknown fragment type"});
}

Result<std::string> Renderer::expand_block_ref(
    const BlockRef& blockRef, const RenderContext& context,
    std::vector<std::pair<BlockId, Version>>& blocksUsed) const {
  if (!blockCache_) {
    return Result<std::string>(
        Error{ErrorCode::StorageError, "No block cache available"});
  }

  const Block* block = nullptr;
  Version usedVersion;

  if (blockRef.use_latest()) {
    block = blockCache_->get_latest_block(blockRef.block_id());
    if (block) {
      usedVersion = block->version();
    }
  } else {
    usedVersion = blockRef.version().value_or(Version{0, 0});
    block = blockCache_->get_block(blockRef.block_id(), usedVersion);
  }

  if (!block) {
    return Result<std::string>(Error::block_not_found(blockRef.block_id()));
  }

  // Track block usage
  blocksUsed.emplace_back(blockRef.block_id(), usedVersion);

  // Resolve parameters
  auto paramsResult = blockRef.resolve_params(*block, context.params);
  if (paramsResult.has_error()) {
    return Result<std::string>(paramsResult.error());
  }

  // Expand template
  return block->templ().expand(paramsResult.value());
}

StructuralStyle Renderer::get_effective_style(const Composition& composition) {
  if (composition.style_profile().has_value()) {
    return composition.style_profile()->structural;
  }
  return {};
}

}  // namespace tf
