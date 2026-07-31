#include "../include/utils.h"
#include "../include/server_core.h"
#include <string>

void send_message(int sockfd, const std::string& payload) {
    MessageHeader hdr;
    hdr.payload_length = payload.size();
    hdr.msg_type = 1;

    std::vector<uint8_t> buffer(sizeof(MessageHeader) + payload.size());
    memcpy(buffer.data(), &hdr, sizeof(MessageHeader));
    memcpy(buffer.data() + sizeof(MessageHeader), payload.data(), payload.size());

    Write(sockfd, buffer.data(), buffer.size());
}

int main(int argc, char **argv) {
    if (argc != 3)
        cerr << "Usage: client <IP_ADRESS> <PORT>\n", exit(EXIT_FAILURE);
    
    int sockfd = Socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in servaddr{};
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
        cerr << "Invalid IP: " << strerror(errno) << "\n", exit(EXIT_FAILURE);

    Connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    cout << "Connected to server. Sending requests...\n";

    // Send 15 messages quickly to trigger the rate limiter (burst is 10)
    for (int i = 0; i < 15; i++) {
        send_message(sockfd, "Hello Server, message #" + to_string(i));
    }

    // Read responses
    char recvline[4096];
    while (true) {
        ssize_t n = read(sockfd, recvline, sizeof(recvline));
        if (n > 0) {
            // Simplified parsing: Assume we read the header and payload together for the demo
            if (n >= sizeof(MessageHeader)) {
                MessageHeader* hdr = (MessageHeader*)recvline;
                std::string payload(recvline + sizeof(MessageHeader), hdr->payload_length);
                cout << "Received: " << payload << "\n";
            }
        } else if (n == 0) {
            cout << "Server closed connection.\n";
            break;
        }
    }
    
    Close(sockfd);
    return 0;
}