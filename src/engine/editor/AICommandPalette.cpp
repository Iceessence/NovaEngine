#include "AICommandPalette.h"
#include "engine/ai/Memory.h"
#include "engine/ai/Proposals.h"
#include <imgui.h>
#include <string>

namespace nova::ai {
static bool s_open=false;
void DrawAICommandPalette(Editor&){
    if (ImGui::IsKeyPressed(ImGuiKey_P)) s_open = !s_open;
    if (!s_open) return;
    ImGui::Begin("AI Command Palette", &s_open);
    static char buf[1024] = {};
    ImGui::InputTextMultiline("##cmd", buf, IM_ARRAYSIZE(buf), ImVec2(-1, 120));
    if (ImGui::Button("Propose Change (no auto-apply)")) {
        std::string request = buf;
        Memory::Append("user", request);
        auto prop = Proposals::CreateFromRequest(request);
        Memory::Append("assistant", "PROPOSAL " + prop.id + ": " + prop.summaryPath);
        ImGui::TextColored(ImVec4(0,1,0,1), "Created proposal: %s", prop.id.c_str());
        buf[0] = 0;
    }
    ImGui::Separator();
    ImGui::TextDisabled("Press P to toggle. Type natural language commands.");
    ImGui::End();
}
}
