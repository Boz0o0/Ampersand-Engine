#include <algorithm>
#include <ampersand/core/ecs/CommandQueue.hpp>
#include <ampersand/core/ecs/Scheduler.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <ampersand/core/resources/Stats.hpp>
#include <chrono>
#include <cmath>
#include <stdexcept>

namespace ampersand::core::ecs {

namespace {

constexpr float kMsPerSecond = 1000.0F;

using Milliseconds = std::chrono::duration<float, std::milli>;

}  // namespace

Scheduler::Scheduler(World& world) : _world(world) {
    if (!_world.hasResource<Clock>()) {
        _world.addResource<Clock>();
    }
    if (!_world.hasResource<CommandQueue>()) {
        _world.addResource<CommandQueue>();
    }
    _endOfFrame.emplace_back(
        [](World& world) { world.getResource<CommandQueue>().apply(world); });
}

void Scheduler::update(float deltaTime) {
    if (!std::isfinite(deltaTime) || deltaTime < 0.0F) {
        throw std::invalid_argument(
            "Scheduler::update: deltaTime must be finite and not negative");
    }
    const TimePoint start = std::chrono::steady_clock::now();
    auto& clock = _world.getResource<Clock>();
    clock.dt = deltaTime;
    clock.elapsed += deltaTime;
    for (const auto& system : _systems) {
        system->update(_world);
    }
    for (const auto& work : _endOfFrame) {
        work(_world);
    }
    if (_world.hasResource<Stats>()) {
        recordStats(start);
    }
    _previousStart = start;
}

void Scheduler::recordStats(TimePoint start) {
    auto& stats = _world.getResource<Stats>();
    stats.updateMs =
        Milliseconds(std::chrono::steady_clock::now() - start).count();
    stats.entities = _world.entityCount();
    if (!_previousStart) {
        stats.frameMs = 0.0F;
        return;
    }
    stats.frameMs = Milliseconds(start - *_previousStart).count();
    std::ranges::rotate(stats.history, stats.history.begin() + 1);
    stats.history.back() = stats.frameMs;
    _fpsMs += stats.frameMs;
    ++_fpsFrames;
    if (_fpsMs >= kMsPerSecond) {
        stats.fps = static_cast<float>(_fpsFrames) * kMsPerSecond / _fpsMs;
        _fpsMs = 0.0F;
        _fpsFrames = 0;
    }
}

}  // namespace ampersand::core::ecs
