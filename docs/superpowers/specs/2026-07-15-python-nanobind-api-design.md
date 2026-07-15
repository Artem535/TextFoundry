# Python nanobind API Design

## Goal

Expose TextFoundry's core authoring workflow to Python without changing the
C++ GUI or CLI. Python clients must be able to create an in-memory engine,
manage versioned blocks and compositions, render deterministically, and invoke
the existing OpenAI-compatible AI workflows.

## Scope

Add a `textfoundry` extension module built by CMake in
`src/textfoundry_python`. It links the existing `textfoundry_core` and
`textfoundry_ai_openai` libraries and uses nanobind; it does not duplicate
domain or HTTP logic in Python.

The module exposes:

- value and enum types needed for normal authoring: `Version`, `BlockType`,
  parameters, block/composition drafts and published values;
- `Engine` construction with an explicit in-memory data path, lifecycle
  operations, and deterministic render operations;
- OpenAI-compatible configuration and explicit generation, normalization, and
  composition-rewrite operations already provided by C++ AI adapters.

Qt/QML targets, FTXUI/CLI targets, packaging frontends, and alternate Python
HTTP implementations are out of scope.

## API Boundary and Errors

Bindings form a thin translation layer. Python-visible objects map directly to
the stable C++ domain types, while repositories, ObjectBox model details, and
raw C++ pointers remain internal. Operations that return `tf::Result<T>` raise
a module-specific Python exception carrying the TextFoundry error code and
message. Non-result operations return their bound value directly.

AI remains explicit: binding an adapter never makes deterministic rendering
perform a network call. Adapter configuration accepts ordinary Python values;
AI calls use the same C++ request and response types as the GUI and CLI.

## Build and Distribution

`find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)` and
nanobind are wired into the root CMake build. The binding target is built only
when the required Python development components and nanobind are available.
The vcpkg manifest records nanobind. The extension output is importable from
the build tree for tests; packaging a wheel is deliberately deferred.

## Delivery Tasks

1. **Add build dependencies.** Add nanobind to the vcpkg manifest and a
   root-level option for the Python module. Discover nanobind and Python
   development components so a missing Python toolchain cleanly disables only
   the bindings.
2. **Create the binding target.** Add `src/textfoundry_python` and an
   importable `textfoundry` extension linked to `textfoundry_core` and
   `textfoundry_ai_openai`; make its build-tree output available to CTest.
3. **Bind deterministic domain types.** Expose versions, enums, parameters,
   drafts, published values, blocks, compositions, and renderer-facing types
   needed for the standard authoring workflow.
4. **Bind engine operations and errors.** Expose in-memory `Engine`
   construction, block/composition lifecycle, and render operations. Convert
   every surfaced `tf::Result<T>` failure into a documented module exception.
5. **Bind explicit AI workflows.** Expose OpenAI-compatible configuration and
   C++ generation, normalization, and composition-rewrite entry points without
   introducing implicit network calls from render operations.
6. **Add Python integration tests.** Cover publish → compose → render, a
   domain-error exception, and AI adapter interaction through a fake transport;
   register all tests with CTest and keep them offline.
7. **Document use and validate the build.** Add a minimal Python example,
   build with the standard preset, run CTest, and verify importing the built
   module.

## Verification

Add Python tests run through CTest. A core integration test publishes a block,
creates a composition, renders it with parameters, and verifies a domain-error
exception. An AI adapter test uses the existing fake transport seam to assert
that an explicit Python AI request reaches the C++ adapter without live
network access. Configure/build tests cover bindings where Python is present;
the existing C++ targets continue to build without GUI support.
