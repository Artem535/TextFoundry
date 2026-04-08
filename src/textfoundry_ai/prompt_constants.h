#pragma once

#include <string_view>

namespace tf::ai::prompts {

inline constexpr std::string_view kBlockGenerationSystemPrompt = R"(You generate reusable TextFoundry blocks.
Return only the structured block payload that matches the provided JSON schema.
The block must be explicit, reusable, and safe to publish as a draft for later human review.
Prefer rich, fully usable block content over compressed summaries.
Preserve clear internal formatting such as headings, bullet lists, numbered steps, examples, and tagged sections when they help readability.
Do not flatten well-structured prompt content into short generic prose.
Preserve the original language of the source content unless the user explicitly asks for translation or language conversion.
If the source mixes languages, preserve that mixed-language behavior unless explicitly instructed otherwise.
Do not translate examples, section text, or quoted assistant responses just because a preferred language field is provided.
Use stable dot-separated ids.
Use "templ" for the block template field.
Allowed type values: role, system, mission, safety, constraint, style, domain, meta.
Choose only one of those exact strings for "type".)";

inline constexpr std::string_view kBlockBatchSystemPrompt = R"(You decompose prompt text into reusable TextFoundry blocks.
Return only the structured payload that matches the provided JSON schema.
Split the source into a small set of reusable blocks with clear boundaries.
Prefer 3-8 blocks unless the source is genuinely simpler or more complex.
Preserve the richness of the source. Do not rewrite a detailed structured prompt into a thin summary.
If the source already contains explicit sections, headings, XML-like tags, rules, examples, or response instructions, preserve those logical sections rather than collapsing them.
Keep psychologically or behaviorally important distinctions separate, for example system framing, mission, safety, behavior, response flow, depth guidance, style, examples, and history placeholders.
Preserve concrete examples whenever they materially shape assistant behavior.
Preserve formatting that helps downstream prompting quality: headings, bullet lists, numbered steps, quoted example dialogues, placeholders, and section tags.
Preserve the original language of each section unless the user explicitly asks for translation or language conversion.
If the source prompt contains mixed-language content, preserve that mixed-language structure unless explicitly instructed to unify the language.
Do not translate examples, safety text, response flow, or stylistic instructions by default.
Use stable dot-separated ids.
Use "templ" for the block template field.
Allowed type values: role, system, mission, safety, constraint, style, domain, meta.
Choose only one of those exact strings for each block "type".
Suggested mapping:
- system: top-level system framing and operating mode
- mission: goals, tasks, responsibilities, objectives
- safety: safety rules, prohibitions, guardrails, escalation rules
- role: persona or role behavior
- constraint: hard behavioral constraints or output requirements
- style: tone, formatting, response style
- domain: factual domain knowledge or working context
- meta: metadata or helper blocks
Avoid creating duplicate or trivial blocks.)";

inline constexpr std::string_view kBlockGenerationUserIntro =
    "Generate one reusable TextFoundry block.\n";
inline constexpr std::string_view kBlockGenerationUserRequestLabel =
    "User request:\n";
inline constexpr std::string_view kPreferredIdLabel = "Preferred id: ";
inline constexpr std::string_view kPreferredTypeLabel = "Preferred type: ";
inline constexpr std::string_view kPreferredLanguageLabel =
    "Preferred block language metadata: ";
inline constexpr std::string_view kExistingBlockIdsToAvoidLabel =
    "Existing block ids to avoid:\n";
inline constexpr std::string_view kBlockGenerationUserOutro =
    "Return defaults as a string-to-string object and tags as a flat array.\n"
    "Allowed type values: role, system, mission, safety, constraint, style, domain, meta.\n"
    "Preserve the language of the source content unless translation is explicitly requested.";

inline constexpr std::string_view kBatchUserIntro =
    "Decompose the following prompt text into reusable TextFoundry blocks.\n";
inline constexpr std::string_view kBatchSourceTextLabel = "Source text:\n";
inline constexpr std::string_view kNamespacePrefixLabel =
    "Use this namespace prefix for generated ids when possible: ";
inline constexpr std::string_view kReusableIdsLabel =
    "Existing block ids that may be reused when the same logical section is being updated:\n";
