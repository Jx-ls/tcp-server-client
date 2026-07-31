#include "../include/utils.h"
#include "../include/server_core.h"
#include <string>
#include <thread>
#include <atomic>

std::atomic<bool> running{true};

void send_packet(int sockfd, uint16_t msg_type, const std::string& payload) {
    MessageHeader hdr;
    hdr.payload_length = payload.size();
    hdr.msg_type = msg_type;

    std::vector<uint8_t> buffer(sizeof(MessageHeader) + payload.size());
    memcpy(buffer.data(), &hdr, sizeof(MessageHeader));
    memcpy(buffer.data() + sizeof(MessageHeader), payload.data(), payload.size());

    Write(sockfd, buffer.data(), buffer.size());
}

void receive_loop(int sockfd) {
    std::vector<uint8_t> buffer;
    char temp[4096];

    while (running) {
        ssize_t n = read(sockfd, temp, sizeof(temp));
        if (n > 0) {
            buffer.insert(buffer.end(), temp, temp + n);

            while (buffer.size() >= sizeof(MessageHeader)) {
                MessageHeader hdr;
                memcpy(&hdr, buffer.data(), sizeof(MessageHeader));

                if (buffer.size() >= sizeof(MessageHeader) + hdr.payload_length) {
                    std::string payload(
                        buffer.begin() + sizeof(MessageHeader), 
                        buffer.begin() + sizeof(MessageHeader) + hdr.payload_length
                    );
                    buffer.erase(buffer.begin(), buffer.begin() + sizeof(MessageHeader) + hdr.payload_length);

                    // Clear current line, print incoming broadcast/response cleanly, and restore prompt
                    cout << "\r\33[2K" << payload << "\n> " << flush;
                } else {
                    break;
                }
            }
        } else if (n == 0) {
            cout << "\nServer closed the connection.\n";
            running = false;
            break;
        } else {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                break;
            }
        }
    }
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
    SetNonBlocking(sockfd);
    cout << "Connected to server successfully!\nType messages or '/ping' to test latency.\n";

    std::thread listener(receive_loop, sockfd);
    listener.detach();

    cout << "> " << flush;

    string msg;
    while (running && getline(cin, msg)) {
        if (msg.empty()) {
            cout << "> " << flush;
            continue;
        }

        // Clear the user's typed line instantly to let the server's broadcast take over cleanly
        cout << "\33[A\33[2K\r" << flush;

        if (msg == "/ping") {
            send_packet(sockfd, MSG_PING, "ping");
        } else {
            send_packet(sockfd, MSG_CHAT, msg);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    Close(sockfd);
    return 0;
}