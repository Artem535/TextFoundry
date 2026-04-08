#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "block.h"
#include "error.h"

namespace tf {

/**
 * User-authored request for AI-assisted block generation.
 *
 * This is an explicit authoring operation. The engine may use the request as a
 * hint source, but generation is never applied implicitly during rendering.
 */
struct BlockGenerationRequest {
  std::string prompt;
  std::optional<BlockId> preferred_id;
  std::optional<BlockType> preferred_type;
  std::optional<std::string> preferred_language;
  std::vector<BlockId> existing_block_ids;
  bool allow_id_collision = false;
};

/**
 * User-authored request for AI-assisted prompt decomposition into blocks.
 *
 * This is an explicit authoring operation. The source prompt is treated as
 * content to decompose into reusable block candidates for later review.
 */
struct PromptSlicingRequest {
  std::string source_text;
  std::optional<std::string> preferred_language;
  std::optional<std::string> namespace_prefix;
  std::vector<BlockId> existing_block_ids;
  std::vector<BlockId> reusable_block_ids;
  bool allow_id_collision = false;
};

/**
 * Structured block suggestion returned by an external generator.
 */
struct GeneratedBlockData {
  BlockId id;
  BlockType type = BlockType::Domain;
  std::string language = "en";
  std::string description;
  std::string templ;
  Params defaults;
  std::vector<std::string> tags;
};

struct GeneratedBlockBatch {
  std::vector<GeneratedBlockData> blocks;
};

/**
 * AI-assisted block generation port.
 *
 * Implementations may use a remote LLM, local model, or deterministic stub.
 * The engine depends only on this contract, never on a concrete provider API.
 */
class IBlockGenerator {
 public:
  virtual ~IBlockGenerator() = default;

  [[nodiscard]] virtual Result<GeneratedBlockData> GenerateBlock(
      const BlockGenerationRequest& request) const = 0;

  [[nodiscard]] virtual Result<GeneratedBlockBatch> GenerateBlocks(
      const PromptSlicingRequest& request) const = 0;
};

}  // namespace tf
