//
// Created by a.durynin on 29.01.2026.
//

#pragma once

#include "composition.h"
#include "block.h"
#include "error.h"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace tf {

/**
 * RenderResult - result of rendering a composition
 */
struct RenderResult {
    std::string text;                    ///< Final rendered text
    CompositionId compositionId;         ///< Source composition ID
    Version compositionVersion;          ///< Source composition version
    std::vector<std::pair<BlockId, Version>> blocksUsed;  ///< Blocks used in rendering

    [[nodiscard]] bool isEmpty() const noexcept;
};

/**
 * BlockCache - interface for block lookup during rendering
 * Abstracts storage access for stateless Renderer
 */
class IBlockCache {
public:
    virtual ~IBlockCache() = default;

    /**
     * Get block by ID and version
     * @returns Block or nullptr if not found
     */
    [[nodiscard]] virtual const Block* getBlock(const BlockId& id, Version version) const = 0;

    /**
     * Get latest published version of a block
     * @returns Block or nullptr if not found
     */
    [[nodiscard]] virtual const Block* getLatestBlock(const BlockId& id) const = 0;

    /**
     * Clear all cached blocks
     * Called before each render operation to ensure fresh data
     */
    virtual void clear() = 0;
};

/**
 * Renderer - stateless service for transforming Composition to text
 *
 * Processing stages:
 * 1. Template Expansion - substitute parameters through hierarchy
 * 2. StructuralStyle Application - apply wrappers, delimiters, formats
 * 3. Output Generation - produce final text without semantic modification
 *
 * Renderer is stateless - works only with passed snapshots
 */
class Renderer {
public:
    Renderer() = default;
    explicit Renderer(std::unique_ptr<IBlockCache> cache);

    /**
     * Set block cache for block resolution
     */
    void setBlockCache(std::unique_ptr<IBlockCache> cache);

    /**
     * Clear the block cache
     * Called before each render operation to ensure fresh data
     */
    void clearCache();

    /**
     * Render composition to text
     *
     * @param composition Composition to render (must be Published)
     * @param context Runtime parameters
     * @returns RenderResult or Error
     */
    [[nodiscard]] Result<RenderResult> render(
        const Composition& composition,
        const RenderContext& context = RenderContext{}
    ) const;

    /**
     * Render a single block (useful for testing/block preview)
     *
     * @param block Block to render
     * @param context Runtime parameters
     * @returns Rendered string or Error
     */
    [[nodiscard]] static Result<std::string> renderBlock(
        const Block& block,
        const RenderContext& context = RenderContext{}
    );

    /**
     * Apply structural style to rendered fragments
     */
    [[nodiscard]] static std::string applyStructuralStyle(
        const std::vector<std::string>& fragmentTexts,
        const StructuralStyle& style
    ) ;

private:
    std::unique_ptr<IBlockCache> blockCache_;

    /**
     * Render a single fragment
     */
    [[nodiscard]] Result<std::string> renderFragment(
        const Fragment& fragment,
        const RenderContext& context,
        std::vector<std::pair<BlockId, Version>>& blocksUsed
    ) const;

    /**
     * Resolve and expand a BlockRef
     */
    [[nodiscard]] Result<std::string> expandBlockRef(
        const BlockRef& blockRef,
        const RenderContext& context,
        std::vector<std::pair<BlockId, Version>>& blocksUsed
    ) const;

    /**
     * Get structural style from composition or default
     */
    [[nodiscard]] static StructuralStyle getEffectiveStyle(const Composition& composition);
};

} // namespace tf
