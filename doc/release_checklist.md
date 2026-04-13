# Release Checklist

## Packaging

- Build `core_tests`, `tf`, and `tf-gui` from a clean tree.
- Run `ctest --test-dir build-rel --output-on-failure -R core_tests`.
- Verify `cmake --install build-rel --prefix <staging-dir>` installs:
  - `bin/tf`
  - `bin/tf-gui`
  - `share/applications/textfoundry.desktop`
  - `share/metainfo/textfoundry.metainfo.xml`
  - `share/icons/hicolor/scalable/apps/textfoundry.svg`
- Launch `tf-gui` from the installed staging directory and confirm the window icon and desktop entry metadata.

## Product Smoke Test

- Create a block and save a new version.
- Open multiple block editor tabs and confirm unsaved-changes warnings work on close/navigation.
- Open block version history and switch between versions.
- Verify `Expand All` / `Collapse All` work in the blocks tree.
- Deprecate and delete a block, then confirm tree state does not jump or fully reset on single-item updates.
- Create a composition, edit fragments, and save.
- Delete a composition through the confirmation dialog.
- Use `AI Slice` on a tagged prompt and publish blocks plus composition.
- Use `AI Slice` with an existing namespace and confirm blocks update existing ids instead of creating `_2`, `_3`, etc.
- Render a composition with runtime params.
- Run text-level `Normalize` on the render page.
- Run composition-level `Normalize Style` on the compositions page.
- Open `Compare Raw` for two composition versions and verify side-by-side raw prompt diff renders correctly.
- Run `Rewrite Blocks`, preview AI-generated patches, and apply them as a new composition version.
- Restart the app and confirm AI settings reload correctly.

## AI Workflows

- Verify `base URL` and `model` persist across restarts.
- Verify `API key` is read back from system key storage.
- Verify `HTTP timeout` persists across restarts and affects rebuilt AI connections.
- Confirm block generation fails clearly on invalid provider response.
- Confirm `AI Rewrite` for an existing block updates the current block version instead of creating a new block id.
- Confirm composition normalization preserves placeholders and does not silently drop required params.
- Confirm `Rewrite Blocks` preserves block ids, composition order, and placeholders.
- Confirm structured-output parsing errors log enough raw provider response to debug model incompatibility.

## Release Quality

- Check app icon readability at `256`, `64`, `32`, and `24` px.
- Check `Render` layout on both wide and narrow windows.
- Verify AI actions use consistent icons and labels:
  - `AI Rewrite`
  - `Slice Into Blocks`
  - `Rewrite Blocks`
  - `Normalize Style`
- Verify compare dialog colors and line numbers remain readable in the active theme.
- Review `PRD` entries for the new block types and AI workflows.

## Known Follow-ups

- The install smoke test may require running outside the sandbox because CMake writes `install_manifest.txt` into the build directory.
- Add screenshots to AppStream metadata before publishing to desktop software centers.
- Decide on the final `project_license` value in AppStream metadata before release.
- Consider replacing the current compare row renderer with a more advanced diff widget if intra-line highlighting or chunk folding becomes necessary.
