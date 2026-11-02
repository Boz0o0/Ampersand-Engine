#include <imgui.h>

#include <algorithm>
#include <cstring>
#include <editor/Editor.hpp>
#include <filesystem>

namespace ampersand::editor {

void Editor::drawFiles() {
    ImGui::Begin("Files");

    if (!this->_project) {
        ImGui::End();
        return;
    }
    Project& project = *this->_project;
    if (ImGui::Button("Refresh")) {
        project.refreshFiles();
    }
    ImGui::Separator();
    for (const FileNode& node : project.getFiles()) {
        drawFileNode(node);
    }

    ImGui::End();
}

void Editor::drawFileNode(const FileNode& node) {
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
        setPayload(kAssetPayload, path);
        ImGui::TextUnformatted(path.c_str());
        ImGui::EndDragDropSource();
    }
}

void Editor::drawHierarchy() {
    ImGui::Begin("Hierarchy");

    if (!this->_project) {
        ImGui::End();
        return;
    }
    std::optional<Scene>& scene = this->_project->getScene();
    if (!scene) {
        ImGui::TextDisabled("Double-click a scene file to open it.");
        ImGui::End();
        return;
    }
    const std::string title = scene->name + (scene->document.dirty ? " *" : "");
    ImGui::TextUnformatted(title.c_str());
    ImGui::Separator();
    const std::string world = std::string("World") +
                              (this->_project->getConfig().dirty ? " *" : "") +
                              "###World";
    this->drawSelectable(world, {
                                    .scene = scene->name,
                                    .pointer = Pointer(),
                                });
    this->drawSelectable("Scene resources",
                         {
                             .scene = scene->name,
                             .pointer = Pointer("/resources"),
                         });
    ImGui::Separator();
    nlohmann::ordered_json& entities = scene->document.data["entities"];
    if (entities.is_array()) {
        const Pointer root("/entities");
        for (std::size_t index = 0; index < entities.size(); ++index) {
            this->drawEntity(entities[index], root / index);
        }
    }
    this->drawHierarchyBackground();
    this->applyHierarchyAction();
    this->drawRenamePopup();

    ImGui::End();
}

void Editor::drawSelectable(const std::string& label, const Selection& item) {
    const bool open = ImGui::TreeNodeEx(
        label.c_str(), treeFlags(true, this->_selection == item));
    if (ImGui::IsItemClicked()) {
        this->_selection = item;
    }
    if (open) {
        ImGui::TreePop();
    }
}

// NOLINTBEGIN(bugprone-signed-bitwise): ImGui flags are signed
ImGuiTreeNodeFlags Editor::treeFlags(bool leaf, bool selected) {
    return ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth |
           (leaf ? ImGuiTreeNodeFlags_Leaf : ImGuiTreeNodeFlags_None) |
           (selected ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None);
}
// NOLINTEND(bugprone-signed-bitwise)

