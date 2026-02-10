#pragma once

#include <tf/engine.h>

#include <optional>
#include <string>

namespace tf {

/**
 * @brief UI-facing controller for block create/edit modal workflow.
 *
 * Encapsulates form fields, validation, and publishing through Engine.
 */
class BlockCreationController {
 public:
  /**
   * @brief Active modal workflow mode.
   */
  enum class Mode { kCreate, kEdit };

  /**
   * @brief Construct controller bound to engine instance.
   * @param engine Engine used for publish/update operations.
   */
  explicit BlockCreationController(Engine& engine);

  /// @name Mutable form field accessors (bound to FTXUI inputs)
  /// @{
  std::string& BlockId() { return block_id_; }
  std::string& BlockTemplate() { return block_template_; }
  std::string& BlockDefaults() { return block_defaults_; }
  std::string& BlockTags() { return block_tags_; }
  std::string& BlockDescription() { return block_description_; }
  std::string& BlockLanguage() { return block_language_; }
  int& BlockTypeIndex() { return block_type_index_; }
  /// @}

  /**
   * @brief Current status line shown in modal footer.
   */
  [[nodiscard]] const std::string& Status() const { return status_; }
  /**
   * @brief Current workflow mode.
   */
  [[nodiscard]] Mode CurrentMode() const { return mode_; }
  /**
   * @brief Block ID being edited in @c Mode::kEdit.
   */
  [[nodiscard]] const std::string& EditingBlockId() const {
    return editing_block_id_;
  }
  /**
   * @brief Modal header text for current mode.
   */
  [[nodiscard]] std::string ModalTitle() const;
  /**
   * @brief Primary action label for current mode.
   */
  [[nodiscard]] std::string SubmitButtonLabel() const;

  /**
   * @brief Reset form values.
   * @param status Status message after reset.
   *
   * In edit mode original block ID stays immutable.
   */
  void Reset(const std::string& status = "Form reset");
  /**
   * @brief Enter create workflow and clear form.
   */
  void BeginCreate();
  /**
   * @brief Enter edit workflow with fields prefilled from existing block.
   * @param block Existing block snapshot used for initial form values.
   */
  void BeginEdit(const Block& block);
  /**
   * @brief Publish current form.
   * @return Published block ID on success, std::nullopt on validation/publish
   * failure.
   *
   * Uses publish-new in create mode and publish-next-version in edit mode.
   */
  std::optional<tf::BlockId> Create();

 private:
  Engine& engine_;  ///< Engine backend for persistence/versioning.

  std::string block_id_;           ///< Raw ID input field.
  std::string block_template_;     ///< Raw template content field.
  std::string block_defaults_;     ///< Raw defaults in key=value CSV format.
  std::string block_tags_;         ///< Raw tags in comma-separated format.
  std::string block_description_;  ///< Optional block description.
  std::string block_language_ = "en";  ///< Language code field.
  int block_type_index_ = 3;           ///< Selected type index, default domain.
  std::string status_ = "Fill in fields and press Create";  ///< Status message.
  Mode mode_ = Mode::kCreate;     ///< Active workflow mode.
  std::string editing_block_id_;  ///< Stable edited ID for mode kEdit.
};

}  // namespace tf
