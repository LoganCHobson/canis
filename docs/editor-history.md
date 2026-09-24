# Editor scene history

Scene history captures the scene and its terrain data after a scene edit finishes.
It does not watch ImGui edits globally. Console/search controls, script documents,
project settings, release options, asset editors, profiler controls, and viewport
tool preferences do not request scene snapshots.

Entity property controls and environment controls opt in through
`SceneHistoryEditScope` in `src/EditorHistoryEvent.hpp`. The scope records edited
widgets and drag/drop delivery within that block. The editor retains the pending
change through a drag or text edit and captures it when the interaction ends.
Component headings and the Add Component search dialog are outside these scopes.

Commands that change a scene without editing a property widget must call
`Editor::NotifySceneChanged()` after applying the change. This includes custom
inspector buttons, component attachment/removal, and assignments from asset
editor windows. Merely opening a popup, selecting an entity, or selecting a tool
must not notify scene history. Operations already using
`CommitSceneHistoryImmediateChange()` retain their own transaction boundary.

Tool-only widgets embedded in a native component inspector use
`SceneHistoryIgnoreScope` to avoid reporting a scene edit. Their normal widget
return values and interaction behavior remain available. Asset editing and
animation preview retain their existing save/restore behavior; this change does
not add an asset undo system.

Regression coverage lives in `EditorHistoryEventTests.cpp` (scope isolation,
checkbox activation, inspector tool settings, drops, idle frames) and
`SceneTabsTests.cpp` (no snapshot for unrelated edits, scene command undo/redo,
and scene tab history).
