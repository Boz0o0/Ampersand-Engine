#pragma once

#include <algorithm>
#include <functional>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

/**
 * @file Types.hpp
 * @brief Default JSON of the component and resource types the editor can add.
 */

namespace ampersand::editor {

/**
 * @brief Component or resource type the engine can load from a JSON file.
 */
struct TypeInfo {
    std::string name;                 // Key of the type in the JSON files
    nlohmann::ordered_json defaults;  // Every field the loader reads
    bool configOnly{false};  // Resource only allowed in world/config.json
};

/**
 * @brief What the editor knows of a field of the engine types, by its key.
 */
struct FieldInfo {
    nlohmann::ordered_json entryDefault;  // Object of free keys (a map): the
                                          // value of a new entry; null if not
    nlohmann::ordered_json itemDefault;   // Array: value of a new item, or null
    std::vector<std::string> choices;     // Allowed names of a string field (or
                                          // null); empty if any string
};

/**
 * @brief Types of the enabled engine modules, built once.
 */
class TypeTable {
public:
    /** @brief The table, built on first use. */
    static const TypeTable& get();

    /** @brief Component types, in the order of the engine modules. */
    [[nodiscard]] const std::vector<TypeInfo>& getComponents() const {
        return _components;
    }

    /** @brief Resource types, in the order of the engine modules. */
    [[nodiscard]] const std::vector<TypeInfo>& getResources() const {
        return _resources;
    }

    /** @brief What is known of the fields named key, or nothing if unknown. */
    [[nodiscard]] const FieldInfo& getField(std::string_view key) const;

    /** @brief Default JSON of the component type name, or {} if unknown. */
    [[nodiscard]] nlohmann::ordered_json getComponentDefaults(
        std::string_view name) const;

private:
    TypeTable();

    /*
     * One per engine module, each adding the types it registers in
     * SceneManager, or nothing if the module is disabled.
     */
    void addCore();
    void addRender();
    void addPhysics();
    void addAudio();
    void addInput();
    void addScripting();

    /**
     * @brief JSON of value, from the engine serializers.
     *
     * Goes through nlohmann::json when T only has a to_json for it.
     */
    template <typename T>
    static nlohmann::ordered_json toJson(const T& value) {
        nlohmann::ordered_json data;
        if constexpr (std::is_constructible_v<nlohmann::ordered_json,
                                              const T&>) {
            data = value;
        } else {
            data = nlohmann::ordered_json::parse(nlohmann::json(value).dump());
        }
        roundFloats(data);
        return data;
    }

    /** @brief Largest value of the uint8_t engine enums, to list them. */
    static constexpr int kEnumValues{256};

    /**
     * @brief Appends the names of Enum written by the engine serializers,
     * without duplicates: unknown values are written as the first name.
     */
    template <typename Enum>
    static void addEnumNames(std::vector<std::string>& names) {
        for (int value = 0; value < kEnumValues; ++value) {
            const nlohmann::json name = static_cast<Enum>(value);
            if (name.is_string() &&
                std::ranges::find(names, name.get<std::string>()) ==
                    names.end()) {
                names.push_back(name.get<std::string>());
            }
        }
    }

    /**
     * @brief Rewrites every float of data with its shortest decimal form as a
     * float, so 0.1F is written 0.1 and not 0.10000000149011612.
     * The engine stores its numbers as float, so no precision is lost.
     */
    static void roundFloats(nlohmann::ordered_json& data);

    std::vector<TypeInfo> _components;
    std::vector<TypeInfo> _resources;
    std::map<std::string, FieldInfo, std::less<>> _fields;
};

}  // namespace ampersand::editor
