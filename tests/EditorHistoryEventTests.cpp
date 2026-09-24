#include "../src/EditorHistoryEvent.hpp"
#include <iostream>
#include <stdexcept>

static void Check(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

int main()
{
    ImGui::CreateContext();
    try
    {
        auto& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.DisplaySize = ImVec2(800, 600);
        unsigned char* pixels; int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        auto& context = *ImGui::GetCurrentContext();
        bool changed = false;
        // A new panel is excluded by default; no window-name or ID denylist.
        for (const char* panel : {"Console", "Assets", "Scripts", "ProjectSettings",
             "Release", "Input Actions", "Shader Graph", "Animation", "Future panel"})
        {
            ImGui::NewFrame();
            ImGui::Begin(panel);
            const ImGuiID id = ImGui::GetID("Checkbox or search");
            ImGui::SetActiveID(id, ImGui::GetCurrentWindow());
            ImGui::MarkItemEdited(id);
            ImGui::End();
            ImGui::Begin("Inspector");
            {
                Canis::SceneHistoryEditScope properties(changed);
                // Merely transferring focus from an edited search is not an edit.
                ImGui::ClearActiveID();
            }
            Check(!changed, "Unrelated panel requested scene history");
            ImGui::End(); ImGui::Render();
        }

        ImGui::NewFrame(); ImGui::Begin("Inspector");
        const ImGuiID id = ImGui::GetID("Enabled");
        {
            Canis::SceneHistoryEditScope properties(changed);
            ImGui::SetActiveID(id, ImGui::GetCurrentWindow());
        }
        Check(!changed, "Unedited click requested scene history");
        {
            Canis::SceneHistoryEditScope properties(changed);
            ImGui::MarkItemEdited(id);
        }
        Check(changed, "Scene property edit was lost");
        changed = false;
        {
            Canis::SceneHistoryEditScope properties(changed);
        }
        Check(!changed, "Second component inherited first component's edit");
        {
            Canis::SceneHistoryEditScope properties(changed);
            ImGui::ClearActiveID();
            // Checkbox/Selectable widgets mark edited after releasing their ID.
            ImGui::MarkItemEdited(id);
        }
        Check(changed, "Release-frame checkbox edit was lost");
        changed = false;
        bool enabled = false;
        const ImGuiID realCheckbox = ImGui::GetID("Actual scene checkbox");
        context.NavId = realCheckbox;
        context.NavActivateId = realCheckbox;
        context.NavActivateDownId = realCheckbox;
        context.NavActivatePressedId = realCheckbox;
        context.NavInputSource = ImGuiInputSource_Keyboard;
        {
            Canis::SceneHistoryEditScope properties(changed);
            ImGui::Checkbox("Actual scene checkbox", &enabled);
        }
        Check(enabled && changed, "Actual checkbox activation did not record a scene edit");
        ImGui::ClearActiveID();
        context.NavActivateId = context.NavActivateDownId = context.NavActivatePressedId = 0;
        changed = false;
        {
            Canis::SceneHistoryEditScope properties(changed);
            Canis::SceneHistoryIgnoreScope toolSettings;
            ImGui::Button("Tool setting");
            ImGui::MarkItemEdited(ImGui::GetItemID());
        }
        Check(!changed, "Inspector tool setting requested scene history");
        {
            Canis::SceneHistoryEditScope drop(changed);
            context.DragDropAcceptIdCurr = id;
            context.DragDropPayload.Delivery = true;
        }
        Check(changed, "Scene drop was lost");
        changed = false;
        {
            Canis::SceneHistoryEditScope unrelatedProperties(changed);
        }
        Check(!changed, "Previously delivered asset drop leaked into scene history");
        context.DragDropPayload.Delivery = false;
        ImGui::End(); ImGui::Render();
        for (int i = 0; i < 120; ++i)
        {
            ImGui::NewFrame(); ImGui::Begin("Inspector");
            { Canis::SceneHistoryEditScope properties(changed); }
            Check(!changed, "Idle frame repeats history capture");
            ImGui::End(); ImGui::Render();
        }
        ImGui::DestroyContext();
        std::cout << "Opt-in scene edits, unrelated panels, tool settings, drops and idle frames passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        ImGui::DestroyContext();
        return 1;
    }
}
