#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <editor/Editor.hpp>
#include <format>
#include <optional>

namespace ampersand::editor {

bool Editor::editValue(const std::string& label, nlohmann::ordered_json& value,
                       std::string_view field) {
    if (field.empty()) {
        field = label;
    }
    const std::vector<std::string>& choices =
        TypeTable::get().getField(field).choices;
    if (!choices.empty() && (value.is_string() || value.is_null())) {
        return editChoice(label, value, choices);
    }
    if (value.is_object()) {
        return editObject(label, value, field);
    }
    if (value.is_array()) {
        return editArray(label, value, field);
    }
    if (value.is_number()) {
        return editNumber(label, value);
    }
    if (value.is_string()) {
        return editString(label, value, field);
    }
    if (value.is_boolean()) {
        bool checked = value.get<bool>();
        if (ImGui::Checkbox(label.c_str(), &checked)) {
            value = checked;
            return true;
        }
        return false;
    }
    ImGui::LabelText(label.c_str(), "null");
    return false;
}

bool Editor::editObject(const std::string& label, nlohmann::ordered_json& value,
                        std::string_view field) {
    if (!TypeTable::get().getField(field).entryDefault.is_null()) {
        return editMap(label, value, field);
    }
    if (hasShape(value, kColorKeys)) {
        return editColor(label, value);
    }
    if (hasShape(value, kVec2Keys)) {
        return editNumbers(label, value, kVec2Keys);
    }
    if (hasShape(value, kVec3Keys)) {
        return editNumbers(label, value, kVec3Keys);
    }
    if (hasShape(value, kRectKeys)) {
        return editNumbers(label, value, kRectKeys);
    }
    if (!ImGui::TreeNode(label.c_str())) {
        return false;
    }
    const bool changed = editFields(value);
    ImGui::TreePop();
    return changed;
}

bool Editor::editFields(nlohmann::ordered_json& object) {
    if (!object.is_object()) {
        return editValue("value", object);
    }
    bool changed = false;
    for (const auto& item : object.items()) {
        changed = editValue(item.key(), item.value()) || changed;
    }
    return changed;
}

bool Editor::editMap(const std::string& label, nlohmann::ordered_json& map,
                     std::string_view field) {
    if (!ImGui::TreeNode(label.c_str())) {
        return false;
    }
    bool changed = false;
    std::optional<std::string> removed;
    std::optional<std::pair<std::string, std::string>> renamed;
    int index = 0;
    for (const auto& item : map.items()) {
        // Indices, not keys, as IDs: renaming keeps the fields focused
        ImGui::PushID(index++);
        if (ImGui::SmallButton("-")) {
            removed = item.key();
        }
        ImGui::SameLine();
        std::optional<std::string> name;
        changed =
            editMapEntry(item.key(), item.value(), field, name) || changed;
        if (name && !name->empty() && !map.contains(*name)) {
            renamed = {item.key(), *name};
        }
        ImGui::PopID();
    }
    if (removed) {
        map.erase(*removed);
        changed = true;
    } else if (renamed) {
        map = renameKey(map, renamed->first, renamed->second);
        changed = true;
    }
    if (ImGui::SmallButton("+")) {
        map[newEntryKey(map)] = TypeTable::get().getField(field).entryDefault;
        changed = true;
    }
    ImGui::TreePop();
    return changed;
}

bool Editor::editMapEntry(const std::string& key, nlohmann::ordered_json& value,
                          std::string_view field,
                          std::optional<std::string>& renamed) {
    if (!ImGui::TreeNode("entry", "%s", key.c_str())) {
        return false;
    }
    std::string name = key;
    if (inputText("Name", name)) {
        renamed = name;
    }
    bool changed = false;
    if (value.is_object()) {
        changed = editFields(value);
    } else if (value.is_array()) {
        changed = editItems(value, field);
    } else {
        changed = editValue("Value", value, field);
    }
    ImGui::TreePop();
    return changed;
}

std::string Editor::newEntryKey(const nlohmann::ordered_json& map) {
    std::string key(kNewEntry);
    for (int number = 2; map.contains(key); ++number) {
        key = std::string(kNewEntry) + " " + std::to_string(number);
    }
    return key;
}

nlohmann::ordered_json Editor::renameKey(const nlohmann::ordered_json& object,
                                         const std::string& from,
                                         const std::string& newKey) {
    nlohmann::ordered_json renamed = nlohmann::ordered_json::object();
    for (const auto& item : object.items()) {
        renamed[item.key() == from ? newKey : item.key()] = item.value();
    }
    return renamed;
}

bool Editor::inputText(const std::string& label, std::string& text) {
    std::array<char, kTextSize> buffer{};
    text.copy(buffer.data(), buffer.size() - 1);
    if (!ImGui::InputText(label.c_str(), buffer.data(), buffer.size())) {
        return false;
    }
    text = buffer.data();
    return true;
}

bool Editor::isAssetKey(std::string_view key) {
    return std::ranges::find(kAssetKeys, key) != kAssetKeys.end();
}

bool Editor::editNumbers(const std::string& label,
                         nlohmann::ordered_json& value,
                         std::span<const std::string_view> keys) {
    const auto count = static_cast<int>(keys.size());
    const bool integers =
        keys.size() == kRectKeys.size() &&
        std::ranges::all_of(keys, [&value](std::string_view key) {
            return value.at(std::string(key)).is_number_integer();
        });
    if (integers) {
        std::array<std::int32_t, kRectKeys.size()> numbers{};
        for (std::size_t index = 0; index < keys.size(); ++index) {
            numbers.at(index) =
                value.at(std::string(keys[index])).get<std::int32_t>();
        }
        if (!ImGui::DragScalarN(label.c_str(), ImGuiDataType_S32,
                                numbers.data(), count)) {
            return false;
        }
        for (std::size_t index = 0; index < keys.size(); ++index) {
            value[std::string(keys[index])] = numbers.at(index);
        }
        return true;
    }
    std::array<float, kRectKeys.size()> numbers{};
    for (std::size_t index = 0; index < keys.size(); ++index) {
        numbers.at(index) = value.at(std::string(keys[index])).get<float>();
    }
    if (!ImGui::DragScalarN(label.c_str(), ImGuiDataType_Float, numbers.data(),
                            count, kDragSpeed)) {
        return false;
    }
    // Only the dragged number is written: the others keep their precision
    for (std::size_t index = 0; index < keys.size(); ++index) {
        nlohmann::ordered_json& number = value[std::string(keys[index])];
        if (numbers.at(index) != number.get<float>()) {
            setFloat(number, numbers.at(index));
        }
    }
    return true;
}

bool Editor::editColor(const std::string& label,
                       nlohmann::ordered_json& value) {
    constexpr float kMax = 255.0F;
    std::array<float, kColorKeys.size()> color{};
    for (std::size_t index = 0; index < kColorKeys.size(); ++index) {
        color.at(index) =
            value.at(std::string(kColorKeys.at(index))).get<float>() / kMax;
    }
    if (!ImGui::ColorEdit4(label.c_str(), color.data())) {
        return false;
    }
    for (std::size_t index = 0; index < kColorKeys.size(); ++index) {
        value[std::string(kColorKeys.at(index))] =
            static_cast<std::int32_t>(std::lround(color.at(index) * kMax));
    }
    return true;
}

bool Editor::editNumber(const std::string& label,
                        nlohmann::ordered_json& value) {
    if (value.is_number_integer()) {
        int number = value.get<int>();
        if (!ImGui::InputInt(label.c_str(), &number)) {
            return false;
        }
        if (value.is_number_unsigned()) {
            value = static_cast<std::uint32_t>(std::max(number, 0));
        } else {
            value = number;
        }
        return true;
    }
    auto number = value.get<float>();
    if (!ImGui::DragFloat(label.c_str(), &number, kDragSpeed)) {
        return false;
    }
    setFloat(value, number);
    return true;
}

bool Editor::editString(const std::string& label, nlohmann::ordered_json& value,
                        std::string_view field) {
    std::string text = value.get<std::string>();
    bool changed = inputText(label, text);
    if (isAssetKey(field) && ImGui::BeginDragDropTarget()) {
        const ImGuiPayload* payload =
            ImGui::AcceptDragDropPayload(kAssetPayload.c_str());
        if (payload != nullptr) {
            text = payloadText(*payload);
            changed = true;
        }
        ImGui::EndDragDropTarget();
    }
    if (changed) {
        value = text;
    }
    return changed;
}

bool Editor::editChoice(const std::string& label, nlohmann::ordered_json& value,
                        const std::vector<std::string>& choices) {
    const std::string current =
        value.is_string() ? value.get<std::string>() : std::string(kNoChoice);
    if (!ImGui::BeginCombo(label.c_str(), current.c_str())) {
        return false;
    }
    bool changed = false;
    if (ImGui::Selectable(std::string(kNoChoice).c_str(), value.is_null())) {
        value = nullptr;
        changed = true;
    }
    for (const std::string& choice : choices) {
        if (ImGui::Selectable(choice.c_str(), choice == current)) {
            value = choice;
            changed = true;
        }
    }
    ImGui::EndCombo();
    return changed;
}

bool Editor::editArray(const std::string& label, nlohmann::ordered_json& value,
                       std::string_view field) {
    if (!ImGui::TreeNode(label.c_str())) {
        return false;
    }
    const bool changed = editItems(value, field);
    ImGui::TreePop();
    return changed;
}

bool Editor::editItems(nlohmann::ordered_json& array, std::string_view field) {
    bool changed = false;
    std::optional<std::size_t> removed;
    for (std::size_t index = 0; index < array.size(); ++index) {
        ImGui::PushID(static_cast<int>(index));
        if (ImGui::SmallButton("-")) {
            removed = index;
        }
        ImGui::SameLine();
        changed =
            editValue(std::to_string(index), array[index], field) || changed;
        ImGui::PopID();
    }
    if (removed) {
        array.erase(*removed);
        changed = true;
    }
    if (ImGui::SmallButton("+")) {
        // A copy of the last item keeps the type of the items
        nlohmann::ordered_json item =
            array.empty() ? TypeTable::get().getField(field).itemDefault
                          : array.back();
        array.push_back(std::move(item));
        changed = true;
    }
    return changed;
}

bool Editor::hasShape(const nlohmann::ordered_json& value,
                      std::span<const std::string_view> keys) {
    return value.is_object() && value.size() == keys.size() &&
           std::ranges::all_of(keys, [&value](std::string_view key) {
               const auto found = value.find(std::string(key));
               return found != value.end() && found->is_number();
           });
}

void Editor::setFloat(nlohmann::ordered_json& number, float value) {
    if (number.is_number_integer() && value == std::trunc(value)) {
        number = static_cast<std::int64_t>(value);
    } else {
        number = std::stod(std::format("{}", value));
    }
}

}  // namespace ampersand::editor
