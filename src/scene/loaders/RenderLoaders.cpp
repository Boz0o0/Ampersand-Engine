#ifdef AMPERSAND_MODULE_RENDER

#include <algorithm>
#include <ampersand/render/Primitive.hpp>
#include <ampersand/render/SfmlRenderer.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <ampersand/render/components/Text.hpp>
#include <ampersand/render/resources/Renderer.hpp>
#include <ampersand/scene/resources/Backends.hpp>
#include <ampersand/scene/resources/SceneManager.hpp>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>

namespace ampersand::scene {

void SceneManager::loadRenderer(core::ecs::World& world, const json& data) {
    std::shared_ptr<render::IRenderer> backend;
    const auto name = data.at("backend").get<std::string>();
    if (name == "sfml") {
        backend = std::make_shared<render::SfmlRenderer>();
    } else {
        throw std::runtime_error("Unknown renderer backend '" + name + "'");
    }
    world.addResource<render::Renderer>(*backend,
                                        data.get<render::WindowConfig>());
    if (!world.hasResource<Backends>()) {
        world.addResource<Backends>();
    }
    world.getResource<Backends>().owned["Renderer"] = backend;
}

void SceneManager::removeRenderer(core::ecs::World& world) {
    if (world.hasResource<render::Renderer>()) {
        world.removeResource<render::Renderer>();
    }
    if (world.hasResource<Backends>()) {
        world.getResource<Backends>().owned.erase("Renderer");
    }
    forgetAssets("texture");
    forgetAssets("font");
}

void SceneManager::loadSprite(core::ecs::World& world, core::ecs::Entity entity,
                              const json& data) {
    auto& renderer = world.getResource<render::Renderer>().renderer;
    render::Sprite sprite;
    sprite.texture = asset("texture", data.at("texture").get<std::string>(),
                           [&renderer](const std::string& path) {
                               return renderer.loadTexture(path);
                           });
    data.at("source").get_to(sprite.source);
    data.at("color").get_to(sprite.color);
    data.at("layer").get_to(sprite.layer);
    world.add(entity, sprite);
}

void SceneManager::loadText(core::ecs::World& world, core::ecs::Entity entity,
                            const json& data) {
    auto& renderer = world.getResource<render::Renderer>().renderer;
    render::Text text;
    text.font = asset("font", data.at("font").get<std::string>(),
                      [&renderer](const std::string& path) {
                          return renderer.loadFont(path);
                      });
    data.at("value").get_to(text.value);
    data.at("size").get_to(text.size);
    data.at("color").get_to(text.color);
    data.at("layer").get_to(text.layer);
    world.add(entity, text);
}

void SceneManager::checkNotNegative(float value, const std::string& field) {
    if (value < 0.0F) {
        throw std::runtime_error(field + " must not be negative");
    }
}

void SceneManager::loadRectangle(core::ecs::World& world,
                                 core::ecs::Entity entity, const json& data) {
    const auto rectangle = data.get<render::Rectangle>();
    checkNotNegative(rectangle.size.x, "Rectangle size");
    checkNotNegative(rectangle.size.y, "Rectangle size");
    checkNotNegative(rectangle.outlineThickness, "Rectangle outlineThickness");
    world.add(entity, rectangle);
}

void SceneManager::loadCircle(core::ecs::World& world, core::ecs::Entity entity,
                              const json& data) {
    const auto circle = data.get<render::Circle>();
    checkNotNegative(circle.radius, "Circle radius");
    checkNotNegative(circle.outlineThickness, "Circle outlineThickness");
    world.add(entity, circle);
}

bool SceneManager::conditionHolds(const json& condition,
                                  const render::AnimationParams& params) {
    const auto param = condition.at("param").get<std::string>();
    if (condition.at("value").is_boolean()) {
        if (condition.contains("op")) {
            throw std::runtime_error("A bool condition has no operator");
        }
        const auto found = params.bools.find(param);
        return found != params.bools.end() &&
               found->second == condition.at("value").get<bool>();
    }
    const auto found = params.floats.find(param);
    const float current = found != params.floats.end() ? found->second : 0.0F;
    const auto value = condition.at("value").get<float>();
    static const std::map<std::string, std::function<bool(float, float)>>
        operators = {
            {"<", std::less<>{}},      {"<=", std::less_equal<>{}},
            {">", std::greater<>{}},   {">=", std::greater_equal<>{}},
            {"==", std::equal_to<>{}}, {"!=", std::not_equal_to<>{}},
        };
    const auto name = condition.at("op").get<std::string>();
    const auto compare = operators.find(name);
    if (compare == operators.end()) {
        throw std::runtime_error("Unknown condition operator '" + name + "'");
    }
    return compare->second(current, value);
}

void SceneManager::loadAnimationStateMachine(core::ecs::World& world,
                                             core::ecs::Entity entity,
                                             const json& data) {
    render::AnimationStateMachine machine(
        data.at("states")
            .get<std::unordered_map<std::string, render::Animation>>(),
        data.at("initial").get<std::string>());
    data.at("params").get_to(machine.params);
    for (const auto& transition : data.at("transitions")) {
        auto when = transition.at("when");
        for (const auto& condition : when) {
            conditionHolds(condition, {});  // Rejects bad conditions now
        }
        machine.transitions.push_back({
            .from = transition.at("from").get<std::string>(),
            .to = transition.at("to").get<std::string>(),
            .condition =
                [when =
                     std::move(when)](const render::AnimationParams& params) {
                    return std::ranges::all_of(
                        when, [&params](const json& condition) {
                            return conditionHolds(condition, params);
                        });
                },
        });
    }
    world.add(entity, std::move(machine));
}

void SceneManager::registerRender() {
    registerResource<render::ClearColor>("ClearColor");
    registerResource<render::Camera>("Camera");
    registerResource(
        "Renderer", loadRenderer,
        [this](core::ecs::World& world) { removeRenderer(world); },
        ResourceScope::Config);
    registerComponent<render::Animation>("Animation");
    registerComponent("Rectangle", loadRectangle);
    registerComponent("Circle", loadCircle);
    registerComponent<render::Line>("Line");
    registerComponent("AnimationStateMachine", loadAnimationStateMachine);
    registerComponent(
        "Sprite",
        [this](core::ecs::World& world, core::ecs::Entity entity,
               const json& data) { loadSprite(world, entity, data); });
    registerComponent(
        "Text", [this](core::ecs::World& world, core::ecs::Entity entity,
                       const json& data) { loadText(world, entity, data); });
}

}  // namespace ampersand::scene

#endif
