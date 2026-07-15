# Python API Examples Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task.

**Goal:** Add deterministic and offline AI Python examples and document how to run them.

**Architecture:** Keep examples thin clients of the existing nanobind API. The deterministic example uses only the engine; the AI example uses the testing fake transport and never requires credentials or network access.

**Tech Stack:** Python 3.12 (or the interpreter selected by CMake), nanobind `textfoundry` module, CMake/vcpkg, CTest.

## Global Constraints

- Deterministic `Engine.render` must never invoke AI or network I/O.
- AI setup must remain explicit through `Engine.configure_openai`.
- Examples must run without real credentials or external services.
- Use the interpreter selected by CMake for the built extension.

### Task 1: Add offline AI example

**Files:**
- Create: `examples/python_ai_example.py`
- Modify: `README.md`

- [ ] Build an `_testing.FakeTransport`, configure a fixture response, call block generation, and print the generated block id.
- [ ] Demonstrate normalization, slicing, and composition rewrite with fixture responses, asserting no network is required.
- [ ] Document both example commands and the `_testing`/offline constraint.
- [ ] Run both examples with `PYTHONPATH=build-rel/python` and the CMake-selected Python; run CTest and `rtk git diff --check`.
- [ ] Commit as `Add Python API usage examples`.

### Task 2: Review and verify examples

**Files:**
- Modify: `examples/python_api_example.py` only if review finds drift.

- [ ] Check imports, public API names, output, and absence of credentials.
- [ ] Re-run release build, CTest, both examples, and diff checks.
- [ ] Commit any focused fixes.
