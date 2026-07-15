#include <nanobind/nanobind.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unordered_map.h>
#include <nanobind/stl/vector.h>

#include <chrono>
#include <new>

#include "openai_compatible_block_generator.h"
#include "openai_compatible_composition_block_rewriter.h"
#include "openai_compatible_normalizer.h"
#include "qt_http_transport.h"
#include "tf/block.h"
#include "tf/block_type.hpp"
#include "tf/composition.h"
#include "tf/engine.h"
#include "tf/error.h"
#include "tf/renderer.h"
#include "tf/types.h"
#include "tf/version.h"

#ifdef TEXTFOUNDRY_BUILD_TESTING
class FakeTransport final : public tf::ai::IHttpTransport {
 public:
  tf::ai::HttpResponse response;
  mutable tf::ai::HttpRequest last_request;
  tf::Result<tf::ai::HttpResponse> PostJson(
      const tf::ai::HttpRequest& request) const override {
    last_request = request;
    return tf::Result<tf::ai::HttpResponse>(response);
  }
};
#endif

namespace nb = nanobind;
using namespace tf;

class PyError : public std::runtime_error {
 public:
  explicit PyError(const tf::Error& e)
      : std::runtime_error(e.message), code(e.code), message(e.message) {}
  ErrorCode code;
  std::string message;
};

template <typename T>
T unwrap(Result<T> result) {
  if (result.HasError()) throw PyError(result.error());
  return std::move(result).value();
}

void raise_error(const Error& error) {
  if (error.is_error()) throw PyError(error);
}

