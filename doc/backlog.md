# Backlog

## TUI: Rich Text Editing For Multiline Inputs

Status: Planned

Priority: Large

Problem:
The current TUI relies on `ftxui::Input` for multiline editing. This is good
enough for basic typing, but it does not provide a proper text-editor model for
selection, range deletion, and reliable cursor behavior in all multiline cases.

Scope:
- Add keyboard text selection for multiline editors, including `Shift+Arrow`,
  `Shift+Home`, and `Shift+End`.
- Support deleting selected ranges with `Backspace` and `Delete`.
- Render cursor and selection explicitly instead of relying on default
  `ftxui::Input` behavior for multiline fields.
- Preserve vertical scrolling, wrapped preview behavior, and current modal UX.
- Apply the new editor first to the block template editor, then reuse it for
  other multiline TUI fields if successful.

Non-goals:
- Rebuilding the whole TUI input system at once.
- Introducing full mouse-based editing parity from the start.
- Changing engine or persistence behavior.

Implementation notes:
- This likely requires a custom editor component instead of plain
  `ftxui::Input`.
- The editor should own explicit state such as text buffer, cursor position,
  selection anchor, and preferred visual column.
- Editing logic should be separated from rendering, so the same component can be
  reused in multiple TUI dialogs.

Suggested phases:
1. Introduce a dedicated multiline editor state/model.
2. Implement cursor movement and selection expansion with keyboard shortcuts.
3. Implement range deletion and replacement on typed input.
4. Render cursor and selection deterministically in the TUI.
5. Integrate into the block template editor.
6. Evaluate rollout to composition description and static text editors.
