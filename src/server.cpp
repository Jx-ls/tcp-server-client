#include "../include/utils.h"

int main (int argc, char **argv) 
{
    int                 listenfd, connfd;
    struct sockaddr_in  servaddr{};
    char                buff[MAXLINE];
    time_t              ticks;

    if (argc != 2)
        cerr << "Usage: server <PORT>\n", exit(0);

    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    SetNonBlocking(listenfd);

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    Bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    Listen(listenfd, MAX_CONN);

    for ( ; ; ) {
        connfd = Accept(listenfd, (struct sockaddr *) NULL, NULL);
        SetNonBlocking(connfd);
        cout << "Connected\n";

        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));

        Write(connfd, buff, strlen(buff));

        Close(connfd);
    }
}