inline constexpr std::string_view kReusableIdsGuidance =
    "When a generated block matches one of those existing sections, reuse that exact id.\n"
    "This is an update of an existing composition, not a full redesign.\n"
    "Preserve the current block structure unless the source text clearly requires a change.\n"
    "Do not collapse many existing sections into one broad block.\n"
    "Prefer keeping roughly the same number of blocks as the existing structure.\n"
    "Preserve most existing block ids and only introduce a new id when a truly new section appears.\n"
    "If two existing sections still exist conceptually, keep them as separate blocks.\n"
    "Keep the level of detail of the existing structure unless the user explicitly asks to simplify it.\n"
    "Do not replace detailed safety, behavior, flow, style, or examples with short generic summaries.\n"
    "Preserve the language of each existing section unless translation is explicitly requested.\n";
inline constexpr std::string_view kPreserveStrengthPrefix =
    "Target preservation strength: keep about ";
inline constexpr std::string_view kPreserveStrengthSuffix =
    "% of the current reusable blocks and their logical boundaries.\n";
inline constexpr std::string_view kPreserveOrderGuidance =
    "Preserve the current order of blocks unless reordering is clearly necessary.\n";
inline constexpr std::string_view kCurrentStructureLabel =
    "Current structure to preserve:\n";
inline constexpr std::string_view kBatchUserOutro =
    "Return blocks as a flat array. Each block must be independently reusable.\n"
    "Allowed type values: role, system, mission, safety, constraint, style, domain, meta.\n"
    "Do not invent any other type values.\n"
    "Block ids must be stable lowercase identifiers using dot-separated segments.\n"
    "Do not return placeholder ids like _2, block_3, section_4, tmp, or draft.\n"
    "If the source contains explicit section tags or named sections, keep a closely matching section structure unless the user clearly requested a redesign.\n"
    "Prefer detailed, production-usable prompt text over compressed paraphrases.\n"
    "Preserve examples, guardrails, response flow, and depth guidance whenever they are present and still relevant.\n"
    "Do not translate the source content unless the user explicitly asked for translation.";

inline constexpr std::string_view kTextNormalizationSystemPrompt = R"(You normalize rendered TextFoundry output.
Preserve meaning.
Apply only the requested semantic style changes.
Respect rewrite strength and preservation constraints strictly.
Do not add explanations, metadata, markdown fences, or commentary.
Return only the normalized text.)";

inline constexpr std::string_view kBlockNormalizationSystemPrompt = R"(You cosmetically normalize a TextFoundry block template.
Make only light editorial changes such as grammar, fluency, clarity, and small phrasing improvements.
Preserve the block structure closely.
Preserve every placeholder exactly, including braces and names.
Do not add or remove placeholders.
Respect rewrite strength and preservation constraints strictly.
Do not turn multiple sections into one or remove explicit tags.
Return only the normalized template text.)";

inline constexpr std::string_view kNormalizeTextIntro =
    "Normalize the following raw text snapshot.\n";
inline constexpr std::string_view kNormalizeBlockIntro =
    "Normalize the following block template conservatively.\n";
inline constexpr std::string_view kRequestedSemanticStyleLabel =
    "Requested semantic style:\n";
inline constexpr std::string_view kTextLabel = "\nText:\n";
inline constexpr std::string_view kBlockIdLabel = "Block id: ";
inline constexpr std::string_view kBlockTypeLabel = "Block type: ";
inline constexpr std::string_view kPreservePlaceholdersLabel =
    "Preserve these placeholders exactly:\n";
inline constexpr std::string_view kTemplateLabel = "\nTemplate:\n";

inline constexpr std::string_view kReviseBlockIntro =
    "Revise the existing TextFoundry block. Keep the same logical block and improve or fix it according to the user instruction.\n";
inline constexpr std::string_view kReviseBlockIdentityGuidance =
    "Do not change the block id, type, or language intent.\n";
inline constexpr std::string_view kReviseBlockPreservationGuidance =
    "Preserve the block's role in the larger prompt. Do not turn a detailed structured block into a shorter generic summary.\n"
    "Prefer preserving existing tags, defaults, section structure, examples, headings, bullet lists, placeholders, and formatting unless the user instruction clearly requires changes.\n"
    "If the block contains explicit behavioral rules, safety rules, response flow, depth guidance, examples, or tagged sections, preserve and improve them rather than collapsing them.\n"
    "Preserve the original language of the block content unless the user explicitly asks for translation or language conversion.\n"
    "If the block mixes languages, keep that mixed-language structure unless the user explicitly asks to unify it.\n"
    "The result should feel more polished, more supportive, and more context-rich when requested, while staying faithful to the original block's purpose.\n";
inline constexpr std::string_view kReviseBlockUserInstructionLabel =
    "\nUser instruction:\n";
inline constexpr std::string_view kReviseBlockCurrentBlockLabel =
    "\n\nCurrent block:\n";

}  // namespace tf::ai::prompts
