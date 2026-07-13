<div align="center">

# TCP Server-Client in C++
_Exploring low-level networking with POSIX sockets, TCP/IP, and Linux system calls in C++._

![](https://img.shields.io/badge/C%2B%2B-20-00599C?style=flat-square&logo=cplusplus&logoColor=white)
![](https://img.shields.io/badge/g%2B%2B-GNU%20Compiler-4EAA25?style=flat-square&logo=gnu&logoColor=white)
![](https://img.shields.io/badge/Linux-POSIX%20Sockets-FCC624?style=flat-square&logo=linux&logoColor=black)
![](https://img.shields.io/badge/TCP-IPv4-0A66C2?style=flat-square)
<br/>
![](https://img.shields.io/badge/Build-Make-427819?style=flat-square&logo=gnu&logoColor=white)
![](https://img.shields.io/badge/Networking-Socket%20Programming-blue?style=flat-square)
![](https://img.shields.io/badge/Platform-Linux-lightgrey?style=flat-square&logo=linux)
[![](https://img.shields.io/github/license/Jx-ls/tcp-server-client?style=flat-square)](./LICENSE)

</div>

## Features

- TCP communication using POSIX sockets
- Separate client and server executables
- Reusable wrapper functions for socket system calls
- Automatic error handling with descriptive messages
- Simple Makefile-based build system
- Clean project structure

## Project Structure

```text
.
├── include/
│   └── utils.h          # Socket wrapper function declarations
├── src/
│   ├── client.cpp       # TCP client
│   ├── server.cpp       # TCP server
│   └── utils.cpp        # Wrapper implementations
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
Connected
```

### Start the client

```bash
./bin/client 127.0.0.1 8080
```

Example client output:

```text
Thu Jul 03 03:27:45 2025
```

## Technologies Used

- C++20
- POSIX Socket API
- Linux System Calls
- GNU Make
- g++

## Concepts Demonstrated

- TCP/IP networking
- Client-server architecture
- IPv4 socket programming
- Blocking I/O
- Network byte order (`htonl`, `htons`)
- IP address conversion (`inet_pton`)
- Socket lifecycle management
- Error handling using wrapper functions

## Future Improvements

- Multi-threaded server for handling multiple concurrent client connections
- Non-blocking sockets using `fcntl()` and I/O multiplexing with `epoll()`
- Token Bucket rate limiting to control client request rates

## Build Requirements

- Linux
- g++ (C++20 compatible)
- GNU Make

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.