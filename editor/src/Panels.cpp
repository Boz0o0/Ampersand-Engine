#include <imgui.h>

#include <editor/Editor.hpp>
#include <filesystem>

namespace ampersand::editor {

void Editor::drawFiles() {
    ImGui::Begin("Files");

    if (this->_project == std::nullopt) return;
    if (ImGui::Button("Refresh")) {
        this->_project->refreshFiles();
    }
    ImGui::Separator();
    for (const FileNode& node : this->_project->getFiles()) {
        drawFileNode(node);
    }

    ImGui::End();
}

void Editor::drawFileNode(const FileNode& node) {
    std::string assetPathPayload = "ASSET_PATH";

    if (node.directory) {
        if (ImGui::TreeNodeEx(node.name.c_str(),
                              ImGuiTreeNodeFlags_OpenOnArrow)) {
            for (const FileNode& child : node.children) {
                drawFileNode(child);
            }
            ImGui::TreePop();
        }
        return;
    }
    ImGui::TreeNodeEx(
        node.name.c_str(),
        ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
    if (ImGui::BeginDragDropSource()) {
        const std::string path = node.path.generic_string();
        ImGui::SetDragDropPayload(assetPathPayload.c_str(), path.c_str(),
                                  path.size() + 1);
        ImGui::TextUnformatted(path.c_str());
        ImGui::EndDragDropSource();
    }
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
