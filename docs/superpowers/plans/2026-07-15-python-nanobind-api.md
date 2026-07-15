# Python nanobind API Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` (recommended) or `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build an importable `textfoundry` Python extension over the existing C++ core and explicit OpenAI-compatible workflows.

**Architecture:** A single nanobind module owns only Python/C++ conversion and exception translation. It links existing static libraries; a binding-local factory constructs the existing Qt transport and adapters, so Python never sees repository or transport interfaces.

**Tech Stack:** C++23, CMake 3.20, vcpkg, nanobind, Python Development.Module, doctest/CTest.

## Global Constraints

- Keep Qt/QML and CLI targets C++-only.
- Do not duplicate domain, render, or HTTP implementation in Python.
- AI calls remain explicit; `Engine.Render` must not initiate network I/O.
- Translate every bound `tf::Result<T>` error into `textfoundry.Error` with `code` and `message`.
- All new AI tests use a fake transport; no live credentials or network.
- Use `rtk` for repository, configure, build, and test commands.

---

### Task 1: Build integration and empty module

**Files:** Modify `vcpkg.json`, `CMakeLists.txt`, `src/textfoundry_engine/CMakeLists.txt`, `src/textfoundry_ai/CMakeLists.txt`; create `src/textfoundry_python/CMakeLists.txt`, `src/textfoundry_python/module.cc`.

- [ ] Add `nanobind` to `dependencies`; add option `TEXTFOUNDRY_BUILD_PYTHON` default `ON`.
- [ ] When enabled, use `find_package(Python COMPONENTS Interpreter Development.Module QUIET)` and `find_package(nanobind CONFIG QUIET)`; add the Python subdirectory only when both targets exist, otherwise issue `STATUS` and preserve all C++ targets.
- [ ] Set `POSITION_INDEPENDENT_CODE ON` on `textfoundry_core` and `textfoundry_ai_openai`, then create `nanobind_add_module(textfoundry module.cc)` linked to both libraries.
- [ ] Start `module.cc` with `NB_MODULE(textfoundry, m) { m.attr("__version__") = "0.2.7"; }` and set the module output directory to `${CMAKE_BINARY_DIR}/python`.
- [ ] Configure with `rtk cmake --preset vcpkg-rel`, build target `textfoundry`, and run `PYTHONPATH=build-rel/python python3 -c 'import textfoundry; assert textfoundry.__version__ == "0.2.7"'`.
- [ ] Commit: `Add nanobind module target`.

### Task 2: Core values, builders, and exception boundary

**Files:** Modify `src/textfoundry_python/module.cc`; create `tests/python/test_core_api.py`; modify `tests/CMakeLists.txt`.

- [ ] First add `tests/python/test_core_api.py` asserting `Version(2, 5).to_string() == "2.5"`, `BlockDraftBuilder("role.greeter")` can set template/defaults and build, and an error exposes `.code` and `.message`.
- [ ] Bind `ErrorCode`, `Version` (`major`, `minor`, `to_string`), `BlockType`, `BlockState`, `SeparatorType`, `ParamSchema`, `Template`, `RenderContext`, `StructuralStyle`, `SemanticStyle`, and `StyleProfile` with Python `dict[str, str]` parameters and `None` mapped to C++ optionals.
- [ ] Bind `BlockDraftBuilder` and `CompositionDraftBuilder` with fluent methods returning `self`; bind published results read-only. Do not bind mutable repositories or raw `Fragment` variants in this task.
- [ ] Define exception `textfoundry.Error`; add one templated `unwrap(Result<T>)` helper that raises it with an enum-valued `code` and message, plus `raise_error(Error)` for non-success `Error` values.
- [ ] Register a CTest named `python_core_api` that invokes Python with `PYTHONPATH` set to the module output directory; run it and `rtk ctest --test-dir build-rel --output-on-failure`.
- [ ] Commit: `Bind Python core value types`.

### Task 3: Engine lifecycle and deterministic rendering

**Files:** Modify `src/textfoundry_python/module.cc`, `tests/python/test_core_api.py`.

- [ ] Add a failing test that constructs `Engine(data_path="memory:python-core")`, publishes `role.greeting` with template `Hello, {{name}}!`, adds its published ref to a composition, publishes it, and asserts `engine.render("demo", {"name": "Ada"}).text == "Hello, Ada!"`.
- [ ] Bind `EngineConfig` as snake_case Python properties and an `Engine` constructor that calls `FullInit()` after setting the supplied in-memory path.
- [ ] Bind `publish_block`, `update_block`, `load_block`, block version/list/delete/deprecate methods; bind equivalent composition methods, `render`, `render_block`, validation, and `RenderResult`.
- [ ] Implement overload adapters explicitly (`version: Version | None`, `context: dict[str, str] | RenderContext`) rather than exposing C++ overload resolution. Verify `render_block("missing")` raises `textfoundry.Error` with `ErrorCode.BlockNotFound`.
- [ ] Re-run `python_core_api` and the full CTest suite; commit: `Expose engine render workflow to Python`.

### Task 4: Explicit OpenAI-compatible configuration

**Files:** Modify `src/textfoundry_python/module.cc`; create `tests/python/test_ai_api.py`; modify `tests/CMakeLists.txt`.

- [ ] Bind `OpenAiCompatibleConfig` but never expose `api_key` through repr or logging. Add `Engine.configure_openai(config, timeout_ms=30000, http2_allowed=True)` that creates one shared `QtHttpTransport`, then installs one generator, normalizer, and composition rewriter on the engine.
- [ ] Bind all request/result value types used by `GenerateBlockData`, batch generation, normalization previews/results, and rewrite previews/results. Bind `has_block_generator`, `has_block_normalizer`, and `has_composition_block_rewriter`.
- [ ] Add test coverage for `configure_openai` setting these three flags without sending HTTP; assert deterministic `render` still succeeds after configuration.
- [ ] Add a C++-only binding test seam `textfoundry._testing.FakeTransport` guarded by `TEXTFOUNDRY_BUILD_TESTING`, or keep the fake transport in a test-only extension. It must capture request data and return fixture JSON; production module exports no transport interface.
- [ ] Test one generator request and one invalid HTTP response through the fake transport; assert a Python `Error` rather than a crash or live request.
- [ ] Run `python_ai_api` and full CTest; commit: `Expose explicit OpenAI workflows to Python`.

### Task 5: Documentation and release verification

**Files:** Modify `README.md`; create `examples/python_api_example.py`.

- [ ] Add a minimal example performing the Task 3 publish/compose/render flow and printing the rendered text; do not include credentials.
- [ ] Document configure/build/import commands, required Python development package, `PYTHONPATH=build-rel/python`, and the explicit-AI rule.
- [ ] Run `rtk cmake --preset vcpkg-rel`, `rtk cmake --build build-rel --parallel`, `rtk ctest --test-dir build-rel --output-on-failure`, and the example. Record command outputs in the commit message/PR description.
- [ ] Run `rtk git diff --check`; commit: `Document Python API usage`.

## Plan Self-Review

- Spec coverage: Tasks 1–5 cover dependency discovery, importable module, deterministic types/engine/error boundary, explicit C++ AI adapters, offline CTest coverage, and usage documentation.
- No-placeholder check: every task names exact files, public API, validation command, and commit boundary.
- Type consistency: Python `Engine.configure_openai` is the only adapter installation entry point; all result-returning public methods use the Task 2 exception helper.
