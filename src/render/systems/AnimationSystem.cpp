#include <algorithm>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <ampersand/render/components/Animation.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <ampersand/render/systems/AnimationSystem.hpp>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace ampersand::render {

void AnimationSystem::update(core::ecs::World& world) {
    const float deltaTime = world.getResource<core::Clock>().dt;
    for (const core::ecs::Entity entity : world.owners<Animation>()) {
        auto& animation = world.get<Animation>(entity);
        if (!world.has<Sprite>(entity) || animation.count <= 0) {
            continue;
        }
        if (!std::isfinite(animation.frameDuration) ||
            animation.frameDuration <= 0.0F) {
            throw std::invalid_argument(
                "Animation frameDuration must be finite and positive");
        }
        if (animation.columns <= 0) {
            throw std::invalid_argument("Animation columns must be positive");
        }
        const float totalDuration =
            animation.frameDuration * static_cast<float>(animation.count);
        animation.elapsed += deltaTime;
        if (animation.loop) {
            animation.elapsed = std::fmod(animation.elapsed, totalDuration);
        } else {
            animation.elapsed = std::min(animation.elapsed, totalDuration);
        }
        const std::int32_t frame =
            animation.first +
            std::min(static_cast<std::int32_t>(animation.elapsed /
                                               animation.frameDuration),
                     animation.count - 1);
        world.get<Sprite>(entity).source = {
            (frame % animation.columns) * animation.frameWidth,
            (frame / animation.columns) * animation.frameHeight,
            animation.frameWidth, animation.frameHeight};
    }
}

}  // namespace ampersand::render