void Editor::drawEntity(nlohmann::ordered_json& entity,
                        const Pointer& pointer) {
    if (!entity.is_object() || !this->_project) {
        return;
    }
    const std::optional<Scene>& scene = this->_project->getScene();
    if (!scene) {
        return;
    }
    const Selection item{
        .scene = scene->name,
        .pointer = pointer,
    };
    const std::size_t index = std::stoul(pointer.back());
    const bool prefab = entity.contains("prefab");
    const ImGuiTreeNodeFlags flags =
        treeFlags(!this->hasChildren(entity, {}), this->_selection == item);

    ImGui::PushID(static_cast<int>(index));
    if (prefab) {
        ImGui::PushStyleColor(ImGuiCol_Text, kPrefabColor);
    }
    const bool open =
        ImGui::TreeNodeEx(this->entityLabel(entity, index).c_str(), flags);
    if (prefab) {
        ImGui::PopStyleColor();
    }
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        this->_selection = item;
    }
    this->drawEntityMenu(pointer);
    this->drawEntityDragDrop(pointer);
    if (open) {
        std::vector<std::string> chain;
        this->drawPrefabChildren(entity, chain);
        auto children = entity.find("children");
        if (children != entity.end() && children->is_array()) {
            for (std::size_t child = 0; child < children->size(); ++child) {
                this->drawEntity((*children)[child],
                                 pointer / "children" / child);
            }
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void Editor::setPayload(const std::string& type, const std::string& text) {
    ImGui::SetDragDropPayload(type.c_str(), text.data(), text.size());
}

std::string Editor::payloadText(const ImGuiPayload& payload) {
    std::string text(static_cast<std::size_t>(payload.DataSize), '\0');
    std::memcpy(text.data(), payload.Data, text.size());
    return text;
}

void Editor::drawEntityMenu(const Pointer& pointer) {
    using Kind = HierarchyAction::Kind;
    if (!ImGui::BeginPopupContextItem()) {
        return;
    }
    if (ImGui::MenuItem("Create Empty")) {
        this->_hierarchyAction = {
            .kind = Kind::CreateEmpty,
        };
    }
    if (ImGui::MenuItem("Create Child")) {
        this->_hierarchyAction = {
            .kind = Kind::CreateEmpty,
            .target = pointer,
        };
    }
    if (ImGui::MenuItem("Duplicate")) {
        this->_hierarchyAction = {
            .kind = Kind::Duplicate,
            .source = pointer,
        };
    }
    if (ImGui::MenuItem("Delete")) {
        this->_hierarchyAction = {
            .kind = Kind::Delete,
            .source = pointer,
        };
    }
    if (ImGui::MenuItem("Rename")) {
        this->_hierarchyAction = {
            .kind = Kind::Rename,
            .source = pointer,
        };
    }
    ImGui::EndPopup();
}

void Editor::drawEntityDragDrop(const Pointer& pointer) {
    if (ImGui::BeginDragDropSource()) {
        const std::string path = pointer.to_string();
        setPayload(kEntityPayload, path);
        ImGui::TextUnformatted(path.c_str());
        ImGui::EndDragDropSource();
    }
    if (!ImGui::BeginDragDropTarget()) {
        return;
    }
    const ImGuiPayload* dragged = ImGui::GetDragDropPayload();
    if (dragged != nullptr && dragged->IsDataType(kEntityPayload.c_str())) {
        const Pointer source(payloadText(*dragged));
        if (!Project::isInside(pointer, source) &&
            ImGui::AcceptDragDropPayload(kEntityPayload.c_str()) != nullptr) {
            this->_hierarchyAction = {
                .kind = HierarchyAction::Kind::Move,
                .source = source,
                .target = pointer,
            };
        }
    }
    ImGui::EndDragDropTarget();
}

void Editor::drawHierarchyBackground() {
    const ImVec2 available = ImGui::GetContentRegionAvail();
    ImGui::Dummy({available.x, std::max(available.y, kDropZoneHeight)});
    if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload* payload =
            ImGui::AcceptDragDropPayload(kEntityPayload.c_str());
        if (payload != nullptr) {
            this->_hierarchyAction = {
                .kind = HierarchyAction::Kind::Move,
                .source = Pointer(payloadText(*payload)),
            };
        }
        ImGui::EndDragDropTarget();
    }
    if (ImGui::BeginPopupContextWindow("HierarchyMenu",
                                       ImGuiPopupFlags_MouseButtonRight |
                                           ImGuiPopupFlags_NoOpenOverItems)) {
        if (ImGui::MenuItem("Create Empty")) {
            this->_hierarchyAction = {
                .kind = HierarchyAction::Kind::CreateEmpty,
            };
        }
        ImGui::EndPopup();
    }
}

void Editor::applyHierarchyAction() {
    using Kind = HierarchyAction::Kind;
    if (!this->_hierarchyAction || !this->_project) {
        return;
    }
    Project& project = *this->_project;
    const HierarchyAction action = std::move(*this->_hierarchyAction);
    this->_hierarchyAction.reset();
    bool changed = false;
    switch (action.kind) {
        case Kind::CreateEmpty:
            changed = project.createEntity(action.target);
            break;
        case Kind::Duplicate:
            changed = project.duplicateEntity(action.source);
            break;
        case Kind::Delete:
            changed = project.deleteEntity(action.source);
            break;
        case Kind::Move:
            changed = project.moveEntity(action.source, action.target);
            break;
        case Kind::Rename: {
            const std::optional<Scene>& scene = project.getScene();
            const std::string name =
                scene && scene->document.data.contains(action.source)
                    ? scene->document.data.at(action.source).value("name", "")
                    : "";
            this->_renaming = action.source;
            this->_renameBuffer.fill('\0');
            name.copy(this->_renameBuffer.data(),
                      this->_renameBuffer.size() - 1);
            ImGui::OpenPopup(kRenamePopup.c_str());
            break;
        }
    }
    // Pointers kept in the selection may now name another entity
    if (changed) {
        this->_selection.reset();
    }
}

void Editor::drawRenamePopup() {
    if (!ImGui::BeginPopupModal(kRenamePopup.c_str(), nullptr,
                                ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }
    if (ImGui::IsWindowAppearing()) {
        ImGui::SetKeyboardFocusHere();
    }
    const bool entered = ImGui::InputText("##name", this->_renameBuffer.data(),
                                          this->_renameBuffer.size(),
                                          ImGuiInputTextFlags_EnterReturnsTrue);
    if (entered || ImGui::Button("OK")) {
        if (this->_project) {
            this->_project->renameEntity(this->_renaming,
                                         this->_renameBuffer.data());
        }
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void Editor::drawPrefabChildren(const nlohmann::ordered_json& entity,
                                std::vector<std::string>& chain) {
    const auto prefab = entity.find("prefab");
    if (prefab == entity.end() || !prefab->is_string()) {
        return;
    }
    const std::string path = std::filesystem::path(prefab->get<std::string>())
                                 .lexically_normal()
                                 .generic_string();
    if (std::ranges::find(chain, path) != chain.end()) {
        return;
    }
    if (!this->_project) {
        return;
    }
    const nlohmann::ordered_json& data = this->_project->getPrefab(path);
    chain.push_back(path);
    ImGui::PushID(path.c_str());
    this->drawPrefabChildren(data, chain);
    const auto children = data.find("children");
    if (children != data.end() && children->is_array()) {
        for (std::size_t index = 0; index < children->size(); ++index) {
            this->drawPrefabEntity((*children)[index], index, chain);
        }
    }
    ImGui::PopID();
    chain.pop_back();
}

void Editor::drawPrefabEntity(const nlohmann::ordered_json& entity,
                              std::size_t index,
                              std::vector<std::string>& chain) {
    if (!entity.is_object()) {
        return;
    }
    const ImGuiTreeNodeFlags flags =
        treeFlags(!this->hasChildren(entity, chain), false);
    ImGui::PushID(static_cast<int>(index));
    ImGui::PushStyleColor(ImGuiCol_Text,
                          ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
    const bool open =
        ImGui::TreeNodeEx(this->entityLabel(entity, index).c_str(), flags);
    ImGui::PopStyleColor();
    if (open) {
        this->drawPrefabChildren(entity, chain);
        const auto children = entity.find("children");
        if (children != entity.end() && children->is_array()) {
            for (std::size_t child = 0; child < children->size(); ++child) {
                this->drawPrefabEntity((*children)[child], child, chain);
            }
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

bool Editor::hasChildren(const nlohmann::ordered_json& entity,
                         std::vector<std::string> chain) {
    const auto children = entity.find("children");
    if (children != entity.end() && children->is_array() &&
        !children->empty()) {
        return true;
    }
    const auto prefab = entity.find("prefab");
    if (prefab == entity.end() || !prefab->is_string()) {
        return false;
    }
    const std::string path = std::filesystem::path(prefab->get<std::string>())
                                 .lexically_normal()
                                 .generic_string();
    if (std::ranges::find(chain, path) != chain.end()) {
        return false;
    }
    if (!this->_project) {
        return false;
    }
    chain.push_back(path);
    return this->hasChildren(this->_project->getPrefab(path), chain);
}

std::string Editor::entityLabel(const nlohmann::ordered_json& entity,
                                std::size_t index) {
    const auto name = entity.find("name");
    if (name != entity.end() && name->is_string() &&
        !name->get<std::string>().empty()) {
        return name->get<std::string>();
    }
    const auto prefab = entity.find("prefab");
    if (prefab != entity.end() && prefab->is_string() && this->_project) {
        const auto& data =
            this->_project->getPrefab(prefab->get<std::string>());
        const auto prefabName = data.find("name");
        if (prefabName != data.end() && prefabName->is_string() &&
            !prefabName->get<std::string>().empty()) {
            return prefabName->get<std::string>();
        }
    }
    return "Entity " + std::to_string(index);
}

void Editor::drawViewport() {
    ImGui::Begin("Viewport");
    ImGui::TextUnformatted("Viewport not available yet !");
    ImGui::End();
}

void Editor::drawInspector() {
    ImGui::Begin("Inspector");
    if (this->_project && this->_selection) {
        drawSelectionInspector(*this->_project, *this->_selection);
    } else {
        ImGui::TextDisabled("Nothing selected.");
    }
    ImGui::End();
}

void Editor::drawSelectionInspector(Project& project,
                                    const Selection& selection) {
    const TypedMapEdit resources{
        .types = TypeTable::get().getResources(),
        .addLabel = "Add resource",
        .showConfigOnly = selection.pointer.empty(),
    };
    if (selection.pointer.empty()) {
        ImGui::SeparatorText("World");
        Document& config = project.getConfig();
        if (editTypedMap(config.data, "resources", resources)) {
            config.dirty = true;
        }
        return;
    }
    std::optional<Scene>& scene = project.getScene();
    if (!scene || scene->name != selection.scene) {
        ImGui::TextDisabled("Nothing selected.");
        return;
    }
    if (selection.pointer == Pointer("/resources")) {
        ImGui::SeparatorText("Scene resources");
        if (editTypedMap(scene->document.data, "resources", resources)) {
            scene->document.dirty = true;
        }
        return;
    }
    drawEntityInspector(project, scene->document, selection.pointer);
}

void Editor::drawEntityInspector(Project& project, Document& document,
                                 const Pointer& pointer) {
    if (!Project::isEntity(document.data, pointer)) {
        ImGui::TextDisabled("Nothing selected.");
        return;
    }
    nlohmann::ordered_json& entity = document.data[pointer];
    bool changed = false;
    const auto name = entity.find("name");
    std::string text = name != entity.end() && name->is_string()
                           ? name->get<std::string>()
                           : std::string();
    if (inputText("Name", text)) {
        entity["name"] = text;
        changed = true;
    }
    if (pointer.parent_pointer() == Pointer("/entities")) {
        const auto flag = entity.find("persistent");
        bool persistent =
            flag != entity.end() && flag->is_boolean() && flag->get<bool>();
        if (ImGui::Checkbox("Persistent", &persistent)) {
            if (persistent) {
                entity["persistent"] = true;
            } else {
                entity.erase("persistent");
            }
            changed = true;
        }
    }
    ImGui::SeparatorText("Components");
    if (entity.contains("prefab")) {
        changed = editPrefabComponents(project, entity) || changed;
    } else {
        const TypedMapEdit components{
            .types = TypeTable::get().getComponents(),
            .addLabel = "Add component",
        };
        changed = editTypedMap(entity, "components", components) || changed;
    }
    if (changed) {
        document.dirty = true;
    }
}

bool Editor::editPrefabComponents(Project& project,
                                  nlohmann::ordered_json& entity) {
    TypedMapEdit edit{
        .types = TypeTable::get().getComponents(),
        .addLabel = "Add component",
    };
    nlohmann::ordered_json components;
    try {
        const nlohmann::ordered_json prefab{{"prefab", entity.at("prefab")}};
        edit.prefab = project.resolved(prefab).value(
            "components", nlohmann::ordered_json::object());
        components = project.resolved(entity).value(
            "components", nlohmann::ordered_json::object());
    } catch (const std::exception& error) {
        ImGui::PushStyleColor(ImGuiCol_Text, kErrorColor);
        ImGui::TextWrapped("%s", error.what());
        ImGui::PopStyleColor();
        return false;
    }
    edit.overrides =
        entity.value("components", nlohmann::ordered_json::object());
    ImGui::TextDisabled("Prefab: %s",
                        entity.at("prefab").get<std::string>().c_str());
    if (!editTypedValues(components, edit)) {
        return false;
    }
    nlohmann::ordered_json overrides =
        Project::mergeDiff(edit.prefab, components);
    if (overrides.empty()) {
        entity.erase("components");
    } else {
        entity["components"] = std::move(overrides);
    }
    return true;
}

bool Editor::editTypedMap(nlohmann::ordered_json& owner, const std::string& key,
                          const TypedMapEdit& edit) {
    const auto found = owner.find(key);
    nlohmann::ordered_json map = found != owner.end() && found->is_object()
                                     ? *found
                                     : nlohmann::ordered_json::object();
    if (!editTypedValues(map, edit)) {
        return false;
    }
    owner[key] = std::move(map);
    return true;
}

bool Editor::editTypedValues(nlohmann::ordered_json& map,
                             const TypedMapEdit& edit) {
    bool changed = false;
    std::optional<std::string> removed;
    for (const auto& item : map.items()) {
        ImGui::PushID(item.key().c_str());
        bool kept = true;
        if (ImGui::CollapsingHeader(item.key().c_str(), &kept,
                                    ImGuiTreeNodeFlags_DefaultOpen)) {
            nlohmann::ordered_json full =
                withDefaults(item.value(), item.key(), edit.types);
            if (editTypedValue(item.key(), full, edit)) {
                item.value() = std::move(full);
                changed = true;
            }
        }
        if (!kept) {
            removed = item.key();
        }
        ImGui::PopID();
    }
    if (removed) {
        map.erase(*removed);
        changed = true;
    }
    if (const auto added = drawAddTypePopup(map, edit)) {
        map[added->name] = added->defaults;
        changed = true;
    }
    return changed;
}

bool Editor::editTypedValue(const std::string& name,
                            nlohmann::ordered_json& value,
                            const TypedMapEdit& edit) {
    const auto own = edit.overrides.is_object() ? edit.overrides.find(name)
                                                : edit.overrides.end();
    if (!value.is_object() || own == edit.overrides.end() ||
        !own->is_object()) {
        return editFields(value);
    }
    const nlohmann::ordered_json base =
        edit.prefab.value(name, nlohmann::ordered_json::object());
    bool changed = false;
    std::optional<std::string> reverted;
    for (const auto& item : value.items()) {
        ImGui::PushID(item.key().c_str());
        const bool overridden = own->contains(item.key());
        if (overridden) {
            if (ImGui::SmallButton("R")) {
                reverted = item.key();
            }
            ImGui::SetItemTooltip("Revert to the prefab value");
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Text, kOverrideColor);
        }
        changed = editValue(item.key(), item.value()) || changed;
        if (overridden) {
            ImGui::PopStyleColor();
        }
        ImGui::PopID();
    }
    if (reverted) {
        if (base.contains(*reverted)) {
            value[*reverted] = base.at(*reverted);
        } else {
            value.erase(*reverted);
        }
        changed = true;
    }
    return changed;
}

nlohmann::ordered_json Editor::withDefaults(
    const nlohmann::ordered_json& value, const std::string& name,
    const std::vector<TypeInfo>& types) {
    const auto type = std::ranges::find(types, name, &TypeInfo::name);
    if (type == types.end() || !value.is_object() ||
        !type->defaults.is_object()) {
        return value;
    }
    nlohmann::ordered_json full = value;
    for (const auto& field : type->defaults.items()) {
        if (!full.contains(field.key())) {
            full[field.key()] = field.value();
        }
    }
    return full;
}

std::optional<TypeInfo> Editor::drawAddTypePopup(
    const nlohmann::ordered_json& map, const TypedMapEdit& edit) {
    ImGui::Separator();
    if (ImGui::Button(edit.addLabel.c_str(), {-FLT_MIN, 0.0F})) {
        ImGui::OpenPopup(edit.addLabel.c_str());
    }
    std::optional<TypeInfo> picked;
    if (!ImGui::BeginPopup(edit.addLabel.c_str())) {
        return picked;
    }
    for (const TypeInfo& type : edit.types) {
        if ((type.configOnly && !edit.showConfigOnly) ||
            map.contains(type.name)) {
            continue;
        }
        if (ImGui::Selectable(type.name.c_str())) {
            picked = type;
        }
    }
    ImGui::EndPopup();
    return picked;
}

}  // namespace ampersand::editor
