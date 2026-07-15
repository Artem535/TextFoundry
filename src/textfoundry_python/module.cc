#include <nanobind/nanobind.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/unordered_map.h>
#include <nanobind/stl/vector.h>

#include "tf/block.h"
#include "tf/block_type.hpp"
#include "tf/composition.h"
#include "tf/error.h"
#include "tf/types.h"
#include "tf/version.h"

namespace nb = nanobind;
using namespace tf;

class PyError : public std::runtime_error {
 public:
  explicit PyError(const tf::Error& e) : std::runtime_error(e.message), code(e.code), message(e.message) {}
  ErrorCode code;
  std::string message;
};

template <typename T> T unwrap(Result<T> result) {
  if (result.HasError()) throw PyError(result.error());
  return std::move(result).value();
}

void raise_error(const Error& error) { if (error.is_error()) throw PyError(error); }

NB_MODULE(textfoundry, m) {
  m.attr("__version__") = "0.2.7";
  nb::object error = nb::module_::import_("builtins").attr("type")(
      "Error", nb::make_tuple(nb::module_::import_("builtins").attr("Exception")), nb::dict());
  m.attr("Error") = error;
  nb::register_exception_translator([](const std::exception_ptr &p, void *) {
    try { if (p) std::rethrow_exception(p); }
    catch (const PyError &e) {
      nb::object cls = nb::module_::import_("textfoundry").attr("Error");
      nb::object instance = cls(e.message);
      instance.attr("code") = nb::cast(e.code);
      instance.attr("message") = e.message;
      PyErr_SetObject(cls.ptr(), instance.ptr());
    }
  });

  nb::enum_<ErrorCode>(m, "ErrorCode")
      .value("MissingParam", ErrorCode::MissingParam).value("InvalidParamType", ErrorCode::InvalidParamType)
      .value("UnknownParam", ErrorCode::UnknownParam).value("VersionRequired", ErrorCode::VersionRequired)
      .value("VersionNotFound", ErrorCode::VersionNotFound).value("InvalidVersion", ErrorCode::InvalidVersion)
      .value("BlockNotFound", ErrorCode::BlockNotFound).value("CompositionNotFound", ErrorCode::CompositionNotFound)
      .value("DuplicateId", ErrorCode::DuplicateId).value("InvalidStateTransition", ErrorCode::InvalidStateTransition)
      .value("DraftRequired", ErrorCode::DraftRequired).value("PublishedRequired", ErrorCode::PublishedRequired)
      .value("TemplateSyntaxError", ErrorCode::TemplateSyntaxError).value("CircularReference", ErrorCode::CircularReference)
      .value("StorageError", ErrorCode::StorageError).value("Success", ErrorCode::Success);
  nb::class_<Version>(m, "Version")
      .def(nb::init<uint16_t, uint16_t>(), nb::arg("major") = 0, nb::arg("minor") = 0)
      .def_rw("major", &Version::major).def_rw("minor", &Version::minor)
      .def("to_string", &Version::ToString);
  nb::enum_<BlockType>(m, "BlockType").value("Role", BlockType::Role).value("System", BlockType::System)
      .value("Mission", BlockType::Mission).value("Safety", BlockType::Safety).value("Constraint", BlockType::Constraint)
      .value("Style", BlockType::Style).value("Domain", BlockType::Domain).value("Meta", BlockType::Meta);
  nb::enum_<BlockState>(m, "BlockState").value("Draft", BlockState::Draft).value("Published", BlockState::Published).value("Deprecated", BlockState::Deprecated);
  nb::enum_<SeparatorType>(m, "SeparatorType").value("Newline", SeparatorType::Newline).value("Paragraph", SeparatorType::Paragraph).value("Hr", SeparatorType::Hr);
  nb::class_<ParamSchema>(m, "ParamSchema")
      .def(nb::init<>()).def_rw("name", &ParamSchema::name).def_rw("required", &ParamSchema::required).def_rw("default_value", &ParamSchema::defaultValue);
  nb::class_<Template>(m, "Template").def(nb::init<std::string>()).def_prop_rw("content", &Template::Content, &Template::SetContent)
      .def("extract_param_names", &Template::ExtractParamNames)
      .def("expand", [](const Template& t, Params p) { return unwrap(t.Expand(p)); });
  nb::class_<RenderContext>(m, "RenderContext")
      .def(nb::init<>()).def_rw("params", &RenderContext::params).def_rw("target_language", &RenderContext::targetLanguage).def_rw("strict_mode", &RenderContext::strictMode)
      .def("with_param", &RenderContext::WithParam, nb::arg("name"), nb::arg("value"), nb::rv_policy::reference_internal)
      .def("with_language", &RenderContext::WithLanguage, nb::rv_policy::reference_internal)
      .def("with_strict_mode", &RenderContext::with_strict_mode, nb::rv_policy::reference_internal);
  nb::class_<StructuralStyle>(m, "StructuralStyle").def(nb::init<>())
      .def_rw("block_wrapper", &StructuralStyle::blockWrapper).def_rw("preamble", &StructuralStyle::preamble).def_rw("postamble", &StructuralStyle::postamble).def_rw("delimiter", &StructuralStyle::delimiter);
  nb::class_<SemanticStyle>(m, "SemanticStyle").def(nb::init<>())
      .def_rw("tone", &SemanticStyle::tone).def_rw("tense", &SemanticStyle::tense).def_rw("target_language", &SemanticStyle::targetLanguage)
      .def_rw("person", &SemanticStyle::person).def_rw("rewrite_strength", &SemanticStyle::rewriteStrength).def_rw("audience", &SemanticStyle::audience)
      .def_rw("locale", &SemanticStyle::locale).def_rw("terminology_rigidity", &SemanticStyle::terminologyRigidity)
      .def_rw("preserve_formatting", &SemanticStyle::preserveFormatting).def_rw("preserve_examples", &SemanticStyle::preserveExamples)
      .def("is_empty", &SemanticStyle::isEmpty);
  nb::class_<StyleProfile>(m, "StyleProfile").def(nb::init<>()).def_rw("structural", &StyleProfile::structural).def_rw("semantic", &StyleProfile::semantic)
      .def_static("plain", &StyleProfile::plain);
  nb::class_<BlockDraftBuilder>(m, "BlockDraftBuilder")
      .def(nb::init<BlockId>())
      .def("with_id", &BlockDraftBuilder::WithId, nb::rv_policy::reference_internal)
      .def("with_type", &BlockDraftBuilder::WithType, nb::rv_policy::reference_internal)
      .def("with_template", &BlockDraftBuilder::WithTemplate, nb::rv_policy::reference_internal)
      .def("with_default", &BlockDraftBuilder::WithDefault, nb::rv_policy::reference_internal)
      .def("with_defaults", &BlockDraftBuilder::WithDefaults, nb::rv_policy::reference_internal)
      .def("with_param_schema", &BlockDraftBuilder::WithParamSchema, nb::rv_policy::reference_internal)
      .def("with_tag", &BlockDraftBuilder::WithTag, nb::rv_policy::reference_internal)
      .def("with_language", &BlockDraftBuilder::WithLanguage, nb::rv_policy::reference_internal)
      .def("with_description", &BlockDraftBuilder::WithDescription, nb::rv_policy::reference_internal)
      .def("with_revision_comment", &BlockDraftBuilder::WithRevisionComment, nb::rv_policy::reference_internal)
      .def("build", &BlockDraftBuilder::build);
  nb::class_<PublishedBlock>(m, "PublishedBlock").def_prop_ro("id", &PublishedBlock::id).def_prop_ro("version", &PublishedBlock::version);
  nb::class_<CompositionDraftBuilder>(m, "CompositionDraftBuilder")
      .def(nb::init<>()).def(nb::init<CompositionId>())
      .def("with_id", &CompositionDraftBuilder::WithId, nb::rv_policy::reference_internal)
      .def("with_style_profile", &CompositionDraftBuilder::WithStyleProfile, nb::rv_policy::reference_internal)
      .def("with_project_key", &CompositionDraftBuilder::WithProjectKey, nb::rv_policy::reference_internal)
      .def("with_description", &CompositionDraftBuilder::WithDescription, nb::rv_policy::reference_internal)
      .def("with_revision_comment", &CompositionDraftBuilder::WithRevisionComment, nb::rv_policy::reference_internal)
      .def("add_static_text", &CompositionDraftBuilder::AddStaticText, nb::rv_policy::reference_internal)
      .def("add_separator", &CompositionDraftBuilder::AddSeparator, nb::rv_policy::reference_internal)
      .def("add_block_ref", nb::overload_cast<const BlockId&, uint16_t, uint16_t, Params>(&CompositionDraftBuilder::AddBlockRef), nb::arg("id"), nb::arg("major"), nb::arg("minor"), nb::arg("params") = Params{}, nb::rv_policy::reference_internal)
      .def("build", &CompositionDraftBuilder::build);
  nb::class_<BlockDraft>(m, "BlockDraft");
  nb::class_<CompositionDraft>(m, "CompositionDraft");
}
