# TextFoundry

TextFoundry is a desktop-first workbench for structured prompt authoring.

Instead of treating prompts as large ad hoc text blobs, TextFoundry models them
as reusable assets:

- `Blocks` for reusable prompt fragments
- `Compositions` for assembling blocks into larger prompts
- `Versions` for traceable revisions
- deterministic `Render` for producing final output
- explicit AI-assisted actions for generation, revision, slicing, and rewrite

The project is built around the idea that prompt engineering should be managed
as an asset workflow rather than a pile of copied text files.

## What Problem It Solves

Many teams still keep prompts in chat messages, loose markdown files, code
string literals, or manually copied "final versions". That breaks down quickly:

- reusable prompt parts are hard to extract
- version history is unclear
- changes are difficult to compare
- AI-assisted edits mix with manual edits without a clear boundary
- teams lose deterministic control over what is actually being sent to an LLM

TextFoundry addresses this by introducing explicit assets and workflows instead
of treating prompts as anonymous text blobs.

## Core Model

The main domain entities are:

- `Block` — a reusable, versioned prompt fragment with metadata, template text,
  defaults, tags, and language information
- `Composition` — an ordered assembly of fragments built from block references,
  static text, and separators
- `Version` — a traceable revision of a block or composition
- `Renderer` — the deterministic path that turns a composition into a final
  prompt

Architecturally, the project keeps a strict boundary between:

- deterministic rendering
- explicit AI-assisted workflows

Normal rendering is not supposed to silently call an LLM. AI is exposed as a
tooling layer, not as hidden runtime behavior.

## Current Surface

The repository currently contains:

- a Qt/QML GUI workbench in `src/text_foundry_gui`
- a CLI/TUI surface in `src/text_foundry_cli`
- a core engine and storage layer in `src/textfoundry_engine`
- OpenAI-compatible AI integration in `src/textfoundry_ai`

## Typical Workflow

A typical TextFoundry session looks like this:

1. Create or revise prompt blocks.
2. Publish a new version of those blocks.
3. Assemble a composition from block references and static fragments.
4. Render the composition deterministically with runtime parameters.
5. Optionally run AI-assisted actions such as generation, normalization,
   slicing, or rewrite.
6. Review the result and explicitly decide whether to keep, publish, or discard
   the changes.

## Main Capabilities

At the moment, TextFoundry focuses on:

- creating and editing prompt blocks
- assembling blocks into compositions
- versioning block and composition changes
- deterministic prompt rendering with runtime parameters
- previewing raw, rendered, and normalized output
- AI-assisted block generation, block revision, slicing, normalization, and composition rewrite

## Current Architecture

At a high level, the codebase is split into four main layers:

- `textfoundry_engine` for domain logic, rendering, validation, and storage-backed repositories
- `textfoundry_ai` for OpenAI-compatible adapters and AI-assisted workflows
- `text_foundry_gui` for the main desktop workbench
- `text_foundry_cli` for command-line and terminal-oriented flows

The persistence layer currently uses `ObjectBox` as the primary storage engine.
The GUI is based on `Qt 6 / QML`, and the TUI is based on `FTXUI`.

## Current State

The project already supports a real end-to-end loop:

- publish versioned blocks
- assemble and version compositions
- render with runtime parameters
- edit through new revisions instead of in-place mutation
- use AI-assisted workflows through explicit user actions

This is already a working prompt-authoring system, not just a prototype layout.

## Build

The project uses CMake and vcpkg for the C++ dependency set.

Basic release configure/build:

```bash
cmake --preset vcpkg-rel
cmake --build build-rel --parallel
ctest --test-dir build-rel --output-on-failure
```

Notes:

- the GUI is built when Qt 6 is available
- Linux AppImage is the main self-contained Linux artifact
- Fedora RPM is built in a separate Fedora-native GitHub Actions pipeline

### Main Build Dependencies

The repository currently depends on:

- `C++23`
- `CMake`
- `vcpkg`
- `Qt 6`
- `ObjectBox`
- `FTXUI`
- `CLI11`
- `spdlog`
- `reflectcpp`

Some GUI-side dependencies are resolved either from the system or through
vendored/fetched builds, depending on the platform and workflow.

### Build Notes

- The default configure preset is `vcpkg-rel`.
- The GUI target is built only when a suitable Qt installation is available.
- On Linux, AppImage is the primary distributable.
- Fedora RPM is intentionally built in a separate Fedora-native pipeline so it
  links against Fedora system packages instead of a generic Qt SDK bundle.

## Documentation

Project documentation is published as an Antora site and also kept in `doc/`.

Key sections:

- project overview and requirements
- architecture and ADRs
- implementation status
- API reference
- product presentation

Build docs locally:

```bash
npm install
npm run docs:build
```

The generated site is written to `public/`.

## Published Pages

GitHub Pages is structured as:

- `/` for the product presentation
- `/docs/` for the documentation site

## Packaging

Current packaging targets:

- Linux: AppImage
- macOS: DMG
- Windows: Inno Setup installer
- Fedora: separate RPM pipeline

## Current Limitations

Some constraints are intentional at the current stage:

- AI features require an external OpenAI-compatible endpoint
- semantic rewrite and normalization are explicit workflows, not part of the
  default deterministic render path
- the public documentation includes both current implementation notes and
  forward-looking architectural material, so `doc/status.adoc` should be read
  as the implementation snapshot
- packaging and runtime integration are still being hardened across all three
  desktop platforms

## Roadmap Direction

The current desktop workbench is the center of the repository, but the broader
direction also includes:

- a future `Prompt Server`
- published prompt registry workflows
- remote render/runtime surfaces
- clearer separation between local authoring and server-side execution

In short, the project is moving toward a managed prompt asset system rather than
only a local editor.

## Repository Layout

- `src/textfoundry_engine` — core domain and storage
- `src/textfoundry_ai` — AI adapters and prompt workflows
- `src/text_foundry_gui` — Qt/QML GUI
- `src/text_foundry_cli` — CLI and TUI
- `doc` — source documentation
- `.github/workflows` — CI, release, pages, and Fedora RPM workflows

## Related Docs

- [Antora setup](doc/README-antora.adoc)
- [Documentation index](doc/modules/ROOT/pages/index.adoc)
- [Product presentation source](doc/modules/ROOT/pages/project/about/product-presentation.adoc)
