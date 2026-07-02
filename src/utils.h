#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <sys/socket.h>
#include <iostream>
#include <sys/errno.h>
#include <unistd.h>
#include <time.h>
#define MAXLINE     4096
#define BUFFSIZE    8096
#define MAX_CONN    5
using namespace std;

inline int Socket(int domain, int type, int protocol) {
    int n;
    if ( (n = socket(domain, type, protocol)) < 0) 
        cerr << "Socket Error: " << strerror(errno) << "\n", exit(EXIT_FAILURE);
    return n;
}

inline void Bind(int fd, const struct sockaddr *addr, socklen_t len) {
    int n;
    if ( (n = bind(fd, addr, len)) < 0)
        cerr << "Binding Error: " << strerror(errno) << "\n", exit(EXIT_FAILURE);
}

inline void Listen(int fd, int n) {
    int m;
    if ( (m = listen(fd, n)) < 0)
        cerr << "Listening Error: " << strerror(errno) << "\n", exit(EXIT_FAILURE);
}

inline int Accept(int fd, struct sockaddr *__restrict addr, socklen_t *__restrict addr_len) {
    int n;
    if ( (n = accept(fd, addr, addr_len)) < 0)
        cerr << "Accepting Error: " << strerror(errno) << "\n", exit(EXIT_FAILURE);
    return n;
}

inline void Connect(int fd, const struct sockaddr *addr, socklen_t len) {
    int n;
    if ( (n = connect(fd, addr, len)) < 0)
        cerr << "Connection Error: " << strerror(errno) << "\n", exit(EXIT_FAILURE);
}

inline ssize_t Read(int fd, void *buf, size_t nbytes) {
    ssize_t n;
    if ( (n = read(fd, buf, nbytes)) < 0)
        cerr << "Read Error: " << strerror(errno) << "\n", exit(EXIT_FAILURE);
    return n;
}

inline void Write(int fd, const void *buf, size_t n) {
    int m;
    if ( (m = write(fd, buf, n)) < 0)
        cerr << "Write Error: " << strerror(errno) << "\n", exit(EXIT_FAILURE);
}

inline void Close(int fd) {
    int n;
    if ( (n = close(fd)) < 0)
        cerr << "Close Error: " << strerror(errno) << "\n", exit(EXIT_FAILURE);
}