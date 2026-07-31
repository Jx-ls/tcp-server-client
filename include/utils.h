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
#include <fcntl.h>
#include <time.h>
#define MAXLINE     4096
#define BUFFSIZE    8096
#define MAX_CONN    5
using namespace std;

int Socket(int domain, int type, int protocol);

void Bind(int fd, const struct sockaddr *addr, socklen_t len);

void Listen(int fd, int n);

int Accept(int fd, struct sockaddr *__restrict addr, socklen_t *__restrict addr_len);

void Connect(int fd, const struct sockaddr *addr, socklen_t len);

ssize_t Read(int fd, void *buf, size_t nbytes);

void Write(int fd, const void *buf, size_t n);

void Close(int fd);

void SetNonBlocking(int fd);