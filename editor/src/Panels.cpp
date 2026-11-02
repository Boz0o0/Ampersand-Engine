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
    if (ImGui::IsItemHovered() &&
        ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) &&
        node.path.extension() == ".json") {
        this->openScene(node.path);
    }
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

    if (this->_project == std::nullopt) return;
    const std::optional<Scene>& scene = this->_project->getScene();
    if (scene == std::nullopt) {
        ImGui::TextDisabled("Double-click a scene file to open it.");
        ImGui::End();
        return;
    }
    ImGui::TextUnformatted(scene->name.c_str());
    ImGui::Separator();
    int index = 0;
    for (const auto& entity : scene->document.data.at("entities")) {
        drawEntityNode(entity, index++);
    }

    ImGui::End();
}

void Editor::drawEntityNode(const nlohmann::ordered_json& entity, int index) {
    if (!entity.is_object()) {
        return;
    }
    std::string name = entity.value("name", "");
    if (name.empty() && entity.contains("prefab")) {
        name =
            std::filesystem::path(entity.value("prefab", "")).stem().string();
    }
    if (name.empty()) {
        name = "Entity";
    }
    const auto children = entity.find("children");
    const bool parent =
        children != entity.end() && children->is_array() && !children->empty();

    ImGui::PushID(index);
    if (!parent) {
        ImGui::TreeNodeEx(
            name.c_str(),
            ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
    } else if (ImGui::TreeNodeEx(name.c_str(),
                                 ImGuiTreeNodeFlags_OpenOnArrow)) {
        int childIndex = 0;
        for (const auto& child : *children) {
            drawEntityNode(child, childIndex++);
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
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
