#pragma once
#include <cassert>
#include <string>
#include <thread>
#include <queue>
#include <functional>
#include <condition_variable>

namespace fs = std::filesystem;


#if defined(_WIN32)
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

std::string getWorkingDirectory(const std::string& _path);


template <typename... Args>
std::size_t combineHashes(Args... hashes) {
    return (hashes ^ ...);
}

template <typename T1, typename T2>
struct std::hash<std::pair<T1, T2>> {  // WHY?!?!!1
    std::size_t operator()(const std::pair<T1, T2>& pair) const noexcept {
        const auto h1 = std::hash<T1>{}(pair.first);
        const auto h2 = std::hash<T2>{}(pair.second);
        return combineHashes(h1, h2);
    }
};


/// A helper to execute a callback when a scope ends
template <typename Fn>
class ScopeEndCallback {
    Fn m_Callback;

public:
    explicit ScopeEndCallback(Fn callback) : m_Callback(callback) {}
    ~ScopeEndCallback() { m_Callback(); }
};


/// A lock guard but with a destructor callback
template <typename Mtx, typename Fn>
class LockGuard_t {
    std::lock_guard<Mtx> m_Guard;
    Fn m_Callback;

public:
    LockGuard_t(Mtx& mutex, Fn callback): m_Guard{mutex}, m_Callback(callback) {}
    ~LockGuard_t() { m_Callback(); }
};

template <typename Mtx, typename Fn>
LockGuard_t(Mtx&, Fn) -> LockGuard_t<Mtx, Fn>;


class ThreadPool_t {
    class Thread {
        std::thread m_Handle;
        ThreadPool_t& m_PoolInstance;
        std::size_t m_CoolDown{};
        bool m_Stop = false;

        void task() const {
            while (true) {
                std::unique_lock lock{m_PoolInstance.m_QMutex};
                m_CV.wait(lock, [this] {
                    return !this->m_PoolInstance.m_TaskQueue.empty() || m_Stop;
                });

                if (m_Stop && m_PoolInstance.m_TaskQueue.empty())
                    return;

                const std::function todo = std::move(m_PoolInstance.m_TaskQueue.front());
                m_PoolInstance.m_TaskQueue.pop();
                lock.unlock();
                todo();
            }
        }
    
    public:
        explicit Thread(ThreadPool_t& inst): m_PoolInstance(inst) {
            m_Handle = std::thread{[this]() {
                task();
            }};
        };
    
        void stop() {
            {
                std::lock_guard lock{m_PoolInstance.m_QMutex};
                m_Stop = true;
            }

            m_CV.notify_all();
            m_Handle.join();
        }
    };

    std::vector<Thread> m_Threads;
    std::queue<std::function<void()>> m_TaskQueue;
    std::mutex m_QMutex;

    inline static std::condition_variable m_CV;
    friend ThreadPool_t;

public:
    ThreadPool_t() = default;

    explicit ThreadPool_t(int base_thread_counts) {
        // TODO: handle the edge-case of 0
        assert(base_thread_counts > 0);
        m_Threads.reserve(base_thread_counts);
        while (base_thread_counts--)
            m_Threads.emplace_back(*this);
    }
    
    template <typename Fn>
    void enqueue(Fn callable) {
        {
            std::lock_guard lock{m_QMutex};
            m_TaskQueue.emplace(std::move(callable));
        } m_CV.notify_one();
    }

    void shutdown() {
        for (auto& thread : m_Threads)
            thread.stop();
    }
};

