#include <gtest/gtest.h>

#include <ampersand/core/ecs/Scheduler.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/resources/Stats.hpp>
#include <ampersand/net/UdpSocket.hpp>
#include <ampersand/net/systems/NetStatsSystem.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <thread>

using ampersand::core::Stats;
using ampersand::core::ecs::Scheduler;
using ampersand::core::ecs::World;
using ampersand::net::Endpoint;
using ampersand::net::NetStatsSystem;
using ampersand::net::UdpSocket;
using namespace std::chrono_literals;

namespace {

// receive() never blocks, so poll until the datagram crosses the loopback.
bool receiveWithin(UdpSocket& socket) {
    std::array<std::byte, 64> buffer{};
    Endpoint from;
    const auto deadline = std::chrono::steady_clock::now() + 1s;
    while (std::chrono::steady_clock::now() < deadline) {
        if (socket.receive(buffer, from)) {
            return true;
        }
        std::this_thread::sleep_for(1ms);
    }
    return false;
}

/** @brief Sends "hello" (5 bytes) from the socket to itself and reads it. */
void sendToItself(UdpSocket& socket) {
    socket.send("hello", {.address = "127.0.0.1", .port = socket.localPort()});
    ASSERT_TRUE(receiveWithin(socket));
}

}  // namespace

TEST(NetStatsSystem, WritesNothingWithoutStats) {
    World world;
    Scheduler scheduler(world);
    const UdpSocket socket;
    scheduler.add<NetStatsSystem>(socket);

    EXPECT_NO_THROW(scheduler.update(0.0F));

    EXPECT_FALSE(world.hasResource<Stats>());
}

TEST(NetStatsSystem, RatesAreZeroDuringTheFirstInterval) {
    World world;
    Scheduler scheduler(world);
    auto& stats = world.addResource<Stats>();
    UdpSocket socket;
    scheduler.add<NetStatsSystem>(socket, 1h);

    scheduler.update(0.0F);
    sendToItself(socket);
    scheduler.update(0.0F);

    EXPECT_EQ(stats.values.at("net.kbSentPerSec"), 0.0F);
    EXPECT_EQ(stats.values.at("net.kbReceivedPerSec"), 0.0F);
    EXPECT_EQ(stats.values.at("net.packetsSentPerSec"), 0.0F);
    EXPECT_EQ(stats.values.at("net.packetsReceivedPerSec"), 0.0F);
    EXPECT_EQ(stats.values.at("net.packetsDropped"), 0.0F);
}

TEST(NetStatsSystem, MeasuresTheRatesOverTheInterval) {
    World world;
    Scheduler scheduler(world);
    auto& stats = world.addResource<Stats>();
    UdpSocket socket;
    scheduler.add<NetStatsSystem>(socket, 20ms);

    scheduler.update(0.0F);
    sendToItself(socket);
    sendToItself(socket);
    std::this_thread::sleep_for(30ms);
    scheduler.update(0.0F);

    // 2 packets of 5 bytes over at least 20 ms: at most 100 packets/s and
    // 0.5 kB/s.
    EXPECT_GT(stats.values.at("net.packetsSentPerSec"), 0.0F);
    EXPECT_LE(stats.values.at("net.packetsSentPerSec"), 100.0F);
    EXPECT_GT(stats.values.at("net.packetsReceivedPerSec"), 0.0F);
    EXPECT_LE(stats.values.at("net.packetsReceivedPerSec"), 100.0F);
    EXPECT_GT(stats.values.at("net.kbSentPerSec"), 0.0F);
    EXPECT_LE(stats.values.at("net.kbSentPerSec"), 0.5F);
    EXPECT_GT(stats.values.at("net.kbReceivedPerSec"), 0.0F);
    EXPECT_LE(stats.values.at("net.kbReceivedPerSec"), 0.5F);
}

TEST(NetStatsSystem, RatesOnlyCountTheLastInterval) {
    World world;
    Scheduler scheduler(world);
    auto& stats = world.addResource<Stats>();
    UdpSocket socket;
    scheduler.add<NetStatsSystem>(socket, 20ms);

    scheduler.update(0.0F);
    sendToItself(socket);
    std::this_thread::sleep_for(30ms);
    scheduler.update(0.0F);
    ASSERT_GT(stats.values.at("net.packetsSentPerSec"), 0.0F);

    std::this_thread::sleep_for(30ms);
    scheduler.update(0.0F);

    EXPECT_EQ(stats.values.at("net.packetsSentPerSec"), 0.0F);
    EXPECT_EQ(stats.values.at("net.kbSentPerSec"), 0.0F);
}

TEST(NetStatsSystem, KeepsTheOtherValues) {
    World world;
    Scheduler scheduler(world);
    auto& stats = world.addResource<Stats>();
    stats.values["game.ping"] = 42.0F;
    const UdpSocket socket;
    scheduler.add<NetStatsSystem>(socket);

    scheduler.update(0.0F);

    EXPECT_EQ(stats.values.at("game.ping"), 42.0F);
}

TEST(NetStatsSystem, ThrowsOnANonPositiveInterval) {
    const UdpSocket socket;
    EXPECT_THROW(NetStatsSystem(socket, 0ms), std::invalid_argument);
    EXPECT_THROW(NetStatsSystem(socket, -1ms), std::invalid_argument);
}
