#include "utils.h"

int main (int argc, char **argv) {
    if (argc != 3)
        cerr << "Usage: client <IP_ADRESS> <PORT>\n", exit(EXIT_FAILURE);
    
    int                 sockfd, n;
    struct sockaddr_in  servaddr{};
    char                recvline[MAXLINE + 1];

    sockfd = Socket(AF_INET, SOCK_STREAM, 0);

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    if ( inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
        cerr << "Invalid IP: " << strerror(errno) << "\n", exit(EXIT_FAILURE);

    Connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    n = Read(sockfd, recvline, MAXLINE);
    recvline[n] = 0;
    cout << recvline;
    return 0;
}