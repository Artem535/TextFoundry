//
// Created by artem.d on 28.01.2026.
//

#include "blockref.h"

namespace tf {
  BlockRef::BlockRef(BlockId block_id) : blockId_(std::move(block_id)) {
  }

  BlockRef::BlockRef(BlockId blockId, Version version) : blockId_(std::move(blockId)), version_(version),
                                                         useLatest_(false) {
  }

  BlockRef::BlockRef(BlockId blockId, Version version, Params local_params): blockId_(std::move(blockId)),
                                                                            version_(version),
                                                                            localParams_(std::move(local_params)),
                                                                            useLatest_(false) {
  }

  const BlockId & BlockRef::block_id() const noexcept { return blockId_; }

  const std::optional<Version> & BlockRef::version() const noexcept { return version_; }

  const Params & BlockRef::local_params() const noexcept { return localParams_; }

  bool BlockRef::use_latest() const noexcept { return useLatest_; }

  void BlockRef::set_block_id(BlockId id) { blockId_ = std::move(id); }

  void BlockRef::set_version(Version ver) {
    version_ = ver;
    useLatest_ = false;
  }

  void BlockRef::set_local_params(Params params) { localParams_ = std::move(params); }

  void BlockRef::set_use_latest(const bool use_latest) {
    useLatest_ = use_latest;
    if (use_latest) {
      version_.reset();
    }
  }

  BlockRef & BlockRef::with_param(const std::string &name, ParamValue value) {
    localParams_[name] = std::move(value);
    return *this;
  }

  Error BlockRef::validate(bool is_draft_context) const {
    if (blockId_.empty()) {
      return Error{ErrorCode::InvalidParamType, "BlockRef must have a blockId"};
    }
    if (!is_draft_context && useLatest_) {
      return Error::version_required();
    }
    return Error::success();
  }

  Result<Params> BlockRef::resolve_params(
    const Block &block,
    const Params &runtime_context
  ) const {
    // Start with all parameters needed by the template
    const auto paramNames = block.templ().extract_param_names();
    Params resolved;

    for (const auto &paramName: paramNames) {
      auto result = block.resolve_param(paramName, localParams_, runtime_context);
      if (result.has_error()) {
        return Result<Params>(result.error());
      }
      resolved[paramName] = result.value();
    }

    return Result<Params>(resolved);
  }
} // namespace tf
