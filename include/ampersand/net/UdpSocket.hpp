#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ampersand::net {

/** @brief Thrown when a socket operation fails (bind, resolve, send...). */
class NetError : public std::runtime_error {
public:
    /**
     * @brief Creates the error.
     *
     * @param message Description of the error.
     */
    explicit NetError(const std::string& message)
        : std::runtime_error("Network error: " + message) {}
};

/**
 * @brief An IPv4 address and a port, identifying a peer on the network.
 *
 * The address is always numeric (e.g. "127.0.0.1"); use UdpSocket::resolve
 * to turn a host name into an Endpoint.
 */
struct Endpoint {
    std::string address;
    std::uint16_t port{0};

    bool operator==(const Endpoint& other) const = default;

    /** @brief Formats the endpoint as "address:port". */
    [[nodiscard]] std::string toString() const {
        return this->address + ":" + std::to_string(this->port);
    }
};

/**
 * @brief A blocking IPv4 UDP socket that sends and receives raw datagrams.
 *
 * It knows nothing about the game protocol: it only moves bytes between
 * endpoints. Asio is hidden behind it, so code using the socket does not
 * need to include it.
 */
class UdpSocket {
public:
    /** @brief Opens a socket bound to a port picked by the system. */
    UdpSocket();

    /**
     * @brief Opens a socket bound to the given port on every interface.
     *
     * @param port Local port to listen on.
     * @throws NetError If the port cannot be bound (already in use...).
     */
    explicit UdpSocket(std::uint16_t port);

    ~UdpSocket();

    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;
    UdpSocket(UdpSocket&&) noexcept;
    UdpSocket& operator=(UdpSocket&&) noexcept;

    /**
     * @brief Resolves a host name or address into an IPv4 endpoint.
     *
     * @param host Host name ("localhost") or numeric address ("127.0.0.1").
     * @param port Remote port.
     * @throws NetError If the host cannot be resolved.
     */
    [[nodiscard]] static Endpoint resolve(const std::string& host,
                                          std::uint16_t port);

    /**
     * @brief Sends one datagram to an endpoint.
     *
     * @param data Bytes to send.
     * @param target Destination.
     * @throws NetError If the datagram cannot be sent.
     */
    void send(std::span<const std::byte> data, const Endpoint& target);

    /** @brief Sends the characters of a string as one datagram. */
    void send(std::string_view data, const Endpoint& target);

    /**
     * @brief Waits for one datagram and copies it into a buffer.
     *
     * Bytes beyond the buffer size are lost, as with any UDP socket.
     *
     * @param buffer Where to store the received bytes.
     * @param from Filled with the sender's endpoint.
     * @return The number of bytes written into the buffer.
     * @throws NetError If the receive fails.
     */
    std::size_t receive(std::span<std::byte> buffer, Endpoint& from);

    /** @brief The local port the socket is bound to. */
    [[nodiscard]] std::uint16_t localPort() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace ampersand::net
