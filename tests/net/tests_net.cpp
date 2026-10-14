#include <gtest/gtest.h>

#include <ampersand/net/UdpSocket.hpp>
#include <array>
#include <cstddef>
#include <string>
#include <utility>

using ampersand::net::Endpoint;
using ampersand::net::NetError;
using ampersand::net::UdpSocket;

namespace {

std::string receiveString(UdpSocket& socket, Endpoint& from) {
    std::array<std::byte, 1024> buffer{};
    const std::size_t length = socket.receive(buffer, from);
    return {reinterpret_cast<const char*>(buffer.data()), length};
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
    ASSERT_EQ(server.receive(buffer, sender), data.size());
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
    EXPECT_EQ(server.receive(buffer, sender), buffer.size());
}

TEST(UdpSocket, IsMovable) {
    UdpSocket first;
    const auto port = first.localPort();
    UdpSocket second(std::move(first));
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
