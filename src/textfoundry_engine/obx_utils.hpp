//
// Created by AI Assistant on 30.01.2026.
// Utilities for converting between ObjectBox entities and domain objects
//

#pragma once

#include "database_scheme.obx.hpp"
#include "block.h"
#include "composition.h"
#include "fragment.h"
#include "blockref.h"
#include "version.h"
#include "blocktype.hpp"

#include <rfl/json.hpp>

#include <vector>
#include <unordered_set>
#include <optional>

namespace tf::utils {
  // ============================================================================
  // Enums Conversion
  // ============================================================================

  /**
   * Convert ObxBlock type (int8_t) to BlockType enum
   */
  inline BlockType obxTypeToBlockType(int8_t type) {
    // BlockType enum: Role=0, Constraint=1, Style=2, Domain=3, Meta=4
    switch (type) {
      case 0: return BlockType::Role;
      case 1: return BlockType::Constraint;
      case 2: return BlockType::Style;
      case 3: return BlockType::Domain;
      case 4: return BlockType::Meta;
      default: return BlockType::Domain; // default fallback
    }
  }

  /**
   * Convert BlockType enum to ObxBlock type (int8_t)
   */
  inline int8_t blockTypeToObxType(BlockType type) {
    switch (type) {
      case BlockType::Role: return 0;
      case BlockType::Constraint: return 1;
      case BlockType::Style: return 2;
      case BlockType::Domain: return 3;
      case BlockType::Meta: return 4;
    }
    return 3; // Domain as default
  }

  /**
   * Convert ObxState code (int8_t) to BlockState enum
   * ObxState codes: Draft=0, Published=1, Deprecated=2
   */
  inline BlockState obxStateCodeToBlockState(int8_t code) {
    switch (code) {
      case 0: return BlockState::Draft;
      case 1: return BlockState::Published;
      case 2: return BlockState::Deprecated;
      default: return BlockState::Draft;
    }
  }

  /**
   * Convert BlockState enum to ObxState code (int8_t)
   */
  inline int8_t blockStateToObxStateCode(BlockState state) {
    switch (state) {
      case BlockState::Draft: return 0;
      case BlockState::Published: return 1;
      case BlockState::Deprecated: return 2;
    }
    return 0; // Draft as default
  }

  /**
   * Convert ObxFragment separator type (int8_t) to SeparatorType enum
   * 0=None, 1=Newline, 2=Paragraph, 3=Hr
   */
  inline SeparatorType obxSeparatorTypeToSeparatorType(int8_t type) {
    switch (type) {
      case 1: return SeparatorType::Newline;
      case 2: return SeparatorType::Paragraph;
      case 3: return SeparatorType::Hr;
      default: return SeparatorType::Newline; // default fallback
    }
  }

  /**
   * Convert SeparatorType enum to ObxFragment separator type (int8_t)
   */
  inline int8_t separatorTypeToObxSeparatorType(SeparatorType type) {
    switch (type) {
      case SeparatorType::Newline: return 1;
      case SeparatorType::Paragraph: return 2;
      case SeparatorType::Hr: return 3;
    }
    return 1; // Newline as default
  }

  /**
   * Convert ObxFragment fragment type (int8_t) to FragmentType enum
   * 0=BlockRef, 1=StaticText, 2=Separator
   */
  inline FragmentType obxFragmentTypeToFragmentType(int8_t type) {
    switch (type) {
      case 0: return FragmentType::BlockRef;
      case 1: return FragmentType::StaticText;
      case 2: return FragmentType::Separator;
      default: return FragmentType::StaticText;
    }
  }

  /**
   * Convert FragmentType enum to ObxFragment fragment type (int8_t)
   */
  inline int8_t fragmentTypeToObxFragmentType(FragmentType type) {
    switch (type) {
      case FragmentType::BlockRef: return 0;
      case FragmentType::StaticText: return 1;
      case FragmentType::Separator: return 2;
    }
    return 1; // StaticText as default
  }

  // ============================================================================
  // Version Conversion
  // ============================================================================

  /**
   * Create Version from major/minor components
   */
  inline Version obxVersionToVersion(uint16_t major, uint16_t minor) {
    return Version{major, minor};
  }

  /**
   * Extract major version component from Version
   */
  inline uint16_t versionToMajor(const Version &version) {
    return version.major;
  }

  /**
   * Extract minor version component from Version
   */
  inline uint16_t versionToMinor(const Version &version) {
    return version.minor;
  }

