# Networking

The net module moves raw bytes over UDP. It knows nothing about any game
protocol: building and reading packets is up to the game.

Asio stays hidden inside the module, so game code only includes
`<ampersand/net/UdpSocket.hpp>`.

## UdpSocket

`ampersand::net::UdpSocket` is a blocking IPv4 UDP socket.

| Method                      | Role                                                    |
| --------------------------- | ------------------------------------------------------- |
| `UdpSocket()`               | Binds to a port picked by the system (clients).         |
| `UdpSocket(port)`           | Binds to a given port (servers).                        |
| `resolve(host, port)`       | Turns `"localhost"` or `"127.0.0.1"` into an `Endpoint`. |
| `send(data, endpoint)`      | Sends one datagram (bytes or a string).                 |
| `receive(buffer, endpoint)` | Waits for one datagram and fills the sender endpoint.   |
| `localPort()`               | The port the socket is bound to.                        |

Errors (port already used, unknown host...) are thrown as `ampersand::net::NetError`.

```cpp
#include <ampersand/net/UdpSocket.hpp>

using namespace ampersand::net;

// Server
UdpSocket server(4242);
std::array<std::byte, 1024> buffer{};
Endpoint client;
const std::size_t length = server.receive(buffer, client);
server.send("ok", client);

// Client
UdpSocket socket;
socket.send("hello", UdpSocket::resolve("localhost", 4242));
```
