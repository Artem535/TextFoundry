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
- Open block version history and switch between versions.
- Create a composition, edit fragments, and save.
- Use `AI Slice` on a tagged prompt and publish blocks plus composition.
- Render a composition with runtime params.
- Run text-level `Normalize` on the render page.
- Run composition-level `Normalize Composition` on the compositions page.
- Restart the app and confirm AI settings reload correctly.

## AI Workflows

- Verify `base URL` and `model` persist across restarts.
- Verify `API key` is read back from system key storage.
- Confirm block generation fails clearly on invalid provider response.
- Confirm composition normalization preserves placeholders and does not silently drop required params.

## Release Quality

- Check app icon readability at `256`, `64`, `32`, and `24` px.
- Check `Render` layout on both wide and narrow windows.
- Verify AI actions use consistent icons and labels.
- Review `PRD` entries for the new block types and AI workflows.

## Known Follow-ups

- The install smoke test may require running outside the sandbox because CMake writes `install_manifest.txt` into the build directory.
- Add screenshots to AppStream metadata before publishing to desktop software centers.
- Decide on the final `project_license` value in AppStream metadata before release.
