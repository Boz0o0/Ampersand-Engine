#include <algorithm>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <ampersand/render/components/Animation.hpp>
#include <ampersand/render/components/AnimationStateMachine.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <ampersand/render/systems/AnimationSystem.hpp>
#include <cmath>
#include <cstddef>
#include <stdexcept>

namespace ampersand::render {

void AnimationSystem::update(core::ecs::World& world) {
    const float deltaTime = world.getResource<core::Clock>().dt;
    for (const core::ecs::Entity entity :
         world.owners<AnimationStateMachine>()) {
        if (!world.has<Animation>(entity)) {
            world.add(entity, Animation{});
            world.get<AnimationStateMachine>(entity).reload();
        }
        updateStateMachine(world.get<AnimationStateMachine>(entity),
                           world.get<Animation>(entity));
    }
    for (const core::ecs::Entity entity : world.owners<Animation>()) {
        auto& animation = world.get<Animation>(entity);
        if (!world.has<Sprite>(entity) || animation.frames.empty()) {
            continue;
        }
        if (!std::isfinite(animation.frameDuration) ||
            animation.frameDuration <= 0.0F) {
            throw std::invalid_argument(
                "Animation frameDuration must be finite and positive");
        }
        const float totalDuration = animation.frameDuration *
                                    static_cast<float>(animation.frames.size());
        animation.elapsed += deltaTime;
        if (animation.loop) {
            animation.elapsed = std::fmod(animation.elapsed, totalDuration);
        } else {
            animation.elapsed = std::min(animation.elapsed, totalDuration);
        }
        const auto frame =
            std::min(static_cast<std::size_t>(animation.elapsed /
                                              animation.frameDuration),
                     animation.frames.size() - 1);
        world.get<Sprite>(entity).source = animation.frames[frame];
    }
}

void AnimationSystem::updateStateMachine(AnimationStateMachine& machine,
                                         Animation& animation) {
    if (machine._action && machine._playing == *machine._action &&
        isFinished(animation)) {
        machine._action.reset();
        machine._playing.clear();
    }
    if (!machine._action) {
        const auto transition = std::ranges::find_if(
            machine.transitions,
            [&machine](const AnimationTransition& candidate) {
                return (candidate.from == machine._current ||
                        candidate.from == "*") &&
                       candidate.condition &&
                       candidate.condition(machine.params);
            });
        if (transition != machine.transitions.end()) {
            machine.setCurrent(transition->to);
        }
    }
    const std::string& wanted = machine._action.value_or(machine._current);
    if (wanted == machine._playing) {
        return;
    }
    const auto clip = machine.states.find(wanted);
    if (clip == machine.states.end()) {
        throw std::invalid_argument("AnimationStateMachine has no state '" +
                                    wanted + "'");
    }
    animation = clip->second;
    animation.elapsed = 0.0F;
    animation.loop = animation.loop && !machine._action;
    machine._playing = wanted;
}

}  // namespace ampersand::render
