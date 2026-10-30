#include <imgui.h>

#include <editor/Editor.hpp>

namespace ampersand::editor {

void Editor::drawFiles() {
    ImGui::Begin("Files");

    ImGui::TextUnformatted("Assets");
    ImGui::Separator();

    ImGui::BulletText("Textures");
    ImGui::BulletText("Models");
    ImGui::BulletText("Scenes");

    ImGui::End();
}

void Editor::drawHierarchy() {
    ImGui::Begin("Hierarchy");

    ImGui::TextUnformatted("Scene Objects");
    ImGui::Separator();

    if (ImGui::Selectable("Camera")) {
        // Select camera
    }
    if (ImGui::Selectable("Player")) {
        // Select player
    }
    if (ImGui::Selectable("Light")) {
        // Select light
    }

    ImGui::End();
}

void Editor::drawViewport() {
    ImGui::Begin("Viewport");
    ImGui::TextUnformatted("Viewport not available yet !");
    ImGui::End();
}

void Editor::drawInspector() {
    ImGui::Begin("Inspector");

    ImGui::TextUnformatted("Selected Object");
    ImGui::Separator();
    ImGui::TextDisabled("Nothing selected.");

    ImGui::End();
}

}  // namespace ampersand::editor
