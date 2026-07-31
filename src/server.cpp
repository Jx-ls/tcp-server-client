#include "../include/utils.h"
#include "../include/server_core.h"
#include <unordered_map>

ThreadPool pool(4); // 4 worker threads
std::unordered_map<int, std::shared_ptr<Connection>> connections;
int epollfd;

void modify_epoll_events(int fd, uint32_t events) {
    struct epoll_event ev{};
    ev.events = events | EPOLLET;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}

void process_message(std::shared_ptr<Connection> conn, std::vector<uint8_t> payload) {
    // Simulated business logic
    std::string response_str = "Processed: " + std::string(payload.begin(), payload.end());
    
    // Construct outgoing frame
    MessageHeader out_hdr;
    out_hdr.payload_length = response_str.size();
    out_hdr.msg_type = 2; // Response

    std::vector<uint8_t> out_data(sizeof(MessageHeader) + out_hdr.payload_length);
    memcpy(out_data.data(), &out_hdr, sizeof(MessageHeader));
    memcpy(out_data.data() + sizeof(MessageHeader), response_str.c_str(), out_hdr.payload_length);

    // Lock and append to write buffer
    {
        std::lock_guard<std::mutex> lock(conn->write_mtx);
        conn->write_buf.insert(conn->write_buf.end(), out_data.begin(), out_data.end());
    }
    
    // Arm EPOLLOUT to notify the event loop that data is ready to send
    modify_epoll_events(conn->fd, EPOLLIN | EPOLLOUT);
}

void handle_read(std::shared_ptr<Connection> conn) {
    char temp_buf[4096];
    while (true) {
        ssize_t n = read(conn->fd, temp_buf, sizeof(temp_buf));
        if (n > 0) {
            conn->read_buf.insert(conn->read_buf.end(), temp_buf, temp_buf + n);
        } else if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            break; // Finished reading all available data
        } else {
            // Connection closed or error
            Close(conn->fd);
            connections.erase(conn->fd);
            return;
        }
    }

    // Protocol Framing: Extract messages
    while (conn->read_buf.size() >= sizeof(MessageHeader)) {
        MessageHeader hdr;
        memcpy(&hdr, conn->read_buf.data(), sizeof(MessageHeader));

        if (conn->read_buf.size() >= sizeof(MessageHeader) + hdr.payload_length) {
            // Full message received
            if (!conn->rate_limiter.consume()) {
                cout << "Rate limit exceeded for fd " << conn->fd << ". Dropping request.\n";
                // Advance buffer to drop it
                conn->read_buf.erase(conn->read_buf.begin(), conn->read_buf.begin() + sizeof(MessageHeader) + hdr.payload_length);
                continue;
            }

            std::vector<uint8_t> payload(
                conn->read_buf.begin() + sizeof(MessageHeader), 
                conn->read_buf.begin() + sizeof(MessageHeader) + hdr.payload_length
            );

            // Remove framed message from buffer
            conn->read_buf.erase(conn->read_buf.begin(), conn->read_buf.begin() + sizeof(MessageHeader) + hdr.payload_length);

            // Offload to Thread Pool
            pool.enqueue([conn, payload]() {
                process_message(conn, payload);
            });
        } else {
            break; // Wait for more data
        }
    }
}

void handle_write(std::shared_ptr<Connection> conn) {
    std::lock_guard<std::mutex> lock(conn->write_mtx);
    if (conn->write_buf.empty()) {
        modify_epoll_events(conn->fd, EPOLLIN);
        return;
    }

    ssize_t n = write(conn->fd, conn->write_buf.data(), conn->write_buf.size());
    if (n > 0) {
        conn->write_buf.erase(conn->write_buf.begin(), conn->write_buf.begin() + n);
        if (conn->write_buf.empty()) {
            modify_epoll_events(conn->fd, EPOLLIN); // Revert to only reading
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2) cerr << "Usage: server <PORT>\n", exit(0);

    int listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    SetNonBlocking(listenfd);

    struct sockaddr_in servaddr{};
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    Bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    Listen(listenfd, MAX_CONN);

    epollfd = epoll_create1(0);
    struct epoll_event ev{}, events[MAX_EVENTS];
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listenfd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);

    cout << "Server listening on port " << argv[1] << "...\n";

    for (;;) {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < nfds; ++i) {
            int current_fd = events[i].data.fd;

            if (current_fd == listenfd) {
                // Handle new connections
                while (true) {
                    struct sockaddr_in cliaddr{};
                    socklen_t clilen = sizeof(cliaddr);
                    int connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
                    if (connfd < 0) break; // EAGAIN

                    SetNonBlocking(connfd);
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = connfd;
                    epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev);
                    
                    connections[connfd] = std::make_shared<Connection>(connfd);
                    cout << "New client connected: fd " << connfd << "\n";
                }
            } else if (events[i].events & EPOLLIN) {
                if (connections.count(current_fd)) {
                    handle_read(connections[current_fd]);
                }
            } else if (events[i].events & EPOLLOUT) {
                if (connections.count(current_fd)) {
                    handle_write(connections[current_fd]);
                }
            } else if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                // Connection Lifecycle: Cleanup
                cout << "Client disconnected: fd " << current_fd << "\n";
                Close(current_fd);
                connections.erase(current_fd);
            }
        }
    }
}