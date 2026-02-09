//
// Created by artem.d on 31.01.2026.
//

#include "repository_block_cache.h"

namespace tf {

const Block* RepositoryBlockCache::get_block(const BlockId& id,
                                             Version version) const {
  auto key = std::make_pair(id, version);
  auto it = cache_.find(key);
  if (it != cache_.end()) {
    return &it->second;
  }

  // Cache miss - load from repository
  if (!repo_) {
    return nullptr;
  }

  auto result = repo_->load(id, version);
  if (result.has_error()) {
    return nullptr;
  }

  auto [inserted, _] = cache_.emplace(key, std::move(result.value()));
  return &inserted->second;
}

const Block* RepositoryBlockCache::get_latest_block(const BlockId& id) const {
  const auto it = latestCache_.find(id);
  if (it != latestCache_.end()) {
    return &it->second;
  }

  // Cache miss - load from repository
  if (!repo_) {
    return nullptr;
  }

  auto result = repo_->load_latest(id);
  if (result.has_error()) {
    return nullptr;
  }

  auto [inserted, _] = latestCache_.emplace(id, std::move(result.value()));
  return &inserted->second;
}

void RepositoryBlockCache::clear() {
  cache_.clear();
  latestCache_.clear();
}

}  // namespace tf
