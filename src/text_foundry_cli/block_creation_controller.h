#pragma once

#include <tf/engine.h>

#include <optional>
#include <string>

namespace tf {

class BlockCreationController {
 public:
  enum class Mode { kCreate, kEdit };

  explicit BlockCreationController(Engine& engine);

  std::string& BlockId() { return block_id_; }
  std::string& BlockTemplate() { return block_template_; }
  std::string& BlockDefaults() { return block_defaults_; }
  std::string& BlockTags() { return block_tags_; }
  std::string& BlockDescription() { return block_description_; }
  std::string& BlockLanguage() { return block_language_; }
  int& BlockTypeIndex() { return block_type_index_; }
  [[nodiscard]] const std::string& Status() const { return status_; }
  [[nodiscard]] Mode CurrentMode() const { return mode_; }
  [[nodiscard]] const std::string& EditingBlockId() const {
    return editing_block_id_;
  }
  [[nodiscard]] std::string ModalTitle() const;
  [[nodiscard]] std::string SubmitButtonLabel() const;

  void Reset(const std::string& status = "Form reset");
  void BeginCreate();
  void BeginEdit(const Block& block);
  std::optional<tf::BlockId> Create();

 private:
  Engine& engine_;

  std::string block_id_;
  std::string block_template_;
  std::string block_defaults_;
  std::string block_tags_;
  std::string block_description_;
  std::string block_language_ = "en";
  int block_type_index_ = 3;  // domain
  std::string status_ = "Fill in fields and press Create";
  Mode mode_ = Mode::kCreate;
  std::string editing_block_id_;
};

}  // namespace tf
