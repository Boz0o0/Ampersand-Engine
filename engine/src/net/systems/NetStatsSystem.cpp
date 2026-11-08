#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/resources/Stats.hpp>
#include <ampersand/net/systems/NetStatsSystem.hpp>
#include <chrono>
#include <cstdint>
#include <stdexcept>

namespace ampersand::net {

namespace {

constexpr float kBytesPerKb = 1000.0F;

using Seconds = std::chrono::duration<float>;

/** @brief Count per second of what changed between two values. */
float perSecond(std::uint64_t previous, std::uint64_t current, float seconds) {
    return static_cast<float>(current - previous) / seconds;
}

}  // namespace

NetStatsSystem::NetStatsSystem(const UdpSocket& socket,
                               std::chrono::milliseconds interval)
    : _socket(socket), _interval(interval) {
    if (interval <= std::chrono::milliseconds::zero()) {
        throw std::invalid_argument(
            "NetStatsSystem: the interval must be positive");
    }
}

void NetStatsSystem::update(core::ecs::World& world) {
    if (!world.hasResource<core::Stats>()) {
        return;
    }
    auto& values = world.getResource<core::Stats>().values;
    const NetStats& current = _socket.stats();
    const TimePoint now = std::chrono::steady_clock::now();
    values["net.packetsDropped"] = static_cast<float>(current.packetsDropped);

    if (!_intervalStart) {
        values["net.kbSentPerSec"] = 0.0F;
        values["net.kbReceivedPerSec"] = 0.0F;
        values["net.packetsSentPerSec"] = 0.0F;
        values["net.packetsReceivedPerSec"] = 0.0F;
    } else if (now - *_intervalStart >= _interval) {
        const float seconds = Seconds(now - *_intervalStart).count();
        values["net.kbSentPerSec"] =
            perSecond(_previous.bytesSent, current.bytesSent, seconds) /
            kBytesPerKb;
        values["net.kbReceivedPerSec"] =
            perSecond(_previous.bytesReceived, current.bytesReceived, seconds) /
            kBytesPerKb;
        values["net.packetsSentPerSec"] =
            perSecond(_previous.packetsSent, current.packetsSent, seconds);
        values["net.packetsReceivedPerSec"] = perSecond(
            _previous.packetsReceived, current.packetsReceived, seconds);
    } else {
        return;
    }
    _previous = current;
    _intervalStart = now;
}

}  // namespace ampersand::net
