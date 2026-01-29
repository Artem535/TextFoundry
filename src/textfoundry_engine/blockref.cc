//
// Created by artem.d on 28.01.2026.
//

#include "blockref.h"

namespace tf {
  BlockRef::BlockRef(BlockId blockId) : blockId_(std::move(blockId)) {
  }

  BlockRef::BlockRef(BlockId blockId, Version version) : blockId_(std::move(blockId)), version_(version),
                                                         useLatest_(false), block_id_(std::move(blockId)) {
  }

  BlockRef::BlockRef(BlockId blockId, Version version, Params localParams): blockId_(std::move(blockId)),
                                                                            version_(version),
                                                                            localParams_(std::move(localParams)),
                                                                            useLatest_(false) {
  }

  const BlockId & BlockRef::blockId() const noexcept { return blockId_; }

  const std::optional<Version> & BlockRef::version() const noexcept { return version_; }

  const Params & BlockRef::localParams() const noexcept { return localParams_; }

  bool BlockRef::useLatest() const noexcept { return useLatest_; }

  void BlockRef::setBlockId(BlockId id) { blockId_ = std::move(id); }

  void BlockRef::setVersion(Version ver) {
    version_ = ver;
    useLatest_ = false;
  }

  void BlockRef::setLocalParams(Params params) { localParams_ = std::move(params); }

  void BlockRef::setUseLatest(bool useLatest) {
    useLatest_ = useLatest;
    if (useLatest) {
      version_.reset();
    }
  }

  BlockRef & BlockRef::withParam(const std::string &name, ParamValue value) {
    localParams_[name] = std::move(value);
    return *this;
  }

  Error BlockRef::validate(bool isDraftContext) const {
    if (blockId_.empty()) {
      return Error{ErrorCode::InvalidParamType, "BlockRef must have a blockId"};
    }
    if (!isDraftContext && useLatest_) {
      return Error::versionRequired();
    }
    return Error::success();
  }

  Result<Params> BlockRef::resolveParams(
    const Block &block,
    const Params &runtimeContext
  ) const {
    // Start with all parameters needed by the template
    const auto paramNames = block.templ().extractParamNames();
    Params resolved;

    for (const auto &paramName: paramNames) {
      auto result = block.resolveParam(paramName, localParams_, runtimeContext);
      if (result.hasError()) {
        return result.error();
      }
      resolved[paramName] = result.value();
    }

    return resolved;
  }
} // namespace tf