  // ============================================================================
  // Block Conversion
  // ============================================================================

  /**
   * Convert ObxBlock to domain Block
   * Note: This creates a basic Block. Tags and ParamSchema require additional queries.
   */
  inline Block obxBlockToBlock(const ObxBlock &obxBlock) {
    Block block;
    block.setId(obxBlock.blockId);
    block.setVersion(Version{obxBlock.versionMajor, obxBlock.versionMinor});
    block.setType(obxTypeToBlockType(obxBlock.type));
    block.setState(obxStateCodeToBlockState(obxBlock.state));
    block.setTemplate(Template(obxBlock.templateContent));
    block.setDescription(obxBlock.description);

    if (!obxBlock.defaultsJson.empty()) {
      block.setDefaults(rfl::json::read<Params>(obxBlock.defaultsJson).value());
    }

    if (!obxBlock.paramsJson.empty()) {
      block.setParamSchema(rfl::json::read<std::vector<ParamSchema> >(obxBlock.paramsJson).value());
    }

    return block;
  }

  /**
   * Convert domain Block to ObxBlock for storage
   * Note: Sets all fields except relations (projectId, languageId, previousVersionId, nextVersionId)
   * which must be set separately based on context.
   */
  inline ObxBlock blockToObxBlock(const Block &block, const obx_id id = 0) {
    ObxBlock obxBlock;
    obxBlock.id = id;
    obxBlock.blockId = block.id();
    obxBlock.versionMajor = block.version().major;
    obxBlock.versionMinor = block.version().minor;
    obxBlock.type = blockTypeToObxType(block.type());
    obxBlock.state = blockStateToObxStateCode(block.state());
    obxBlock.templateContent = block.templ().content();
    obxBlock.defaultsJson = rfl::json::write(block.defaults());
    obxBlock.description = block.description();
    // Note: createdAt, updatedAt should be set by storage layer
    return obxBlock;
  }

  // ============================================================================
  // Composition Conversion
  // ============================================================================

  /**
   * Convert ObxComposition to domain Composition
   * Note: Fragments must be loaded and converted separately.
   */
  inline Composition obxCompositionToComposition(const ObxComposition &obxComp) {
    Composition comp;
    comp.setId(obxComp.compositionId);
    comp.setVersion(Version{obxComp.versionMajor, obxComp.versionMinor});
    comp.setProjectKey(""); // Must be resolved from project relation
    comp.setDescription(obxComp.description);
    comp.setState(obxStateCodeToBlockState(obxComp.state));

    if (!obxComp.styleProfileJson.empty()) {
      comp.setStyleProfile(rfl::json::read<StyleProfile>(obxComp.styleProfileJson).value());
    }

    return comp;
  }

  /**
   * Convert domain Composition to ObxComposition for storage
   * Note: Sets basic fields. Relations (projectId, targetLanguageId,
   * previousVersionId, nextVersionId) must be set separately.
   */
  inline ObxComposition compositionToObxComposition(const Composition &comp, obx_id id = 0) {
    ObxComposition obxComp;
    obxComp.id = id;
    obxComp.compositionId = comp.id();
    obxComp.versionMajor = comp.version().major;
    obxComp.versionMinor = comp.version().minor;
    obxComp.description = comp.description();
    if (comp.styleProfile().has_value()) {
      obxComp.styleProfileJson = rfl::json::write(comp.styleProfile());
    }
    return obxComp;
  }

  // ============================================================================
  // Fragment Conversion
  // ============================================================================

  /**
   * Convert ObxFragment to domain Fragment
   */
  inline Fragment obxFragmentToFragment(const ObxFragment &obxFrag) {
    FragmentType fragType = obxFragmentTypeToFragmentType(obxFrag.fragmentType);

    switch (fragType) {
      case FragmentType::BlockRef: {
        BlockRef ref;
        ref.setBlockId(obxFrag.refBlockId);
        if (!obxFrag.refUseLatest) {
          ref.setVersion(Version{obxFrag.refVersionMajor, obxFrag.refVersionMinor});
        }
        ref.setUseLatest(obxFrag.refUseLatest);
        if (!obxFrag.refLocalParamsJson.empty()) {
          ref.setLocalParams(rfl::json::read<Params>(obxFrag.refLocalParamsJson).value());
        }
        return Fragment::makeBlockRef(std::move(ref));
      }

      case FragmentType::StaticText: {
        return Fragment::makeStaticText(obxFrag.staticContent);
      }

      case FragmentType::Separator: {
        SeparatorType sepType = obxSeparatorTypeToSeparatorType(obxFrag.separatorType);
        return Fragment::makeSeparator(sepType);
      }
    }

    // Fallback
    return Fragment::makeStaticText("");
  }

