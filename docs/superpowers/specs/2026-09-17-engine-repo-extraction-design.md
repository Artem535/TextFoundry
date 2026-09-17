# Engine Repository Extraction Design

## Goal

Extract `textfoundry_core` (today `src/textfoundry_engine/`) into its own
repository, `TextFoundryEngine`, so it can be consumed as an external
dependency by more than one project — the existing TextFoundry app, and a
second, independent C++ consumer (CppWiki, which needs the same
Block → Composition → Version reuse model for its own wiki pages). Today
the engine lives embedded inside the TextFoundry monorepo, which makes it
unusable outside that repository without vendoring a copy.

## Why now

`textfoundry_core` already has no Qt dependency (`src/textfoundry_engine/CMakeLists.txt`
links only `ctre`, `spdlog`, `objectbox`, `reflectcpp`, `fmt`) and the root
`CMakeLists.txt` already exports it as an installable CMake package:
`write_basic_package_version_file`, `cmake/TextFoundryConfig.cmake.in`,
`install(EXPORT TextFoundryTargets ... NAMESPACE TextFoundry::)`. This
packaging infrastructure already exists and is already scoped to the
engine's own dependencies (`TextFoundryConfig.cmake.in` only
`find_dependency`s `ctre`/`spdlog`/`reflectcpp` and fetches `objectbox`) —
extraction is a smaller step than it would be in a project without this
groundwork.

The one real complication: `tests/test_main.cc` is a single 1207-line
doctest binary, `core_tests`, that exercises both `textfoundry_core` and
`textfoundry_ai_openai` together through relative includes
(`../src/textfoundry_engine/tf/...` and `../src/textfoundry_ai/...`).
Splitting this is the bulk of the actual work.

## Scope

In scope: creating the new repository with the engine's source, tests, and
build/package infrastructure; migrating the TextFoundry repository to
consume it as an external dependency instead of an embedded subdirectory;
CI for the new repository. `textfoundry_ai`, the GUI, and the CLI are not
touched beyond how they link the engine.

## New repository: `TextFoundryEngine`

- Hosted at `git@github.com:Artem535/TextFoundryEngine.git` — same account
  as `TextFoundry` and `CppWiki`.
- Contents: `src/textfoundry_engine/tf/` (all `Block`, `Composition`,
  `Version`, `Renderer`, `ObjectBox` repository code), a standalone root
  `CMakeLists.txt`, and `cmake/TextFoundryConfig.cmake.in` adapted from the
  existing one.
- CMake target stays `textfoundry_core`, exported as `TextFoundry::Core`
  (`EXPORT_NAME Core` is unchanged) — minimizes churn in every consumer,
  including the new one (CppWiki links `TextFoundry::Core` the same way
  TextFoundry itself will).
- Versioning starts fresh at `v0.1.0`: a new release unit, not a
  continuation of the app's `0.2.7`.
- Git history: extracted via `git subtree split -P src/textfoundry_engine`
  from the TextFoundry repository, so `git log`/`git blame` on engine files
  survives the move instead of starting from a single flattened commit.

## Splitting `tests/test_main.cc`

Read through the file and classify each `TEST_CASE` by what it includes:

- Cases touching only `tf/*.h` headers (`Block`, `Composition`, `Version`,
  `Renderer`, `Engine`, `Error`, ...) move to the new repository as its own
  `core_tests` binary, using `doctest` the same way it's used today.
- Cases touching `textfoundry_ai/*` headers (integration between engine and
  AI adapters) stay in the TextFoundry repository. That repository's
  `core_tests` continues to exist, but now links `textfoundry_core` fetched
  externally rather than an in-tree target.

No duplication: each `TEST_CASE` moves to exactly one place, chosen by what
it actually exercises.

## Migrating TextFoundry to consume the engine externally

Replace, in the root `CMakeLists.txt`:

```cmake
add_subdirectory(src/textfoundry_engine)
```

with a `FetchContent_Declare`/`FetchContent_MakeAvailable` pair pinned to a
released tag — the same pattern already used for `ftxui`, `objectbox`, and
`qtkeychain` in this file:

```cmake
FetchContent_Declare(
    textfoundry_engine
    GIT_REPOSITORY https://github.com/Artem535/TextFoundryEngine.git
    GIT_TAG v0.1.0
)
FetchContent_MakeAvailable(textfoundry_engine)
```

`src/textfoundry_engine/` is deleted from the TextFoundry repository once
the split is verified working. `textfoundry_ai`, `text_foundry_cli`,
`text_foundry_gui`, and `tests/` need no code changes — they already link
`textfoundry_core` by target name, which now resolves to the fetched
target instead of an in-tree one.

## CI for the new repository

The existing `.github/workflows/ci.yml` is entirely about the GUI app: Qt
install, AppImage/DMG/Inno Setup packaging, release publishing — none of
it applies to a Qt-free library. The new repository's CI is a small subset
of the existing steps: checkout, bootstrap vcpkg (same `VCPKG_COMMIT`
pattern), `cmake --preset` configure, `cmake --build`, `ctest`. No Qt
install, no packaging, no publish job. Start with Linux only; extend to
macOS/Windows later if a consumer needs it verified there.

## Delivery Tasks

1. **Classify `test_main.cc`.** Read all 1207 lines, tag each `TEST_CASE`
   as engine-only or AI-touching.
2. **Extract with history.** `git subtree split -P src/textfoundry_engine`
   from TextFoundry; push the result as the initial history of the new
   `TextFoundryEngine` repository.
3. **Stand up the new repository's build.** Standalone root
   `CMakeLists.txt`, adapted `TextFoundryConfig.cmake.in`, `vcpkg.json`
   scoped to the engine's own dependencies (`ctre`, `spdlog`, `reflectcpp`,
   `fmt`, `objectbox`, `doctest`).
4. **Move the engine-only tests** into the new repository as its
   `core_tests` binary; confirm they pass standalone.
5. **Add CI** to the new repository per the CI section above.
6. **Tag `v0.1.0`.**
7. **Migrate TextFoundry**: swap `add_subdirectory` for `FetchContent`
   pinned to `v0.1.0`, delete `src/textfoundry_engine/`, keep the
   AI-touching tests in TextFoundry's own `core_tests` linking the fetched
   target.
8. **Full CI pass** on TextFoundry with the new dependency wired in.

## Verification

`ctest` green in both repositories. TextFoundry's GUI and CLI build and
their existing tests pass with the engine linked externally. `git log --
tf/block.h` in the new repository shows history predating the extraction,
not a single flattened commit.
