#include "../include/utils.h"
#include "../include/server_core.h"
#include <unordered_map>
#include <atomic>
#include <iomanip>
#include <sstream>

ThreadPool pool(4);
std::unordered_map<int, std::shared_ptr<Connection>> connections;
std::mutex connections_mtx;
int epollfd;

std::atomic<long> total_requests_processed{0};
auto server_start_time = std::chrono::steady_clock::now();

std::string get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm* local_time = std::localtime(&now_c);
    
    std::ostringstream oss;
    oss << std::put_time(local_time, "%H:%M:%S");
    return oss.str();
}

void modify_epoll_events(int fd, uint32_t events) {
    struct epoll_event ev{};
    ev.events = events | EPOLLET;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}

void send_packet_to_client(std::shared_ptr<Connection> conn, uint16_t msg_type, const std::string& data) {
    MessageHeader hdr;
    hdr.payload_length = data.size();
    hdr.msg_type = msg_type;

    std::vector<uint8_t> out_data(sizeof(MessageHeader) + hdr.payload_length);
    memcpy(out_data.data(), &hdr, sizeof(MessageHeader));
    memcpy(out_data.data() + sizeof(MessageHeader), data.c_str(), hdr.payload_length);

    {
        std::lock_guard<std::mutex> lock(conn->write_mtx);
        conn->write_buf.insert(conn->write_buf.end(), out_data.begin(), out_data.end());
    }
    modify_epoll_events(conn->fd, EPOLLIN | EPOLLOUT);
}

void process_message(std::shared_ptr<Connection> conn, uint16_t msg_type, std::vector<uint8_t> payload) {
    total_requests_processed++;
    std::string payload_str(payload.begin(), payload.end());
    std::string timestamp = get_current_timestamp();

    if (msg_type == MSG_CHAT) {
        std::string broadcast_msg = "[" + timestamp + "] [Client " + std::to_string(conn->fd) + "]: " + payload_str;
        cout << broadcast_msg << "\n";

        std::lock_guard<std::mutex> lock(connections_mtx);
        for (auto& pair : connections) {
            send_packet_to_client(pair.second, MSG_CHAT, broadcast_msg);
        }
    } 
    else if (msg_type == MSG_STATS) {
        auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - server_start_time).count();
        
        std::string stats = "[" + timestamp + "] === Server Metrics ===\n"
                            "Uptime: " + std::to_string(uptime) + "s\n"
                            "Active Clients: " + std::to_string(connections.size()) + "\n"
                            "Total Requests: " + std::to_string(total_requests_processed) + "\n"
                            "Thread Pool Queue: " + std::to_string(pool.queue_size());
        
        send_packet_to_client(conn, MSG_STATS, stats);
    } 
    else if (msg_type == MSG_PING) {
        send_packet_to_client(conn, MSG_PING, "[" + timestamp + "] PONG");
    }
}

void handle_read(std::shared_ptr<Connection> conn) {
    char temp_buf[4096];
    while (true) {
        ssize_t n = read(conn->fd, temp_buf, sizeof(temp_buf));
        if (n > 0) {
            conn->read_buf.insert(conn->read_buf.end(), temp_buf, temp_buf + n);
        } else if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            break; 
        } else {
            std::lock_guard<std::mutex> lock(connections_mtx);
            Close(conn->fd);
            connections.erase(conn->fd);
            cout << "Client disconnected: fd " << conn->fd << "\n";
            return;
        }
    }

    while (conn->read_buf.size() >= sizeof(MessageHeader)) {
        MessageHeader hdr;
        memcpy(&hdr, conn->read_buf.data(), sizeof(MessageHeader));

        if (conn->read_buf.size() >= sizeof(MessageHeader) + hdr.payload_length) {
            if (!conn->rate_limiter.consume()) {
                cout << "\033[33m[Rate Limit] Dropped request from fd " << conn->fd << "\033[0m\n";
                send_packet_to_client(conn, MSG_CHAT, "[Server] Rate limit exceeded. Slow down.");
                conn->read_buf.erase(conn->read_buf.begin(), conn->read_buf.begin() + sizeof(MessageHeader) + hdr.payload_length);
                continue;
            }

            std::vector<uint8_t> payload(
                conn->read_buf.begin() + sizeof(MessageHeader), 
                conn->read_buf.begin() + sizeof(MessageHeader) + hdr.payload_length
            );

            conn->read_buf.erase(conn->read_buf.begin(), conn->read_buf.begin() + sizeof(MessageHeader) + hdr.payload_length);

            pool.enqueue([conn, type = hdr.msg_type, payload]() {
                process_message(conn, type, payload);
            });
        } else {
            break; 
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
            modify_epoll_events(conn->fd, EPOLLIN);
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2) cerr << "Usage: server <PORT>\n", exit(0);

    int listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
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

    cout << "\033[32m[Server] Listening on port " << argv[1] << "...\033[0m\n";

    for (;;) {
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < nfds; ++i) {
            int current_fd = events[i].data.fd;

            if (current_fd == listenfd) {
                while (true) {
                    struct sockaddr_in cliaddr{};
                    socklen_t clilen = sizeof(cliaddr);
                    int connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
                    if (connfd < 0) break; 

                    SetNonBlocking(connfd);
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = connfd;
                    epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev);
                    
                    {
                        std::lock_guard<std::mutex> lock(connections_mtx);
                        connections[connfd] = std::make_shared<Connection>(connfd);
                    }
                    cout << "\033[36m[Connection] New client connected: fd " << connfd << "\033[0m\n";
                }
            } else if (events[i].events & EPOLLIN) {
                std::shared_ptr<Connection> c;
                {
                    std::lock_guard<std::mutex> lock(connections_mtx);
                    if (connections.count(current_fd)) c = connections[current_fd];
                }
                if (c) handle_read(c);
            } else if (events[i].events & EPOLLOUT) {
                std::shared_ptr<Connection> c;
                {
                    std::lock_guard<std::mutex> lock(connections_mtx);
                    if (connections.count(current_fd)) c = connections[current_fd];
                }
                if (c) handle_write(c);
            } else if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                std::lock_guard<std::mutex> lock(connections_mtx);
                Close(current_fd);
                connections.erase(current_fd);
                cout << "\033[31m[Disconnection] Client disconnected: fd " << current_fd << "\033[0m\n";
            }
        }
    }
}