NB_MODULE(textfoundry, m) {
  m.attr("__version__") = "0.2.7";
  nb::object error =
      nb::module_::import_("builtins")
          .attr("type")("Error",
                        nb::make_tuple(
                            nb::module_::import_("builtins").attr("Exception")),
                        nb::dict());
  m.attr("Error") = error;
  error.attr("__module__") = "textfoundry";
  nb::register_exception_translator([](const std::exception_ptr& p, void*) {
    try {
      if (p) std::rethrow_exception(p);
    } catch (const PyError& e) {
      nb::object cls = nb::module_::import_("textfoundry").attr("Error");
      nb::object instance = cls(e.message);
      instance.attr("code") = nb::cast(e.code);
      instance.attr("message") = e.message;
      PyErr_SetObject(cls.ptr(), instance.ptr());
    }
  });

  nb::enum_<ErrorCode>(m, "ErrorCode")
      .value("MissingParam", ErrorCode::MissingParam)
      .value("InvalidParamType", ErrorCode::InvalidParamType)
      .value("UnknownParam", ErrorCode::UnknownParam)
      .value("VersionRequired", ErrorCode::VersionRequired)
      .value("VersionNotFound", ErrorCode::VersionNotFound)
      .value("InvalidVersion", ErrorCode::InvalidVersion)
      .value("BlockNotFound", ErrorCode::BlockNotFound)
      .value("CompositionNotFound", ErrorCode::CompositionNotFound)
      .value("DuplicateId", ErrorCode::DuplicateId)
      .value("InvalidStateTransition", ErrorCode::InvalidStateTransition)
      .value("DraftRequired", ErrorCode::DraftRequired)
      .value("PublishedRequired", ErrorCode::PublishedRequired)
      .value("TemplateSyntaxError", ErrorCode::TemplateSyntaxError)
      .value("CircularReference", ErrorCode::CircularReference)
      .value("StorageError", ErrorCode::StorageError)
      .value("Success", ErrorCode::Success);
  nb::class_<Version>(m, "Version")
      .def(nb::init<uint16_t, uint16_t>(), nb::arg("major") = 0,
           nb::arg("minor") = 0)
      .def_rw("major", &Version::major)
      .def_rw("minor", &Version::minor)
      .def("to_string", &Version::ToString);
  nb::enum_<BlockType>(m, "BlockType")
      .value("Role", BlockType::Role)
      .value("System", BlockType::System)
      .value("Mission", BlockType::Mission)
      .value("Safety", BlockType::Safety)
      .value("Constraint", BlockType::Constraint)
      .value("Style", BlockType::Style)
      .value("Domain", BlockType::Domain)
      .value("Meta", BlockType::Meta);
  nb::enum_<BlockState>(m, "BlockState")
      .value("Draft", BlockState::Draft)
      .value("Published", BlockState::Published)
      .value("Deprecated", BlockState::Deprecated);
  nb::enum_<SeparatorType>(m, "SeparatorType")
      .value("Newline", SeparatorType::Newline)
      .value("Paragraph", SeparatorType::Paragraph)
      .value("Hr", SeparatorType::Hr);
  nb::class_<ParamSchema>(m, "ParamSchema")
      .def(nb::init<>())
      .def_rw("name", &ParamSchema::name)
      .def_rw("required", &ParamSchema::required)
      .def_rw("default_value", &ParamSchema::defaultValue);
  nb::class_<Template>(m, "Template")
      .def(nb::init<std::string>())
      .def_prop_rw("content", &Template::Content, &Template::SetContent)
      .def("extract_param_names", &Template::ExtractParamNames)
      .def("expand",
           [](const Template& t, Params p) { return unwrap(t.Expand(p)); });
  nb::class_<RenderContext>(m, "RenderContext")
      .def(nb::init<>())
      .def_rw("params", &RenderContext::params)
      .def_rw("target_language", &RenderContext::targetLanguage)
      .def_rw("strict_mode", &RenderContext::strictMode)
      .def("with_param", &RenderContext::WithParam, nb::arg("name"),
           nb::arg("value"), nb::rv_policy::reference_internal)
      .def("with_language", &RenderContext::WithLanguage,
           nb::rv_policy::reference_internal)
      .def("with_strict_mode", &RenderContext::with_strict_mode,
           nb::rv_policy::reference_internal);
  nb::class_<StructuralStyle>(m, "StructuralStyle")
      .def(nb::init<>())
      .def_rw("block_wrapper", &StructuralStyle::blockWrapper)
      .def_rw("preamble", &StructuralStyle::preamble)
      .def_rw("postamble", &StructuralStyle::postamble)
      .def_rw("delimiter", &StructuralStyle::delimiter);
  nb::class_<SemanticStyle>(m, "SemanticStyle")
      .def(nb::init<>())
      .def_rw("tone", &SemanticStyle::tone)
      .def_rw("tense", &SemanticStyle::tense)
      .def_rw("target_language", &SemanticStyle::targetLanguage)
      .def_rw("person", &SemanticStyle::person)
      .def_rw("rewrite_strength", &SemanticStyle::rewriteStrength)
      .def_rw("audience", &SemanticStyle::audience)
      .def_rw("locale", &SemanticStyle::locale)
      .def_rw("terminology_rigidity", &SemanticStyle::terminologyRigidity)
      .def_rw("preserve_formatting", &SemanticStyle::preserveFormatting)
      .def_rw("preserve_examples", &SemanticStyle::preserveExamples)
      .def("is_empty", &SemanticStyle::isEmpty);
  nb::class_<StyleProfile>(m, "StyleProfile")
      .def(nb::init<>())
      .def_rw("structural", &StyleProfile::structural)
      .def_rw("semantic", &StyleProfile::semantic)
      .def_static("plain", &StyleProfile::plain);
  nb::class_<BlockDraftBuilder>(m, "BlockDraftBuilder")
      .def(nb::init<BlockId>())
      .def("with_id", &BlockDraftBuilder::WithId,
           nb::rv_policy::reference_internal)
      .def("with_type", &BlockDraftBuilder::WithType,
           nb::rv_policy::reference_internal)
      .def("with_template", &BlockDraftBuilder::WithTemplate,
           nb::rv_policy::reference_internal)
      .def("with_default", &BlockDraftBuilder::WithDefault,
           nb::rv_policy::reference_internal)
      .def("with_defaults", &BlockDraftBuilder::WithDefaults,
           nb::rv_policy::reference_internal)
      .def("with_param_schema", &BlockDraftBuilder::WithParamSchema,
           nb::rv_policy::reference_internal)
      .def("with_tag", &BlockDraftBuilder::WithTag,
           nb::rv_policy::reference_internal)
      .def("with_language", &BlockDraftBuilder::WithLanguage,
           nb::rv_policy::reference_internal)
      .def("with_description", &BlockDraftBuilder::WithDescription,
           nb::rv_policy::reference_internal)
      .def("with_revision_comment", &BlockDraftBuilder::WithRevisionComment,
           nb::rv_policy::reference_internal)
      .def("build", &BlockDraftBuilder::build);
  nb::class_<PublishedBlock>(m, "PublishedBlock")
      .def_prop_ro("id", &PublishedBlock::id)
      .def_prop_ro("version", &PublishedBlock::version);
  nb::class_<PublishedComposition>(m, "PublishedComposition")
      .def_prop_ro("id", &PublishedComposition::id)
      .def_prop_ro("version", &PublishedComposition::version);
  nb::class_<CompositionDraftBuilder>(m, "CompositionDraftBuilder")
      .def(nb::init<>())
      .def(nb::init<CompositionId>())
      .def("with_id", &CompositionDraftBuilder::WithId,
           nb::rv_policy::reference_internal)
      .def("with_style_profile", &CompositionDraftBuilder::WithStyleProfile,
           nb::rv_policy::reference_internal)
      .def("with_project_key", &CompositionDraftBuilder::WithProjectKey,
           nb::rv_policy::reference_internal)
      .def("with_description", &CompositionDraftBuilder::WithDescription,
           nb::rv_policy::reference_internal)
      .def("with_revision_comment",
           &CompositionDraftBuilder::WithRevisionComment,
           nb::rv_policy::reference_internal)
      .def("add_static_text", &CompositionDraftBuilder::AddStaticText,
           nb::rv_policy::reference_internal)
      .def("add_separator", &CompositionDraftBuilder::AddSeparator,
           nb::rv_policy::reference_internal)
      .def("add_block_ref",
           nb::overload_cast<const BlockId&, uint16_t, uint16_t, Params>(
               &CompositionDraftBuilder::AddBlockRef),
           nb::arg("id"), nb::arg("major"), nb::arg("minor"),
           nb::arg("params") = Params{}, nb::rv_policy::reference_internal)
      .def("build", &CompositionDraftBuilder::build);
  nb::class_<BlockDraft>(m, "BlockDraft");
  nb::class_<Block>(m, "Block")
      .def(nb::init<BlockId>())
      .def_prop_ro("id", &Block::Id)
      .def_prop_ro("type", &Block::type)
      .def_prop_ro("state", &Block::state)
      .def_prop_ro("version", &Block::version)
      .def_prop_ro("templ", &Block::templ)
      .def_prop_ro("defaults", &Block::defaults)
      .def_prop_ro("language", &Block::language)
      .def_prop_ro("description", &Block::description);
  nb::class_<CompositionDraft>(m, "CompositionDraft");

  nb::class_<EngineConfig>(m, "EngineConfig")
      .def(nb::init<>())
      .def_rw("project_key", &EngineConfig::ProjectKey)
      .def_rw("strict_mode", &EngineConfig::strict_mode)
      .def_rw("default_data_path", &EngineConfig::default_data_path);

  nb::class_<RenderResult>(m, "RenderResult")
      .def_rw("text", &RenderResult::text)
      .def_rw("composition_id", &RenderResult::compositionId)
      .def_rw("composition_version", &RenderResult::compositionVersion)
      .def_rw("blocks_used", &RenderResult::blocksUsed)
      .def("is_empty", &RenderResult::isEmpty);

  using namespace tf::ai;
  nb::class_<HttpResponse>(m, "HttpResponse")
      .def(nb::init<>())
      .def_rw("status_code", &HttpResponse::status_code)
      .def_rw("body", &HttpResponse::body);
  nb::class_<OpenAiCompatibleConfig>(m, "OpenAiCompatibleConfig")
      .def(nb::init<>())
      .def_rw("base_url", &OpenAiCompatibleConfig::base_url)
      .def_rw("model", &OpenAiCompatibleConfig::model)
      .def_rw("api_key", &OpenAiCompatibleConfig::api_key)
      .def_rw("organization", &OpenAiCompatibleConfig::organization)
      .def_rw("endpoint_path", &OpenAiCompatibleConfig::endpoint_path)
      .def("__repr__", [](const OpenAiCompatibleConfig& c) {
        return "OpenAiCompatibleConfig(base_url='" + c.base_url + "', model='" +
               c.model + "', api_key='***', endpoint_path='" + c.endpoint_path +
               "')";
      });

  nb::class_<BlockGenerationRequest>(m, "BlockGenerationRequest")
      .def(nb::init<>())
      .def_rw("prompt", &BlockGenerationRequest::prompt)
      .def_rw("preferred_id", &BlockGenerationRequest::preferred_id)
      .def_rw("preferred_type", &BlockGenerationRequest::preferred_type)
      .def_rw("preferred_language", &BlockGenerationRequest::preferred_language)
      .def_rw("existing_block_ids", &BlockGenerationRequest::existing_block_ids)
      .def_rw("allow_id_collision",
              &BlockGenerationRequest::allow_id_collision);
  nb::class_<PromptSlicingRequest>(m, "PromptSlicingRequest")
      .def(nb::init<>())
      .def_rw("source_text", &PromptSlicingRequest::source_text)
      .def_rw("preferred_language", &PromptSlicingRequest::preferred_language)
      .def_rw("namespace_prefix", &PromptSlicingRequest::namespace_prefix)
      .def_rw("existing_block_ids", &PromptSlicingRequest::existing_block_ids)
      .def_rw("reusable_block_ids", &PromptSlicingRequest::reusable_block_ids)
      .def_rw("reusable_block_summaries",
              &PromptSlicingRequest::reusable_block_summaries)
      .def_rw("preserve_reuse_percent",
              &PromptSlicingRequest::preserve_reuse_percent)
      .def_rw("preserve_order", &PromptSlicingRequest::preserve_order)
      .def_rw("allow_id_collision", &PromptSlicingRequest::allow_id_collision);
  nb::class_<GeneratedBlockData>(m, "GeneratedBlockData")
      .def(nb::init<>())
      .def_rw("id", &GeneratedBlockData::id)
      .def_rw("type", &GeneratedBlockData::type)
      .def_rw("language", &GeneratedBlockData::language)
      .def_rw("description", &GeneratedBlockData::description)
      .def_rw("templ", &GeneratedBlockData::templ)
      .def_rw("defaults", &GeneratedBlockData::defaults)
      .def_rw("tags", &GeneratedBlockData::tags);
  nb::class_<GeneratedBlockBatch>(m, "GeneratedBlockBatch")
      .def(nb::init<>())
      .def_rw("blocks", &GeneratedBlockBatch::blocks);
  nb::class_<BlockNormalizationRequest>(m, "BlockNormalizationRequest")
      .def(nb::init<>())
      .def_rw("source_block", &BlockNormalizationRequest::source_block)
      .def_rw("style", &BlockNormalizationRequest::style);
  nb::class_<NormalizedBlockData>(m, "NormalizedBlockData")
      .def(nb::init<>())
      .def_rw("templ", &NormalizedBlockData::templ)
      .def_rw("description", &NormalizedBlockData::description)
      .def_rw("language", &NormalizedBlockData::language);
  nb::class_<CompositionNormalizationRequest>(m,
                                              "CompositionNormalizationRequest")
      .def(nb::init<>())
      .def_rw("source_composition_id",
              &CompositionNormalizationRequest::source_composition_id)
      .def_rw("source_version",
              &CompositionNormalizationRequest::source_version)
      .def_rw("style", &CompositionNormalizationRequest::style)
      .def_rw("target_composition_id",
              &CompositionNormalizationRequest::target_composition_id)
      .def_rw("normalize_static_text",
              &CompositionNormalizationRequest::normalize_static_text)
      .def_rw("reuse_cached_blocks",
              &CompositionNormalizationRequest::reuse_cached_blocks);
  nb::class_<NormalizedCompositionResult>(m, "NormalizedCompositionResult")
      .def(nb::init<>())
      .def_rw("composition_id", &NormalizedCompositionResult::composition_id)
      .def_rw("composition_version",
              &NormalizedCompositionResult::composition_version)
      .def_rw("rewritten_blocks",
              &NormalizedCompositionResult::rewritten_blocks);
  nb::class_<NormalizedCompositionPreview>(m, "NormalizedCompositionPreview")
      .def(nb::init<>())
      .def_rw("composition_id", &NormalizedCompositionPreview::composition_id)
      .def_rw("preview_text", &NormalizedCompositionPreview::preview_text)
      .def_rw("rewritten_blocks",
              &NormalizedCompositionPreview::rewritten_blocks);
  nb::class_<CompositionBlockRewriteRequest>(m,
                                             "CompositionBlockRewriteRequest")
      .def(nb::init<>())
      .def_rw("source_composition_id",
              &CompositionBlockRewriteRequest::source_composition_id)
      .def_rw("source_version", &CompositionBlockRewriteRequest::source_version)
      .def_rw("instruction", &CompositionBlockRewriteRequest::instruction)
      .def_rw("preserve_language",
              &CompositionBlockRewriteRequest::preserve_language)
      .def_rw("preserve_placeholders",
              &CompositionBlockRewriteRequest::preserve_placeholders);
  nb::class_<CompositionRewriteContextBlock>(m,
                                             "CompositionRewriteContextBlock")
      .def(nb::init<>())
      .def_rw("block_id", &CompositionRewriteContextBlock::block_id)
      .def_rw("type", &CompositionRewriteContextBlock::type)
      .def_rw("language", &CompositionRewriteContextBlock::language)
      .def_rw("description", &CompositionRewriteContextBlock::description)
      .def_rw("defaults", &CompositionRewriteContextBlock::defaults)
      .def_rw("tags", &CompositionRewriteContextBlock::tags)
      .def_rw("templ", &CompositionRewriteContextBlock::templ);
  nb::class_<CompositionBlockRewriteContext>(m,
                                             "CompositionBlockRewriteContext")
      .def(nb::init<>())
      .def_rw("source_composition_id",
              &CompositionBlockRewriteContext::source_composition_id)
      .def_rw("source_version", &CompositionBlockRewriteContext::source_version)
      .def_rw("instruction", &CompositionBlockRewriteContext::instruction)
      .def_rw("preserve_language",
              &CompositionBlockRewriteContext::preserve_language)
      .def_rw("preserve_placeholders",
              &CompositionBlockRewriteContext::preserve_placeholders)
      .def_rw("blocks", &CompositionBlockRewriteContext::blocks);
  nb::class_<BlockRewritePatch>(m, "BlockRewritePatch")
      .def(nb::init<>())
      .def_rw("block_id", &BlockRewritePatch::block_id)
      .def_rw("description", &BlockRewritePatch::description)
      .def_rw("templ", &BlockRewritePatch::templ)
      .def_rw("defaults", &BlockRewritePatch::defaults)
      .def_rw("tags", &BlockRewritePatch::tags)
      .def_rw("rationale", &BlockRewritePatch::rationale);
  nb::class_<CompositionBlockRewritePreview>(m,
                                             "CompositionBlockRewritePreview")
      .def(nb::init<>())
      .def_rw("source_composition_id",
              &CompositionBlockRewritePreview::source_composition_id)
      .def_rw("source_version", &CompositionBlockRewritePreview::source_version)
      .def_rw("patches", &CompositionBlockRewritePreview::patches);
  nb::class_<AppliedCompositionBlockRewriteResult>(
      m, "AppliedCompositionBlockRewriteResult")
      .def(nb::init<>())
      .def_rw("composition_id",
              &AppliedCompositionBlockRewriteResult::composition_id)
      .def_rw("composition_version",
              &AppliedCompositionBlockRewriteResult::composition_version)
      .def_rw("rewritten_blocks",
              &AppliedCompositionBlockRewriteResult::rewritten_blocks);

  auto context_from_object = [](nb::handle obj) {
    if (obj.is_none()) return RenderContext{};
    if (nb::isinstance<RenderContext>(obj)) return nb::cast<RenderContext>(obj);
    RenderContext context;
    context.params = nb::cast<Params>(obj);
    return context;
  };

  nb::class_<Engine>(m, "Engine")
      .def(
          "__init__",
          [](Engine* self, std::string data_path) {
            EngineConfig config;
            config.default_data_path = std::move(data_path);
            new (self) Engine(std::move(config));
          },
          nb::arg("data_path") = "memory:tf")
      .def("__init__",
           [](Engine* self, EngineConfig config) {
             new (self) Engine(std::move(config));
           })
      .def(
          "publish_block",
          [](Engine& e, BlockDraft&& draft, std::optional<Version> version) {
            if (version)
              return unwrap(e.PublishBlock(std::move(draft), *version));
            return unwrap(
                e.PublishBlock(std::move(draft), Engine::VersionBump::Minor));
          },
          nb::arg("draft"), nb::arg("version") = nb::none())
      .def(
          "update_block",
          [](Engine& e, BlockDraft&& draft) {
            return unwrap(
                e.UpdateBlock(std::move(draft), Engine::VersionBump::Minor));
          },
          nb::arg("draft"))
      .def(
          "load_block",
          [](Engine& e, const BlockId& id, std::optional<Version> version) {
            return version ? unwrap(e.LoadBlock(id, *version))
                           : unwrap(e.LoadBlock(id));
          },
          nb::arg("id"), nb::arg("version") = nb::none())
      .def("get_latest_block_version",
           [](Engine& e, const BlockId& id) {
             return unwrap(e.GetLatestBlockVersion(id));
           })
      .def("list_block_versions",
           [](Engine& e, const BlockId& id) {
             return unwrap(e.ListBlockVersions(id));
           })
      .def("list_blocks", [](Engine& e) { return e.ListBlocks(); })
      .def("delete_block",
           [](Engine& e, const BlockId& id) { raise_error(e.DeleteBlock(id)); })
      .def("deprecate_block",
           [](Engine& e, const BlockId& id, Version version) {
             raise_error(e.DeprecateBlock(id, version));
           })
      .def(
          "publish_composition",
          [](Engine& e, CompositionDraft&& draft,
             std::optional<Version> version) {
            if (version)
              return unwrap(e.PublishComposition(std::move(draft), *version));
            return unwrap(e.PublishComposition(std::move(draft),
                                               Engine::VersionBump::Minor));
          },
          nb::arg("draft"), nb::arg("version") = nb::none())
      .def(
          "update_composition",
          [](Engine& e, CompositionDraft&& draft) {
            return unwrap(e.UpdateComposition(std::move(draft),
                                              Engine::VersionBump::Minor));
          },
          nb::arg("draft"))
      .def(
          "load_composition",
          [](Engine& e, const CompositionId& id,
             std::optional<Version> version) {
            return version ? unwrap(e.LoadComposition(id, *version))
                           : unwrap(e.LoadComposition(id));
          },
          nb::arg("id"), nb::arg("version") = nb::none())
      .def("get_latest_composition_version",
           [](Engine& e, const CompositionId& id) {
             return unwrap(e.GetLatestCompositionVersion(id));
           })
      .def("list_composition_versions",
           [](Engine& e, const CompositionId& id) {
             return unwrap(e.ListCompositionVersions(id));
           })
      .def("list_compositions", &Engine::ListCompositions)
      .def("delete_composition",
           [](Engine& e, const CompositionId& id) {
             raise_error(e.DeleteComposition(id));
           })
      .def("deprecate_composition",
           [](Engine& e, const CompositionId& id, Version version) {
             raise_error(e.DeprecateComposition(id, version));
           })
      .def(
          "render",
          [context_from_object](Engine& e, const CompositionId& id,
                                nb::object context) {
            return unwrap(e.Render(id, context_from_object(context)));
          },
          nb::arg("id"), nb::arg("context") = nb::none())
      .def(
          "render_block",
          [context_from_object](Engine& e, const BlockId& id,
                                nb::object context) {
            try {
              return unwrap(e.RenderBlock(id, context_from_object(context)));
            } catch (PyError& error) {
              if (error.message == "Block not found") {
                error.code = ErrorCode::BlockNotFound;
              }
              throw;
            }
          },
          nb::arg("id"), nb::arg("context") = nb::none())
      .def("validate_block",
           [](Engine& e, const BlockId& id) {
             raise_error(e.ValidateBlock(id));
           })
      .def("validate_composition",
           [](Engine& e, const CompositionId& id) {
             raise_error(e.ValidateComposition(id));
           })
      .def("generate_block_data",
           [](Engine& e, const BlockGenerationRequest& r) {
             return unwrap(e.GenerateBlockData(r));
           })
      .def("generate_block_batch_data",
           [](Engine& e, const PromptSlicingRequest& r) {
             return unwrap(e.GenerateBlockBatchData(r));
           })
      .def("normalize",
           [](Engine& e, const std::string& text, const SemanticStyle& style) {
             return unwrap(e.Normalize(text, style));
           })
      .def("preview_normalize_composition",
           [](Engine& e, const CompositionNormalizationRequest& r) {
             return unwrap(e.PreviewNormalizeComposition(r));
           })
      .def("normalize_composition",
           [](Engine& e, const CompositionNormalizationRequest& r) {
             return unwrap(e.NormalizeComposition(r));
           })
      .def("preview_composition_block_rewrite",
           [](Engine& e, const CompositionBlockRewriteRequest& r) {
             return unwrap(e.PreviewCompositionBlockRewrite(r));
           })
      .def("apply_composition_block_rewrite",
           [](Engine& e, const CompositionBlockRewritePreview& p) {
             return unwrap(e.ApplyCompositionBlockRewrite(p));
           })
      .def("has_normalizer", &Engine::HasNormalizer)
      .def("has_block_normalizer", &Engine::HasBlockNormalizer)
      .def("has_block_generator", &Engine::HasBlockGenerator)
      .def("has_composition_block_rewriter",
           &Engine::HasCompositionBlockRewriter)
      .def(
          "configure_openai",
          [](Engine& e, OpenAiCompatibleConfig config, int timeout_ms,
             bool http2_allowed
#ifdef TEXTFOUNDRY_BUILD_TESTING
             ,
             nb::object fake
#endif
          ) {
            std::shared_ptr<IHttpTransport> transport;
#ifdef TEXTFOUNDRY_BUILD_TESTING
            if (!fake.is_none()) {
              auto& fake_transport = nb::cast<FakeTransport&>(fake);
              transport = std::shared_ptr<IHttpTransport>(
                  &fake_transport, [](IHttpTransport*) {});
            }
#endif
            if (!transport)
              transport = std::make_shared<QtHttpTransport>(
                  std::chrono::milliseconds(timeout_ms), http2_allowed);
            auto normalizer =
                std::make_shared<OpenAiCompatibleNormalizer>(config, transport);
            e.SetBlockGenerator(
                std::make_shared<OpenAiCompatibleBlockGenerator>(config,
                                                                 transport));
            e.SetNormalizer(normalizer);
            e.SetBlockNormalizer(normalizer);
            e.SetCompositionBlockRewriter(
                std::make_shared<OpenAiCompatibleCompositionBlockRewriter>(
                    config, transport));
          },
          nb::arg("config"), nb::arg("timeout_ms") = 30000,
          nb::arg("http2_allowed") = true
#ifdef TEXTFOUNDRY_BUILD_TESTING
          ,
          nb::arg("fake") = nb::none()
#endif
      );
#ifdef TEXTFOUNDRY_BUILD_TESTING
  auto testing = m.def_submodule("_testing");
  nb::class_<FakeTransport>(testing, "FakeTransport")
      .def(nb::init<>())
      .def_rw("response", &FakeTransport::response)
      .def_prop_ro("last_url",
                   [](const FakeTransport& t) { return t.last_request.url; })
      .def_prop_ro("last_body",
                   [](const FakeTransport& t) { return t.last_request.body; });
#endif
}
