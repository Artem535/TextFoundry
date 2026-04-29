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

## Current Surface

The repository currently contains:

- a Qt/QML GUI workbench in `src/text_foundry_gui`
- a CLI/TUI surface in `src/text_foundry_cli`
- a core engine and storage layer in `src/textfoundry_engine`
- OpenAI-compatible AI integration in `src/textfoundry_ai`

## Main Capabilities

At the moment, TextFoundry focuses on:

- creating and editing prompt blocks
- assembling blocks into compositions
- versioning block and composition changes
- deterministic prompt rendering with runtime parameters
- previewing raw, rendered, and normalized output
- AI-assisted block generation, block revision, slicing, normalization, and composition rewrite

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
