#pragma once

#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/net/UdpSocket.hpp>
#include <chrono>
#include <optional>

namespace ampersand::net {

/**
 * @brief Writes the traffic of a UdpSocket in the values of the Stats
 * resource, if the World has one (nothing is written otherwise).
 *
 * The rates are measured with the real clock, over about one interval (one
 * second by default), like the fps of the Stats: they stay at 0 during the
 * first interval. The keys written are:
 * - "net.kbSentPerSec" and "net.kbReceivedPerSec" (1 kB = 1000 bytes)
 * - "net.packetsSentPerSec" and "net.packetsReceivedPerSec"
 * - "net.packetsDropped", the total of the packets dropped by send
 *
 * The socket must outlive the system, and must not be moved while the system
 * uses it.
 */
class NetStatsSystem final : public core::ecs::ISystem {
public:
    /**
     * @brief Creates the system for a socket.
     *
     * @param socket Socket whose traffic is written in the Stats.
     * @param interval Real time over which the rates are measured.
     * @throws std::invalid_argument if interval is not positive.
     */
    explicit NetStatsSystem(
        const UdpSocket& socket,
        std::chrono::milliseconds interval = std::chrono::seconds(1));

    /** @brief A temporary socket would be destroyed before the system. */
    explicit NetStatsSystem(const UdpSocket&& socket,
                            std::chrono::milliseconds interval = {}) = delete;

    /**
     * @brief Writes the dropped packets, and the rates once an interval has
     * passed since the previous ones.
     *
     * @param world World holding the Stats resource, if any.
     */
    void update(core::ecs::World& world) override;

private:
    using TimePoint = std::chrono::steady_clock::time_point;

    const UdpSocket& _socket;
    std::chrono::milliseconds _interval;
    /** @brief Counters of the socket at the start of the interval. */
    NetStats _previous;
    /** @brief Start of the interval, none before the first update with a
     * Stats. */
    std::optional<TimePoint> _intervalStart;
};

}  // namespace ampersand::net
