#ifndef SERVER_CORE_H
#define SERVER_CORE_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <chrono>
#include <memory>
#include <unordered_map>

// --- Custom Binary Protocol ---
enum MessageType {
    MSG_CHAT = 1,
    MSG_STATS = 2,
    MSG_PING = 3
};

#pragma pack(push, 1)
struct MessageHeader {
    uint32_t payload_length;
    uint16_t msg_type; 
};
#pragma pack(pop)

// --- Token Bucket Rate Limiter ---
class RateLimiter {
    int capacity;
    int tokens;
    int refill_rate_per_sec;
    std::chrono::steady_clock::time_point last_refill;
    std::mutex mtx;

public:
    RateLimiter(int cap, int rate) : capacity(cap), tokens(cap), refill_rate_per_sec(rate) {
        last_refill = std::chrono::steady_clock::now();
    }

    bool consume(int count = 1) {
        std::lock_guard<std::mutex> lock(mtx);
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - last_refill;
        
        int new_tokens = elapsed.count() * refill_rate_per_sec;
        if (new_tokens > 0) {
            tokens = std::min(capacity, tokens + new_tokens);
            last_refill = now;
        }

        if (tokens >= count) {
            tokens -= count;
            return true;
        }
        return false;
    }
};

// --- Thread Pool ---
class ThreadPool {
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;

public:
    ThreadPool(size_t threads) : stop(false) {
        for(size_t i = 0; i < threads; ++i)
            workers.emplace_back([this] {
                for(;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this]{ return this->stop || !this->tasks.empty(); });
                        if(this->stop && this->tasks.empty()) return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
    }

    void enqueue(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.push(std::move(task));
        }
        condition.notify_one();
    }

    size_t queue_size() {
        std::unique_lock<std::mutex> lock(queue_mutex);
        return tasks.size();
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for(std::thread &worker: workers) worker.join();
    }
};

// --- Connection Context ---
struct Connection {
    int fd;
    std::vector<uint8_t> read_buf;
    std::vector<uint8_t> write_buf;
    std::mutex write_mtx;
    RateLimiter rate_limiter;

    Connection(int fd) : fd(fd), rate_limiter(20, 10) {} // 20 burst, 10 req/sec limit
};

#endif