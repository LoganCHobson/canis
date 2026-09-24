#pragma once
#include <imgui_internal.h>

namespace Canis
{
    // Keep tool widgets inside a scene inspector from contributing edited
    // events, including numeric widgets which temporarily override item flags.
    class SceneHistoryIgnoreScope
    {
    public:
        SceneHistoryIgnoreScope()
            : m_context(*ImGui::GetCurrentContext()),
              m_previousEdited(m_context.ActiveIdHasBeenEditedThisFrame) {}
        ~SceneHistoryIgnoreScope()
        {
            m_context.ActiveIdHasBeenEditedThisFrame = m_previousEdited;
        }
        SceneHistoryIgnoreScope(const SceneHistoryIgnoreScope&) = delete;
        SceneHistoryIgnoreScope& operator=(const SceneHistoryIgnoreScope&) = delete;

    private:
        ImGuiContext& m_context;
        bool m_previousEdited;
    };

    // Opt in only around controls which edit scene data. Other panels never
    // request a scene snapshot, even when they set ImGui's global edited flag.
    class SceneHistoryEditScope
    {
    public:
        explicit SceneHistoryEditScope(bool& changed)
            : m_changed(changed), m_context(*ImGui::GetCurrentContext()),
              m_previousEdited(m_context.ActiveIdHasBeenEditedThisFrame),
              m_previousDropTarget(m_context.DragDropAcceptIdCurr)
        {
            m_context.ActiveIdHasBeenEditedThisFrame = false;
        }

        ~SceneHistoryEditScope()
        {
            const bool deliveredDrop = m_context.DragDropPayload.Delivery &&
                m_context.DragDropAcceptIdCurr != m_previousDropTarget;
            m_changed |= m_context.ActiveIdHasBeenEditedThisFrame || deliveredDrop;
            m_context.ActiveIdHasBeenEditedThisFrame |= m_previousEdited;
        }

        SceneHistoryEditScope(const SceneHistoryEditScope&) = delete;
        SceneHistoryEditScope& operator=(const SceneHistoryEditScope&) = delete;

    private:
        bool& m_changed;
        ImGuiContext& m_context;
        bool m_previousEdited;
        ImGuiID m_previousDropTarget;
    };
}
