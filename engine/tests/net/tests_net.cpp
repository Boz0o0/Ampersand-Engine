#include <gtest/gtest.h>

#include <ampersand/net/UdpSocket.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <thread>
#include <utility>

using ampersand::net::Endpoint;
using ampersand::net::NetError;
using ampersand::net::NetStats;
using ampersand::net::UdpSocket;

namespace {

// receive() never blocks, so poll until the datagram crosses the loopback.
std::optional<std::size_t> receiveWithin(UdpSocket& socket,
                                         std::span<std::byte> buffer,
                                         Endpoint& from) {
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (std::chrono::steady_clock::now() < deadline) {
        if (const auto length = socket.receive(buffer, from)) {
            return length;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return std::nullopt;
}

std::string receiveString(UdpSocket& socket, Endpoint& from) {
    std::array<std::byte, 1024> buffer{};
    const auto length = receiveWithin(socket, buffer, from);
    if (!length) {
        return {};
    }
    return {reinterpret_cast<const char*>(buffer.data()), *length};
}

}  // namespace

TEST(UdpSocket, DefaultConstructorBindsAnEphemeralPort) {
    const UdpSocket socket;
    EXPECT_NE(socket.localPort(), 0);
}

TEST(UdpSocket, ResolvesLocalhost) {
    const Endpoint endpoint = UdpSocket::resolve("localhost", 4242);
    EXPECT_EQ(endpoint.address, "127.0.0.1");
    EXPECT_EQ(endpoint.port, 4242);
    EXPECT_EQ(endpoint.toString(), "127.0.0.1:4242");
}

TEST(UdpSocket, HelloRoundTrip) {
    UdpSocket server;
    UdpSocket client;
    const Endpoint serverEndpoint =
        UdpSocket::resolve("127.0.0.1", server.localPort());

    client.send("hello", serverEndpoint);
    Endpoint sender;
    EXPECT_EQ(receiveString(server, sender), "hello");
    EXPECT_EQ(sender.port, client.localPort());

    server.send("ok", sender);
    Endpoint reply;
    EXPECT_EQ(receiveString(client, reply), "ok");
    EXPECT_EQ(reply, serverEndpoint);
}

TEST(UdpSocket, SendsRawBytes) {
    UdpSocket server;
    UdpSocket client;
    const std::array<std::byte, 3> data{
        std::byte{0x00},
        std::byte{0xFF},
        std::byte{0x2A},
    };

    client.send(data, {.address = "127.0.0.1", .port = server.localPort()});
    std::array<std::byte, 8> buffer{};
    Endpoint sender;
    ASSERT_EQ(receiveWithin(server, buffer, sender), data.size());
    EXPECT_EQ(buffer[0], data[0]);
    EXPECT_EQ(buffer[1], data[1]);
    EXPECT_EQ(buffer[2], data[2]);
}

TEST(UdpSocket, TruncatesDatagramsBiggerThanTheBuffer) {
    UdpSocket server;
    UdpSocket client;

    client.send("hello", {.address = "127.0.0.1", .port = server.localPort()});
    std::array<std::byte, 2> buffer{};
    Endpoint sender;
    EXPECT_EQ(receiveWithin(server, buffer, sender), buffer.size());
}

TEST(UdpSocket, ReceiveReturnsImmediatelyWhenNothingArrived) {
    UdpSocket socket;
    std::array<std::byte, 8> buffer{};
    Endpoint sender;
    EXPECT_EQ(socket.receive(buffer, sender), std::nullopt);
}

TEST(UdpSocket, IsMovable) {
    UdpSocket first;
    const auto port = first.localPort();
    UdpSocket second(std::move(first));
    EXPECT_EQ(second.localPort(), port);
}

TEST(UdpSocket, MovedFromSocketThrows) {
    UdpSocket first;
    const UdpSocket second(std::move(first));
    std::array<std::byte, 8> buffer{};
    Endpoint sender;

    // NOLINTBEGIN(bugprone-use-after-move,clang-analyzer-cplusplus.Move):
    // checking the moved-from state
    EXPECT_THROW((void)first.localPort(), NetError);
    EXPECT_THROW(first.send("hello", {.address = "127.0.0.1",
                                      .port = second.localPort()}),
                 NetError);
    EXPECT_THROW((void)first.receive(buffer, sender), NetError);
    // NOLINTEND(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
}

TEST(UdpSocket, MoveAssignmentKeepsTheSocket) {
    UdpSocket first;
    const auto port = first.localPort();
    UdpSocket second;
    second = std::move(first);
    EXPECT_EQ(second.localPort(), port);
}

TEST(UdpSocket, ThrowsWhenThePortIsTaken) {
    const UdpSocket first;
    EXPECT_THROW(UdpSocket{first.localPort()}, NetError);
}

TEST(UdpSocket, ThrowsOnInvalidAddress) {
    UdpSocket socket;
    EXPECT_THROW(
        socket.send("hello", {.address = "not an address", .port = 4242}),
        NetError);
}

TEST(UdpSocket, ThrowsOnUnknownHost) {
    EXPECT_THROW((void)UdpSocket::resolve("host.invalid", 4242), NetError);
}

TEST(NetStats, StartsAtZero) {
    const UdpSocket socket;
    const NetStats& stats = socket.stats();
    EXPECT_EQ(stats.packetsSent, 0U);
    EXPECT_EQ(stats.packetsReceived, 0U);
    EXPECT_EQ(stats.bytesSent, 0U);
    EXPECT_EQ(stats.bytesReceived, 0U);
    EXPECT_EQ(stats.packetsDropped, 0U);
}

TEST(NetStats, CountsSentAndReceivedTraffic) {
    UdpSocket server;
    UdpSocket client;
    const Endpoint serverEndpoint =
        UdpSocket::resolve("127.0.0.1", server.localPort());

    client.send("hello", serverEndpoint);
    client.send("salut!", serverEndpoint);
    Endpoint sender;
    EXPECT_EQ(receiveString(server, sender), "hello");
    EXPECT_EQ(receiveString(server, sender), "salut!");

    EXPECT_EQ(client.stats().packetsSent, 2U);
    EXPECT_EQ(client.stats().bytesSent, 11U);
    EXPECT_EQ(client.stats().packetsReceived, 0U);
    EXPECT_EQ(client.stats().bytesReceived, 0U);
    EXPECT_EQ(client.stats().packetsDropped, 0U);

    EXPECT_EQ(server.stats().packetsReceived, 2U);
    EXPECT_EQ(server.stats().bytesReceived, 11U);
    EXPECT_EQ(server.stats().packetsSent, 0U);
    EXPECT_EQ(server.stats().bytesSent, 0U);
}

TEST(NetStats, CountsRawBytes) {
    UdpSocket server;
    UdpSocket client;
    const std::array<std::byte, 3> data{};

    client.send(data, {.address = "127.0.0.1", .port = server.localPort()});

    EXPECT_EQ(client.stats().packetsSent, 1U);
    EXPECT_EQ(client.stats().bytesSent, data.size());
}

TEST(NetStats, CountsOnlyTheBytesWrittenInTheBuffer) {
    UdpSocket server;
    UdpSocket client;

    client.send("hello", {.address = "127.0.0.1", .port = server.localPort()});
    std::array<std::byte, 2> buffer{};
    Endpoint sender;
    ASSERT_EQ(receiveWithin(server, buffer, sender), buffer.size());

    EXPECT_EQ(server.stats().packetsReceived, 1U);
    EXPECT_EQ(server.stats().bytesReceived, buffer.size());
}

TEST(NetStats, EmptyReceiveCountsNothing) {
    UdpSocket socket;
    std::array<std::byte, 8> buffer{};
    Endpoint sender;

    EXPECT_EQ(socket.receive(buffer, sender), std::nullopt);

    EXPECT_EQ(socket.stats().packetsReceived, 0U);
    EXPECT_EQ(socket.stats().bytesReceived, 0U);
}

TEST(NetStats, FailedSendCountsNothing) {
    UdpSocket socket;

    EXPECT_THROW(
        socket.send("hello", {.address = "not an address", .port = 4242}),
        NetError);

    EXPECT_EQ(socket.stats().packetsSent, 0U);
    EXPECT_EQ(socket.stats().bytesSent, 0U);
    EXPECT_EQ(socket.stats().packetsDropped, 0U);
}

TEST(NetStats, FollowsTheSocketWhenMoved) {
    UdpSocket server;
    UdpSocket first;
    first.send("hello", {.address = "127.0.0.1", .port = server.localPort()});

    const UdpSocket second(std::move(first));

    EXPECT_EQ(second.stats().packetsSent, 1U);
    EXPECT_EQ(second.stats().bytesSent, 5U);
    // NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move):
    // stats is the one call that stays valid on a moved-from socket
    EXPECT_NO_THROW((void)first.stats());
}
