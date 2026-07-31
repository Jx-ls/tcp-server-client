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

using namespace std;

// Custom Binary Protocol
#pragma pack(push, 1)
struct MessageHeader {
    uint32_t payload_length;
    uint16_t msg_type; // e.g., 1 = Request, 2 = Response
};
#pragma pack(pop)

// Token Bucket Rate Limiter 
class RateLimiter {
    int capacity;
    int tokens;
    int refill_rate_per_sec;
    chrono::steady_clock::time_point last_refill;
    mutex mtx;

public:
    RateLimiter(int cap, int rate) : capacity(cap), tokens(cap), refill_rate_per_sec(rate) {
        last_refill = chrono::steady_clock::now();
    }

    bool consume(int count = 1) {
        lock_guard<mutex> lock(mtx);
        auto now = chrono::steady_clock::now();
        chrono::duration<double> elapsed = now - last_refill;
        
        int new_tokens = elapsed.count() * refill_rate_per_sec;
        if (new_tokens > 0) {
            tokens = min(capacity, tokens + new_tokens);
            last_refill = now;
        }

        if (tokens >= count) {
            tokens -= count;
            return true;
        }
        return false;
    }
};

// Thread Pool 
class ThreadPool {
    vector<thread> workers;
    queue<function<void()>> tasks;
    mutex queue_mutex;
    condition_variable condition;
    bool stop;

public:
    ThreadPool(size_t threads) : stop(false) {
        for(size_t i = 0; i < threads; ++i)
            workers.emplace_back([this] {
                for(;;) {
                    function<void()> task;
                    {
                        unique_lock<mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this]{ return this->stop || !this->tasks.empty(); });
                        if(this->stop && this->tasks.empty()) return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
    }

    void enqueue(function<void()> task) {
        {
            unique_lock<mutex> lock(queue_mutex);
            tasks.push(std::move(task));
        }
        condition.notify_one();
    }

    ~ThreadPool() {
        {
            unique_lock<mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for(thread &worker: workers) worker.join();
    }
};

// Connection Context 
struct Connection {
    int fd;
    vector<uint8_t> read_buf;
    vector<uint8_t> write_buf;
    mutex write_mtx;
    RateLimiter rate_limiter;

    Connection(int fd) : fd(fd), rate_limiter(10, 5) {} // Max 10 burst, 5 req/sec
};

#endif