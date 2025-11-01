#pragma once
#include <thread>
#include <future>
#include <queue>
#include <utility>

namespace sw {
using u32      = uint32_t;
using Task     = std::packaged_task<void ()>;
using Future_t = std::future<void>;

class Queue {
    std::mutex m_Mutex;
    std::queue<Task> m_Tasks;
    std::condition_variable_any m_CV;

public:
    void push(Task task) {
        {
            std::unique_lock lock(m_Mutex);
            m_Tasks.push(std::move(task));
        }
        m_CV.notify_one();
    }

    std::optional<Task> pop(std::stop_token tok) {
        std::unique_lock lock(m_Mutex);
        m_CV.wait(lock, tok, [this, tok]{ return !m_Tasks.empty() || tok.stop_requested(); });
        if (m_Tasks.empty()) return std::nullopt;
        auto ret = std::move(m_Tasks.front());
        m_Tasks.pop();
        return ret;
    }

    void notify_all() {
        m_CV.notify_all();
    }
};

class ThreadPool {
public:
    void setBaseThreadCount(u32 v = std::thread::hardware_concurrency()) {
        m_BaseThreadCount = v;
        for (u32 _ = 0; _ < v; ++_) {
            m_Threads.emplace_back([this](std::stop_token tok) {
                threadRuntime(tok);
            });
        }
    }

    void enqueue(std::function<void()> callable) {
        if (!m_BaseThreadCount.has_value()) {
            callable();
            return;
        }

        auto task = Task(std::move(callable));
        m_Futures.push_back(task.get_future());
        m_WorkPool.push(std::move(task));
    }

    void wait() {
        for (auto& fut : m_Futures) fut.get();
        m_Futures.clear();
    }

    ~ThreadPool() {
        for (auto& t : m_Threads) t.request_stop();
        m_WorkPool.notify_all();
        for (auto& t : m_Threads) if (t.joinable()) t.join();
    }

private:
    Queue                     m_WorkPool;
    std::optional<u32>        m_BaseThreadCount;
    std::vector<Future_t>     m_Futures;
    std::vector<std::jthread> m_Threads;

    void threadRuntime(std::stop_token tok) {
        while (!tok.stop_requested()) {
            auto taskOpt = m_WorkPool.pop(tok);
            if (!taskOpt) break;
            (*taskOpt)();
        }
    }
};
}
