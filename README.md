<div align="center">

# TCP Server-Client in C++
_Exploring low-level networking with POSIX sockets, TCP/IP, Linux epoll, and concurrency in C++._

![](https://img.shields.io/badge/C%2B%2B-20-00599C?style=flat-square&logo=cplusplus&logoColor=white)
![](https://img.shields.io/badge/g%2B%2B-GNU%20Compiler-4EAA25?style=flat-square&logo=gnu&logoColor=white)
![](https://img.shields.io/badge/Linux-epoll%20%26%20Non--blocking-FCC624?style=flat-square&logo=linux&logoColor=black)
![](https://img.shields.io/badge/TCP-IPv4-0A66C2?style=flat-square)
<br/>
![](https://img.shields.io/badge/Build-Make-427819?style=flat-square&logo=gnu&logoColor=white)
![](https://img.shields.io/badge/Concurrency-Thread%20Pool-blue?style=flat-square)
![](https://img.shields.io/badge/Platform-Linux-lightgrey?style=flat-square&logo=linux)
[![](https://img.shields.io/github/license/Jx-ls/tcp-server-client?style=flat-square)](./LICENSE)

</div>

## Features

- **Multi-threaded TCP Server**: Built using Linux `epoll` and non-blocking sockets for high-performance concurrent client handling.
- **Thread Pool Architecture**: Features a producer-consumer work queue to efficiently process requests under high workloads.
- **Custom Binary Protocol**: Implements robust message framing and serialization for low-overhead client-server communication.
- **Connection Lifecycle Management**: Handles client joins, disconnections, socket cleanup, and resource reclamation gracefully.
- **Token-Bucket Rate Limiting**: Built-in rate limiter to prevent request flooding and ensure fair resource allocation across connected clients.
- **Interactive Chat & Commands**: Real-time message broadcasting and server metrics tracking (`/ping`, stats).
- **Robust Error Handling**: Reusable wrapper functions with descriptive diagnostics.

## Project Structure

```text
.
├── include/
│   ├── utils.h          # Socket wrapper & non-blocking declarations
│   └── server_core.h    # Protocol, thread pool, and rate limiter definitions
├── src/
│   ├── client.cpp       # Interactive TCP client
│   ├── server.cpp       # Multi-threaded epoll server
│   └── utils.cpp        # Wrapper & fcntl implementations
├── build/               # Object files
├── bin/                 # Compiled executables
├── Makefile
├── LICENSE
└── README.md
```

## Building

Clone the repository:

```bash
git clone https://github.com/Jx-ls/tcp-server-client.git
cd tcp-server-client
```

Compile the project:

```bash
make
```

This generates the executables:

```text
bin/server
bin/client
```

To clean generated files:

```bash
make clean
```

## Running

### Start the server

```bash
./bin/server 8080
```

Server output:

```text
[Server] Listening on port 8080...
```

### Start the client

```bash
./bin/client 127.0.0.1 8080
```

Example client output:

```text
Connected to server successfully!
Type messages or '/ping' to test latency.
> Hello from client!
> /ping
PONG
```

## Technologies Used

- C++20  
- POSIX Socket API 
- Linux System Calls (epoll, fcntl, socket)
- C++ Multithreading (std::thread, std::mutex, std::condition_variable)
- GNU Make
- g++

## Concepts Demonstrated

- TCP/IP networking and client-server architecture
- High-performance I/O multiplexing with edge-triggered (EPOLLET) epoll
- Non-blocking socket configuration using fcntl()
- Producer-Consumer thread pool concurrency patterns
- Token-bucket rate limiting algorithms
- Custom binary packet framing and safe buffer serialization
- Network byte order (htonl, htons) and IP conversion (inet_pton)

## Build Requirements

- Linux
- g++ (C++20 compatible)
- GNU Make

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.