  /**
   * Convert domain Fragment to ObxFragment for storage
   * Note: compositionId must be set separately
   */
  inline ObxFragment fragmentToObxFragment(const Fragment &fragment, uint32_t orderIndex, obx_id id = 0) {
    ObxFragment obxFrag;
    obxFrag.id = id;
    obxFrag.orderIndex = orderIndex;
    obxFrag.fragmentType = fragmentTypeToObxFragmentType(fragment.type());

    switch (fragment.type()) {
      case FragmentType::BlockRef: {
        const BlockRef &ref = fragment.asBlockRef();
        obxFrag.refBlockId = ref.blockId();
        obxFrag.refUseLatest = ref.useLatest();
        if (ref.version().has_value()) {
          obxFrag.refVersionMajor = ref.version().value().major;
          obxFrag.refVersionMinor = ref.version().value().minor;
        } else {
          obxFrag.refVersionMajor = 0;
          obxFrag.refVersionMinor = 0;
        }
        // Note: localParams should be serialized to refLocalParamsJson
        obxFrag.refLocalParamsJson = rfl::json::write(ref.localParams());
        break;
      }

      case FragmentType::StaticText: {
        obxFrag.staticContent = fragment.asStaticText().text();
        break;
      }

      case FragmentType::Separator: {
        obxFrag.separatorType = separatorTypeToObxSeparatorType(fragment.asSeparator().type);
        break;
      }
    }

    return obxFrag;
  }

  // ============================================================================
  // Project Conversion
  // ============================================================================

  /**
   * Convert ObxProject project entity
   * Note: Returns project key for use in domain objects
   */
  inline std::string obxProjectToProjectKey(const ObxProject &obxProject) {
    return obxProject.key;
  }

  /**
   * Create ObxProject for storage
   */
  inline ObxProject projectKeyToObxProject(const std::string &key,
                                           const std::string &name = "",
                                           const std::string &description = "") {
    ObxProject obxProject;
    obxProject.key = key;
    obxProject.name = name.empty() ? key : name;
    obxProject.description = description;
    // Note: createdAt, updatedAt should be set by storage layer
    return obxProject;
  }

  // ============================================================================
  // Language Conversion
  // ============================================================================

  /**
   * Convert ObxLanguage to language code string
   */
  inline std::string obxLanguageToCode(const ObxLanguage &obxLang) {
    return obxLang.code;
  }

  /**
   * Create ObxLanguage for storage
   */
  inline ObxLanguage codeToObxLanguage(const std::string &code,
                                       const std::string &nativeName = "") {
    ObxLanguage obxLang;
    obxLang.code = code;
    obxLang.nativeName = nativeName.empty() ? code : nativeName;
    return obxLang;
  }

  // ============================================================================
  // Tag Conversion
  // ============================================================================

  /**
   * Convert ObxTag to tag name string
   */
  inline std::string obxTagToTagName(const ObxTag &obxTag) {
    return obxTag.name;
  }

  /**
   * Create ObxTag for storage
   * Note: projectId must be set separately
   */
  inline ObxTag tagNameToObxTag(const std::string &name) {
    ObxTag obxTag;
    obxTag.name = name;
    // Note: createdAt should be set by storage layer
    return obxTag;
  }

  /**
   * Convert vector of ObxTag to unordered_set of tag names
   */
  inline std::unordered_set<std::string> obxTagsToTagSet(const std::vector<ObxTag> &obxTags) {
    std::unordered_set<std::string> tags;
    for (const auto &obxTag: obxTags) {
      tags.insert(obxTag.name);
    }
    return tags;
  }

  // ============================================================================
  // Audit Log Conversion (optional)
  // ============================================================================

  /**
   * Create ObxAuditLog entry
   */
  inline ObxAuditLog createObxAuditLog(const std::string &entityType,
                                       uint64_t entityId,
                                       const std::string &operation,
                                       const std::string &userId = "",
                                       const std::string &detailsJson = "") {
    ObxAuditLog log;
    log.entityType = entityType;
    log.entityId = entityId;
    log.operation = operation;
    log.userId = userId;
    log.detailsJson = detailsJson;
    // Note: createdAt should be set by storage layer
    return log;
  }
} // namespace tf
