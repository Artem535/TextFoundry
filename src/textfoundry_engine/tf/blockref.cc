//
// Created by artem.d on 28.01.2026.
//

#include "blockref.h"

#include "block.h"

namespace tf {
BlockRef::BlockRef(BlockId blockId) : blockId_(std::move(blockId)) {}

BlockRef::BlockRef(BlockId blockId, Version version)
    : blockId_(std::move(blockId)), version_(version), useLatest_(false) {}

BlockRef::BlockRef(BlockId blockId, Version version, Params LocalParams)
    : blockId_(std::move(blockId)),
      version_(version),
      localParams_(std::move(LocalParams)),
      useLatest_(false) {}

const BlockId& BlockRef::GetBlockId() const noexcept { return blockId_; }

const std::optional<Version>& BlockRef::version() const noexcept {
  return version_;
}

const Params& BlockRef::LocalParams() const noexcept { return localParams_; }

bool BlockRef::UseLatest() const noexcept { return useLatest_; }

void BlockRef::SetBlockId(BlockId id) { blockId_ = std::move(id); }

void BlockRef::SetVersion(Version ver) {
  version_ = ver;
  useLatest_ = false;
}

void BlockRef::SetLocalParams(Params params) {
  localParams_ = std::move(params);
}

void BlockRef::SetUseLatest(const bool UseLatest) {
  useLatest_ = UseLatest;
  if (UseLatest) {
    version_.reset();
  }
}

BlockRef& BlockRef::WithParam(const std::string& name, ParamValue value) {
  localParams_[name] = std::move(value);
  return *this;
}

Error BlockRef::validate(bool is_draft_context) const {
  if (blockId_.empty()) {
    return Error{ErrorCode::InvalidParamType, "BlockRef must have a blockId"};
  }
  if (!is_draft_context && useLatest_) {
    return Error::VersionRequired();
  }
  return Error::success();
}

Result<Params> BlockRef::ResolveParams(const Block& block,
                                        const Params& runtime_context) const {
  // Start with all parameters needed by the template
  const auto paramNames = block.templ().ExtractParamNames();
  Params resolved;

  for (const auto& paramName : paramNames) {
    auto result = block.ResolveParam(paramName, localParams_, runtime_context);
    if (result.HasError()) {
      return Result<Params>(result.error());
    }
    resolved[paramName] = result.value();
  }

  return Result<Params>(resolved);
}
}  // namespace tf
