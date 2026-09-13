#include "Panels/SceneHierarchy.h"
#include <imgui.h>

using namespace Nova;

struct SceneHierarchyState
{
    Entity selection_context = Stub_Entity;
    bool should_display = true;
};

static SceneHierarchyState state;

namespace SceneHierarchyPanel
{
    void SetSelectionContext(Nova::Entity entity) { state.selection_context = entity; }
    void Display()
    {
        if (!state.should_display)
            return;

        ImGui::Begin("Scene Hierarchy", &state.should_display);
        ImGui::End();
    }
}
