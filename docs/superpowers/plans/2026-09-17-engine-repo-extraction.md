# Engine Repository Extraction Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move `textfoundry_core` (today `src/textfoundry_engine/`) into its own repository, `TextFoundryEngine`, and switch TextFoundry to consume it externally via `FetchContent`, the same pattern already used for `ftxui`/`objectbox`/`qtkeychain`.

**Architecture:** Split the one mixed-concern test binary first (inside TextFoundry, without touching the engine's location) so both halves are proven working before anything moves. Extract `src/textfoundry_engine/` with git history via `git subtree split`. Stand up a standalone CMake package in the new repository (the packaging scaffolding already exists in TextFoundry and is already scoped to just this library — reuse it, don't redesign it). Wire CI. Tag a release. Only then switch TextFoundry over and delete the in-tree copy.

**Tech Stack:** CMake 3.20+, C++23, vcpkg, ObjectBox, doctest, GitHub Actions, `gh` CLI.

## Global Constraints

- CMake target name stays `textfoundry_core`, exported as `TextFoundry::Core` (`EXPORT_NAME Core` unchanged) — per spec, this minimizes churn in every consumer.
- New repository: `git@github.com:Artem535/TextFoundryEngine.git`.
- New repository's package config keeps the existing name/namespace (`TextFoundryConfig.cmake`, `TextFoundry::` namespace, `.../cmake/TextFoundry` install dir) — only the git repository and `project()` are named `TextFoundryEngine`; the CMake package identity a consumer sees is unchanged.
- Engine versioning starts fresh at `v0.1.0` (independent of the app's `0.2.7`).
- History must be preserved via `git subtree split -P src/textfoundry_engine`, not a flattened first commit.
- No task may leave either repository's `ctest` red. Every task ends with both repos (or the one being touched) building and testing clean.

---

### Task 1: Split `test_main.cc` into engine-only and AI-only binaries (inside TextFoundry)

**Files:**
- Modify: `tests/test_main.cc` (trim to engine-only)
- Create: `tests/ai_test_main.cc` (AI-only, new)
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Produces: two test binaries, `core_tests` (engine-only, no `textfoundry_ai` link) and `ai_tests` (links `textfoundry_core` + `textfoundry_ai_openai`). `core_tests`'s content is exactly what Task 3 copies into the new repository.

Today `tests/test_main.cc` is one 1207-line doctest binary mixing engine
tests and one AI-adapter test suite. Only `TEST_SUITE("OpenAiCompatibleBlockGenerator")`
(current lines 424–519) and the `FakeHttpTransport` class (current lines
129–147) touch `textfoundry_ai` — everything else (`Version`, `Error`,
`Result`, `BlockGeneration`, `CompositionNormalization`, `BlockType`,
`Template`, `Block`, `BlockRef`, `Fragment`, `Composition`,
`BlockDraftBuilder`, `CompositionDraftBuilder`, the unnamed Renderer suite,
and the unnamed Engine Integration suite) only touches engine headers,
including the `BlockGeneration`/`CompositionNormalization` suites, which
test the engine's own `IBlockGenerator`/`IBlockNormalizer` interfaces
through in-file fakes (`FakeBlockGenerator`, `FakeBlockNormalizer`) — not
`textfoundry_ai`.

- [ ] **Step 1: Create `tests/ai_test_main.cc`** with this exact content:

```cpp
//
// AI-integration tests: exercise textfoundry_ai adapters against a fake
// HTTP transport. Split out of test_main.cc so the engine-only tests can
// move to the TextFoundryEngine repository without an textfoundry_ai
// dependency riding along.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "../src/textfoundry_engine/tf/block_generation.h"
#include "../src/textfoundry_engine/tf/block_type.hpp"
#include "../src/textfoundry_engine/tf/error.h"
#include "../src/textfoundry_ai/openai_compatible_block_generator.h"

using namespace tf;

namespace {

class FakeHttpTransport final : public tf::ai::IHttpTransport {
 public:
  explicit FakeHttpTransport(Result<tf::ai::HttpResponse> response)
      : response_(std::move(response)) {}

  [[nodiscard]] Result<tf::ai::HttpResponse> PostJson(
      const tf::ai::HttpRequest& request) const override {
    last_request = request;
    if (response_.HasError()) {
      return Result<tf::ai::HttpResponse>(response_.error());
    }
    return Result<tf::ai::HttpResponse>(response_.value());
  }

  mutable std::optional<tf::ai::HttpRequest> last_request;

 private:
  Result<tf::ai::HttpResponse> response_;
};

}  // namespace

TEST_SUITE("OpenAiCompatibleBlockGenerator") {
  TEST_CASE("builds chat completions request and parses structured response") {
    auto transport = std::make_shared<FakeHttpTransport>(
        Result<tf::ai::HttpResponse>(tf::ai::HttpResponse{
            .status_code = 200,
            .body =
                R"({"choices":[{"message":{"role":"assistant","content":"{\"id\":\"role.code_reviewer\",\"type\":\"role\",\"language\":\"en\",\"description\":\"Reviews code changes\",\"templ\":\"Review {{subject}} for {{focus}}.\",\"defaults\":{\"focus\":\"correctness\"},\"tags\":[\"review\",\"quality\"]}"}}]})",
        }));

    tf::ai::OpenAiCompatibleBlockGenerator generator(
        {.base_url = "https://example.test/v1",
         .model = "gpt-4.1-mini",
         .api_key = "secret"},
        transport);

    auto result = generator.GenerateBlock(
        {.prompt = "Create a code reviewer block",
         .preferred_type = BlockType::Role,
         .existing_block_ids = {"role.existing"}});
    REQUIRE(result.HasValue());
    REQUIRE(transport->last_request.has_value());

    CHECK(transport->last_request->url ==
          "https://example.test/v1/chat/completions");
    CHECK(transport->last_request->headers.at("Authorization") ==
          "Bearer secret");
    CHECK(transport->last_request->body.find("\"response_format\"") !=
          std::string::npos);
    CHECK(transport->last_request->body.find("\"json_schema\"") !=
          std::string::npos);
    CHECK(result.value().id == "role.code_reviewer");
    CHECK(result.value().type == BlockType::Role);
    CHECK(result.value().templ == "Review {{subject}} for {{focus}}.");
    CHECK(result.value().defaults.at("focus") == "correctness");
    CHECK(result.value().tags.size() == 2);
  }

  TEST_CASE("returns error on non-success HTTP status") {
    auto transport = std::make_shared<FakeHttpTransport>(
        Result<tf::ai::HttpResponse>(
            tf::ai::HttpResponse{.status_code = 401, .body = "{}"}));

    tf::ai::OpenAiCompatibleBlockGenerator generator(
        {.base_url = "https://example.test/v1",
         .model = "gpt-4.1-mini",
         .api_key = "secret"},
        transport);

    auto result = generator.GenerateBlock({.prompt = "Create a block"});
    REQUIRE(result.HasError());
    CHECK(result.error().code == ErrorCode::StorageError);
  }

  TEST_CASE("builds batch request and parses structured block array response") {
    auto transport = std::make_shared<FakeHttpTransport>(
        Result<tf::ai::HttpResponse>(tf::ai::HttpResponse{
            .status_code = 200,
            .body =
                R"({"choices":[{"message":{"role":"assistant","content":"{\"blocks\":[{\"id\":\"team.role.system\",\"type\":\"role\",\"language\":\"en\",\"description\":\"System role\",\"templ\":\"You are {{assistant_name}}.\",\"defaults\":{},\"tags\":[\"system\"]},{\"id\":\"team.constraint.style\",\"type\":\"constraint\",\"language\":\"en\",\"description\":\"Style constraints\",\"templ\":\"Be concise.\",\"defaults\":{},\"tags\":[\"style\"]}]}"}}]})",
        }));

    tf::ai::OpenAiCompatibleBlockGenerator generator(
        {.base_url = "https://example.test/v1",
         .model = "gpt-4.1-mini",
         .api_key = "secret"},
        transport);

    auto result = generator.GenerateBlocks(
        {.source_text = "Long prompt",
         .preferred_language = "en",
         .namespace_prefix = "team"});
    REQUIRE(result.HasValue());
    REQUIRE(transport->last_request.has_value());

    CHECK(transport->last_request->body.find("textfoundry_block_batch") !=
          std::string::npos);
    CHECK(result.value().blocks.size() == 2);
    CHECK(result.value().blocks.at(0).id == "team.role.system");
    CHECK(result.value().blocks.at(1).type == BlockType::Constraint);
  }

  TEST_CASE("returns config validation error before transport call") {
    auto transport = std::make_shared<FakeHttpTransport>(
        Result<tf::ai::HttpResponse>(
            tf::ai::HttpResponse{.status_code = 200, .body = "{}"}));

    tf::ai::OpenAiCompatibleBlockGenerator generator(
        {.base_url = "", .model = "gpt-4.1-mini", .api_key = "secret"},
        transport);

    auto result = generator.GenerateBlock({.prompt = "Create a block"});
    REQUIRE(result.HasError());
    CHECK(result.error().code == ErrorCode::InvalidParamType);
    CHECK_FALSE(transport->last_request.has_value());
  }
}
```

- [ ] **Step 2: Trim `tests/test_main.cc`** — remove exactly three things, in this order:

  a. Remove the AI include (currently line 29):
  ```
  #include "../src/textfoundry_ai/openai_compatible_block_generator.h"
  ```

  b. Remove the `FakeHttpTransport` class (currently lines 129–147):
  ```cpp
  class FakeHttpTransport final : public tf::ai::IHttpTransport {
   public:
    explicit FakeHttpTransport(Result<tf::ai::HttpResponse> response)
        : response_(std::move(response)) {}

    [[nodiscard]] Result<tf::ai::HttpResponse> PostJson(
        const tf::ai::HttpRequest& request) const override {
      last_request = request;
      if (response_.HasError()) {
        return Result<tf::ai::HttpResponse>(response_.error());
      }
      return Result<tf::ai::HttpResponse>(response_.value());
    }

    mutable std::optional<tf::ai::HttpRequest> last_request;

   private:
    Result<tf::ai::HttpResponse> response_;
  };

  ```

  c. Remove the entire `TEST_SUITE("OpenAiCompatibleBlockGenerator")` block
  (currently lines 424–519 — the whole suite, from the `TEST_SUITE("OpenAiCompatibleBlockGenerator") {` line through its closing `}`, immediately before the `// ==================== BlockType Tests ====================` comment).

  Leave everything else in `tests/test_main.cc` untouched — file stays
  named `test_main.cc`, keeps its own `LoggerSetup`, `EngineTestFixture`,
  `FakeBlockGenerator`, `FakeBlockNormalizer`, and every remaining
  `TEST_SUITE`/`TEST_CASE_FIXTURE`.

- [ ] **Step 3: Rewrite `tests/CMakeLists.txt`** to this exact content:

```cmake
find_package(doctest CONFIG REQUIRED)

add_executable(core_tests
        test_main.cc
)

target_link_libraries(core_tests textfoundry_core doctest::doctest)

if(UNIX AND TARGET objectbox)
  set_property(TARGET core_tests PROPERTY BUILD_WITH_INSTALL_RPATH OFF)
  set_property(TARGET core_tests APPEND PROPERTY
    BUILD_RPATH "$<TARGET_FILE_DIR:objectbox>"
  )
endif()

add_test(NAME core_tests COMMAND core_tests)

if(WIN32 AND TARGET objectbox)
  set_tests_properties(core_tests PROPERTIES
    ENVIRONMENT_MODIFICATION
      "PATH=path_list_prepend:$<TARGET_FILE_DIR:objectbox>;PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin;PATH=path_list_prepend:${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin"
  )
endif()

add_executable(ai_tests
        ai_test_main.cc
)

target_link_libraries(ai_tests textfoundry_core textfoundry_ai_openai doctest::doctest)

if(UNIX AND TARGET objectbox)
  set_property(TARGET ai_tests PROPERTY BUILD_WITH_INSTALL_RPATH OFF)
  set_property(TARGET ai_tests APPEND PROPERTY
    BUILD_RPATH "$<TARGET_FILE_DIR:objectbox>"
  )
endif()

add_test(NAME ai_tests COMMAND ai_tests)

if(WIN32 AND TARGET objectbox)
  set_tests_properties(ai_tests PROPERTIES
    ENVIRONMENT_MODIFICATION
      "PATH=path_list_prepend:$<TARGET_FILE_DIR:objectbox>;PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin;PATH=path_list_prepend:${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin"
  )
endif()
```

- [ ] **Step 4: Configure and build**

Run: `cmake --preset vcpkg-rel && cmake --build build-rel --parallel`
Expected: both `core_tests` and `ai_tests` binaries build with no errors.

- [ ] **Step 5: Run both test binaries**

Run: `ctest --test-dir build-rel --output-on-failure`
Expected: both `core_tests` and `ai_tests` pass, same total assertion count
as the original single `core_tests` binary before the split (no test was
dropped, only relocated).

- [ ] **Step 6: Commit**

```bash
git add tests/test_main.cc tests/ai_test_main.cc tests/CMakeLists.txt
git commit -m "$(cat <<'EOF'
Split core_tests into engine-only and AI-only binaries

Prepares for extracting the engine into its own repository: core_tests
now has zero textfoundry_ai dependency and is exactly what moves.

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>
EOF
)"
```

---

### Task 2: Extract engine history and push to the new repository

**Files:** none modified in TextFoundry; creates the `TextFoundryEngine` GitHub repository.

**Interfaces:**
- Produces: `git@github.com:Artem535/TextFoundryEngine.git`, `main` branch,
  containing the pre-split history of `tf/*.h`/`tf/*.cc` and
  `CMakeLists.txt` (both at repository root — `git subtree split -P`
  flattens the prefix directory into the new root automatically).

- [ ] **Step 1: Split the engine's history out of TextFoundry**

Run, from `/home/a.durynin/Projects/C++/textfoundry`:
```bash
git subtree split -P src/textfoundry_engine -b engine-extract
```
Expected: prints a commit hash and creates local branch `engine-extract`
whose tree root is what was `src/textfoundry_engine/` (so `tf/` and
`CMakeLists.txt` sit directly at that branch's root).

- [ ] **Step 2: Verify the split branch's contents**

Run: `git ls-tree engine-extract`
Expected: exactly `CMakeLists.txt` and `tf/` at top level — nothing else.

- [ ] **Step 3: Create the GitHub repository**

Run:
```bash
gh repo create Artem535/TextFoundryEngine --private \
  --description "Reusable text authoring engine: Block, Composition, Version, deterministic Render."
```
Expected: repository created, printed URL
`https://github.com/Artem535/TextFoundryEngine`.

- [ ] **Step 4: Push the split history**

Run:
```bash
git push git@github.com:Artem535/TextFoundryEngine.git engine-extract:main
```
Expected: push succeeds; `main` branch on the new repo now holds the
extracted history.

- [ ] **Step 5: Verify history on the new repo**

Run: `git clone git@github.com:Artem535/TextFoundryEngine.git /home/a.durynin/Projects/C++/TextFoundryEngine`
then `git -C /home/a.durynin/Projects/C++/TextFoundryEngine log --oneline -- tf/block.h`
Expected: more than one commit (pre-extraction history is present, not a
single flattened commit).

- [ ] **Step 6: Clean up the local split branch in TextFoundry**

Run: `git -C /home/a.durynin/Projects/C++/textfoundry branch -D engine-extract`

All further tasks in this plan work inside
`/home/a.durynin/Projects/C++/TextFoundryEngine` unless stated otherwise.

---

### Task 3: Stand up a standalone build in the new repository

**Files (all in `/home/a.durynin/Projects/C++/TextFoundryEngine`):**
- Modify: `CMakeLists.txt` (currently the old nested-subdirectory version; becomes the standalone root)
- Create: `cmake/TextFoundryConfig.cmake.in`
- Create: `vcpkg.json`
- Create: `CMakePresets.json`
- Create: `tests/CMakeLists.txt`
- Create: `tests/test_main.cc`

**Interfaces:**
- Consumes: nothing external beyond what Task 1 already produced (the
  trimmed `tests/test_main.cc` from TextFoundry, copied here with updated
  include paths).
- Produces: a `textfoundry_core` static library target, exported as
  `TextFoundry::Core`, installable via `cmake --install`, and a passing
  `core_tests` binary.

- [ ] **Step 1: Rewrite `CMakeLists.txt`** to this exact content (the
  existing library/install rules are unchanged; everything above and below
  them is new, adapted from TextFoundry's root `CMakeLists.txt`):

```cmake
cmake_minimum_required(VERSION 3.20)
project(TextFoundryEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)

include(GNUInstallDirs)
include(FetchContent)
include(CMakePackageConfigHelpers)

set(TEXTFOUNDRY_INSTALL_CMAKEDIR
    "${CMAKE_INSTALL_LIBDIR}/cmake/TextFoundry")

# ObjectBox
FetchContent_Declare(
        objectbox
        GIT_REPOSITORY https://github.com/objectbox/objectbox-c.git
        GIT_TAG v5.1.0
)

FetchContent_MakeAvailable(objectbox)
set(OBJECTBOX_LICENSE_FILE "${objectbox_SOURCE_DIR}/LICENSE.txt")

# Other lib
find_package(ctre CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(reflectcpp CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)


add_library(textfoundry_core STATIC
        tf/engine.cc
        tf/engine.h
        tf/block_generation.h
        tf/block.cc
        tf/block.h
        tf/block_ref.cc
        tf/block_ref.h
        tf/fragment.cc
        tf/fragment.h
        tf/composition.cc
        tf/composition.h
        tf/renderer.cc
        tf/renderer.h
        tf/block_type.hpp
        tf/version.h
        tf/error.h
        tf/logger.h
        tf/logger.cc
        tf/objectbox_repository.cc
        tf/objectbox_repository.h
        tf/repository_block_cache.cpp
        tf/types.h
        tf/objectbox-model.h
        tf/database_scheme.obx.hpp
        tf/database_scheme.obx.cpp
)

target_include_directories(textfoundry_core
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(textfoundry_core
        PUBLIC
            ctre::ctre
            spdlog::spdlog
            objectbox
            reflectcpp::reflectcpp
            fmt::fmt
)

set_target_properties(textfoundry_core PROPERTIES EXPORT_NAME Core)

install(
        TARGETS textfoundry_core
        EXPORT TextFoundryTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tf
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
)

if(WIN32)
  set(TEXTFOUNDRY_OBJECTBOX_INSTALL_DIR "${CMAKE_INSTALL_BINDIR}")
else()
  set(TEXTFOUNDRY_OBJECTBOX_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}")
endif()

if(TARGET objectbox)
  install(FILES $<TARGET_FILE:objectbox>
    DESTINATION ${TEXTFOUNDRY_OBJECTBOX_INSTALL_DIR}
  )
endif()

enable_testing()
add_subdirectory(tests)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/TextFoundryConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/TextFoundryConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/TextFoundryConfig.cmake"
    INSTALL_DESTINATION "${TEXTFOUNDRY_INSTALL_CMAKEDIR}"
)

install(
    EXPORT TextFoundryTargets
    FILE TextFoundryTargets.cmake
    NAMESPACE TextFoundry::
    DESTINATION "${TEXTFOUNDRY_INSTALL_CMAKEDIR}"
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/TextFoundryConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/TextFoundryConfigVersion.cmake"
    DESTINATION "${TEXTFOUNDRY_INSTALL_CMAKEDIR}"
)
```

- [ ] **Step 2: Create `cmake/TextFoundryConfig.cmake.in`** with this exact
  content (identical to TextFoundry's existing file — it was already
  scoped to only the engine's own dependencies):

```cmake
@PACKAGE_INIT@

include(CMakeFindDependencyMacro)

find_dependency(ctre CONFIG REQUIRED)
find_dependency(spdlog CONFIG REQUIRED)
find_dependency(reflectcpp CONFIG REQUIRED)

include(FetchContent)

if(NOT TARGET objectbox)
  FetchContent_Declare(
      objectbox
      GIT_REPOSITORY https://github.com/objectbox/objectbox-c.git
      GIT_TAG v5.1.0
  )
  FetchContent_MakeAvailable(objectbox)
endif()

include("${CMAKE_CURRENT_LIST_DIR}/TextFoundryTargets.cmake")

check_required_components(TextFoundry)
```

- [ ] **Step 3: Create `vcpkg.json`** with this exact content:

```json
{
  "name": "textfoundry-engine",
  "version": "0.1.0",
  "dependencies": [
    {
      "name": "ctre",
      "version>=": "3.10.0"
    },
    {
      "name": "doctest",
      "version>=": "2.4.12"
    },
    {
      "name": "reflectcpp",
      "version>=": "0.23.0"
    },
    {
      "name": "spdlog",
      "version>=": "1.17.0"
    }
  ]
}
```

- [ ] **Step 4: Create `CMakePresets.json`** with this exact content:

```json
{
    "version": 2,
    "configurePresets": [
      {
        "name": "vcpkg",
        "generator": "Ninja",
        "binaryDir": "${sourceDir}/build",
        "cacheVariables": {
          "CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        }
      },
      {
        "name": "vcpkg-rel",
        "generator": "Ninja",
        "binaryDir": "${sourceDir}/build-rel",
        "cacheVariables": {
          "CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake",
          "CMAKE_BUILD_TYPE": "Release"
        }
      }
    ]
}
```

- [ ] **Step 5: Create `tests/CMakeLists.txt`** with this exact content:

```cmake
find_package(doctest CONFIG REQUIRED)

add_executable(core_tests
        test_main.cc
)

target_link_libraries(core_tests textfoundry_core doctest::doctest)

if(UNIX AND TARGET objectbox)
  set_property(TARGET core_tests PROPERTY BUILD_WITH_INSTALL_RPATH OFF)
  set_property(TARGET core_tests APPEND PROPERTY
    BUILD_RPATH "$<TARGET_FILE_DIR:objectbox>"
  )
endif()

add_test(NAME core_tests COMMAND core_tests)

if(WIN32 AND TARGET objectbox)
  set_tests_properties(core_tests PROPERTIES
    ENVIRONMENT_MODIFICATION
      "PATH=path_list_prepend:$<TARGET_FILE_DIR:objectbox>;PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin;PATH=path_list_prepend:${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin"
  )
endif()
```

- [ ] **Step 6: Create `tests/test_main.cc`**

Copy `tests/test_main.cc` from the TextFoundry repository (the version
produced by Task 1, Step 2 — engine-only, `core_tests`) into this
repository at the same relative path, `tests/test_main.cc`. Then update
every include path: replace `../src/textfoundry_engine/tf/` with `../tf/`
(the new repository has `tf/` at its root, a sibling of `tests/`, instead
of three directories down). Concretely, this line block near the top:

```cpp
#include "../src/textfoundry_engine/tf/logger.h"
```

becomes:

```cpp
#include "../tf/logger.h"
```

and the block of nine includes further down:

```cpp
#include "../src/textfoundry_engine/tf/block.h"
#include "../src/textfoundry_engine/tf/block_generation.h"
#include "../src/textfoundry_engine/tf/block_ref.h"
#include "../src/textfoundry_engine/tf/block_type.hpp"
#include "../src/textfoundry_engine/tf/composition.h"
#include "../src/textfoundry_engine/tf/engine.h"
#include "../src/textfoundry_engine/tf/error.h"
#include "../src/textfoundry_engine/tf/fragment.h"
#include "../src/textfoundry_engine/tf/renderer.h"
#include "../src/textfoundry_engine/tf/version.h"
```

becomes:

```cpp
#include "../tf/block.h"
#include "../tf/block_generation.h"
#include "../tf/block_ref.h"
#include "../tf/block_type.hpp"
#include "../tf/composition.h"
#include "../tf/engine.h"
#include "../tf/error.h"
#include "../tf/fragment.h"
#include "../tf/renderer.h"
#include "../tf/version.h"
```

No other line in the file changes.

- [ ] **Step 7: Configure and build**

Run, from `/home/a.durynin/Projects/C++/TextFoundryEngine`:
```bash
cmake --preset vcpkg-rel
cmake --build build-rel --parallel
```
Expected: `textfoundry_core` and `core_tests` build with no errors.

- [ ] **Step 8: Run the tests**

Run: `ctest --test-dir build-rel --output-on-failure`
Expected: all cases pass — same count as `core_tests` in TextFoundry after
Task 1's split.

- [ ] **Step 9: Verify install works**

Run:
```bash
cmake --install build-rel --config Release --prefix /tmp/tfengine-stage
```
Expected: succeeds; `/tmp/tfengine-stage/lib/cmake/TextFoundry/TextFoundryConfig.cmake`
and `/tmp/tfengine-stage/include/tf/engine.h` both exist.

- [ ] **Step 10: Commit and push**

```bash
git add CMakeLists.txt cmake/TextFoundryConfig.cmake.in vcpkg.json CMakePresets.json tests/
git commit -m "$(cat <<'EOF'
Stand up standalone build for the extracted engine

Adapts the packaging scaffolding that already existed in the TextFoundry
monorepo (it was already scoped to just this library) into a
self-contained root build, and carries over the engine-only test suite.

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>
EOF
)"
git push origin main
```

---

### Task 4: Add CI to the new repository

**Files (in `/home/a.durynin/Projects/C++/TextFoundryEngine`):**
- Create: `.github/workflows/ci.yml`

**Interfaces:** none — this is a leaf task with no consumers in this plan.

- [ ] **Step 1: Create `.github/workflows/ci.yml`** with this exact
  content — checkout, vcpkg bootstrap, configure, build, test; no Qt, no
  packaging, no publish job:

```yaml
name: CI

on:
  push:
    branches: ["main"]
  pull_request:
    branches: ["main"]

permissions:
  contents: read

env:
  VCPKG_DEFAULT_BINARY_CACHE: ${{ github.workspace }}/.cache/vcpkg/archives
  VCPKG_DOWNLOADS: ${{ github.workspace }}/.cache/vcpkg/downloads
  VCPKG_COMMIT: aa2d37682e3318d93aef87efa7b0e88e81cd3d59

jobs:
  build:
    name: Build and test (Linux)
    runs-on: ubuntu-latest

    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Install build dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y build-essential cmake ninja-build curl git pkg-config

      - name: Restore vcpkg binary cache
        id: cache-vcpkg-binary
        uses: actions/cache/restore@v4
        with:
          path: |
            ${{ env.VCPKG_DEFAULT_BINARY_CACHE }}
            ${{ env.VCPKG_DOWNLOADS }}
          key: ${{ runner.os }}-vcpkg-bincache-${{ hashFiles('vcpkg.json') }}

      - name: Restore vcpkg installed tree
        id: cache-vcpkg-installed
        uses: actions/cache/restore@v4
        with:
          path: ${{ github.workspace }}/build-rel/vcpkg_installed
          key: ${{ runner.os }}-vcpkg-installed-${{ hashFiles('vcpkg.json') }}

      - name: Prepare vcpkg cache directories
        run: |
          mkdir -p "${VCPKG_DEFAULT_BINARY_CACHE}"
          mkdir -p "${VCPKG_DOWNLOADS}"

      - name: Bootstrap vcpkg
        run: |
          git clone https://github.com/microsoft/vcpkg "${{ github.workspace }}/vcpkg"
          git -C "${{ github.workspace }}/vcpkg" fetch --depth 1 origin "${VCPKG_COMMIT}"
          git -C "${{ github.workspace }}/vcpkg" checkout --force "${VCPKG_COMMIT}"
          "${{ github.workspace }}/vcpkg/bootstrap-vcpkg.sh" -disableMetrics
          echo "VCPKG_ROOT=${{ github.workspace }}/vcpkg" >> "$GITHUB_ENV"

      - name: Configure
        run: cmake --preset vcpkg-rel

      - name: Save vcpkg binary cache
        if: steps.cache-vcpkg-binary.outputs.cache-hit != 'true'
        uses: actions/cache/save@v4
        with:
          path: |
            ${{ env.VCPKG_DEFAULT_BINARY_CACHE }}
            ${{ env.VCPKG_DOWNLOADS }}
          key: ${{ steps.cache-vcpkg-binary.outputs.cache-primary-key }}

      - name: Save vcpkg installed tree
        if: steps.cache-vcpkg-installed.outputs.cache-hit != 'true'
        uses: actions/cache/save@v4
        with:
          path: ${{ github.workspace }}/build-rel/vcpkg_installed
          key: ${{ steps.cache-vcpkg-installed.outputs.cache-primary-key }}

      - name: Build
        run: cmake --build build-rel --parallel

      - name: Test
        run: ctest --test-dir build-rel --output-on-failure
```

- [ ] **Step 2: Commit and push**

```bash
git add .github/workflows/ci.yml
git commit -m "$(cat <<'EOF'
Add CI: build and test on Linux

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>
EOF
)"
git push origin main
```

- [ ] **Step 3: Verify the workflow run is green**

Run: `gh run watch --repo Artem535/TextFoundryEngine $(gh run list --repo Artem535/TextFoundryEngine --limit 1 --json databaseId --jq '.[0].databaseId')`
Expected: workflow concludes `success`.

---

### Task 5: Tag `v0.1.0`

**Files:** none.

- [ ] **Step 1: Tag the release commit**

Run, from `/home/a.durynin/Projects/C++/TextFoundryEngine`:
```bash
git tag -a v0.1.0 -m "TextFoundryEngine v0.1.0 — extracted from TextFoundry"
git push origin v0.1.0
```
Expected: tag visible at
`https://github.com/Artem535/TextFoundryEngine/releases/tag/v0.1.0`.

---

### Task 6: Migrate TextFoundry to consume the engine externally

**Files (in `/home/a.durynin/Projects/C++/textfoundry`):**
- Modify: `CMakeLists.txt`
- Delete: `src/textfoundry_engine/` (entire directory)
- Delete: `tests/test_main.cc`
- Modify: `tests/CMakeLists.txt` (remove the `core_tests` target)

**Interfaces:**
- Consumes: `TextFoundry::Core` from the fetched `TextFoundryEngine@v0.1.0`
  — same target name every existing consumer (`textfoundry_ai`,
  `text_foundry_cli`, `text_foundry_gui`, `ai_tests`) already links, so
  none of them need code changes.

- [ ] **Step 1: Replace the in-tree engine with FetchContent**

In `CMakeLists.txt`, replace this line:
```cmake
add_subdirectory(src/textfoundry_engine)
```
with:
```cmake
FetchContent_Declare(
  textfoundry_engine
  GIT_REPOSITORY https://github.com/Artem535/TextFoundryEngine.git
  GIT_TAG v0.1.0
)
FetchContent_MakeAvailable(textfoundry_engine)
```

- [ ] **Step 2: Delete the in-tree engine directory**

Run: `git rm -r src/textfoundry_engine`

- [ ] **Step 3: Remove the now-redundant engine-only test binary**

Run: `git rm tests/test_main.cc`

Rewrite `tests/CMakeLists.txt` to this exact content (keeps only
`ai_tests`, which already links the same `textfoundry_core` target name —
now resolved externally):

```cmake
find_package(doctest CONFIG REQUIRED)

add_executable(ai_tests
        ai_test_main.cc
)

target_link_libraries(ai_tests textfoundry_core textfoundry_ai_openai doctest::doctest)

if(UNIX AND TARGET objectbox)
  set_property(TARGET ai_tests PROPERTY BUILD_WITH_INSTALL_RPATH OFF)
  set_property(TARGET ai_tests APPEND PROPERTY
    BUILD_RPATH "$<TARGET_FILE_DIR:objectbox>"
  )
endif()

add_test(NAME ai_tests COMMAND ai_tests)

if(WIN32 AND TARGET objectbox)
  set_tests_properties(ai_tests PROPERTIES
    ENVIRONMENT_MODIFICATION
      "PATH=path_list_prepend:$<TARGET_FILE_DIR:objectbox>;PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin;PATH=path_list_prepend:${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin"
  )
endif()
```

- [ ] **Step 4: Clean and reconfigure**

Run:
```bash
rm -rf build-rel
cmake --preset vcpkg-rel
```
Expected: configure succeeds; log shows `textfoundry_engine` being fetched
from GitHub at `v0.1.0`.

- [ ] **Step 5: Build**

Run: `cmake --build build-rel --parallel`
Expected: full build succeeds — `textfoundry_ai_openai`, `tf` (CLI),
`ai_tests`, and (if Qt6 is present) `tf-gui` all link against the fetched
`TextFoundry::Core` with no source changes needed in any of them.

- [ ] **Step 6: Test**

Run: `ctest --test-dir build-rel --output-on-failure`
Expected: `ai_tests` passes (this is now the only test binary in
TextFoundry — engine coverage lives in `TextFoundryEngine`'s own CI).

- [ ] **Step 7: Commit**

```bash
git add CMakeLists.txt tests/CMakeLists.txt
git commit -m "$(cat <<'EOF'
Consume the engine from TextFoundryEngine via FetchContent

src/textfoundry_engine/ is gone from this repository; textfoundry_core
now resolves to TextFoundryEngine@v0.1.0, fetched the same way ftxui,
objectbox, and qtkeychain already are. No consumer (textfoundry_ai, the
CLI, the GUI) needed a code change — they all already link the target
by name.

Co-Authored-By: Claude Sonnet 5 <noreply@anthropic.com>
EOF
)"
```

- [ ] **Step 8: Push and verify TextFoundry's own CI is green**

```bash
git push origin main
```
Then check the `CI` workflow run on the TextFoundry repository (Linux,
macOS, and Windows matrix legs) all succeed with the externally-fetched
engine.

## Self-Review Notes

- **Spec coverage:** all 8 Delivery Tasks from
  `docs/superpowers/specs/2026-09-17-engine-repo-extraction-design.md` are
  covered: classify tests → Task 1; extract with history → Task 2; stand up
  build → Task 3; move engine-only tests → Task 3 Step 6; add CI → Task 4;
  tag `v0.1.0` → Task 5; migrate TextFoundry → Task 6; full CI pass → Task
  6 Step 8.
- **Type/name consistency:** target name `textfoundry_core` and export name
  `TextFoundry::Core` are identical across Tasks 1, 3, and 6 — no renames
  introduced anywhere.
- **No task leaves a red `ctest`** — each of Tasks 1, 3, and 6 ends with a
  green test run before its commit step.
