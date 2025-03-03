#pragma once
#include <string>
#include <thread>
#include <vector>
#include <queue>
#include <functional>


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


class ThreadPool_t {
    class Thread {
        std::thread m_Handle;
        ThreadPool_t& m_PoolInstance;
        std::size_t m_CoolDown;
        bool m_Stop = false;
        
        void task() {
            while (!m_Stop) {
                auto task = m_PoolInstance.getTask();
                if (task)
                    (*task)();
            }
        }
    
    public:
        explicit Thread(ThreadPool_t& inst): m_PoolInstance(inst) {
            m_Handle = std::thread{[this]() {
                task();
            }};
        };
    
        void stop() {
            m_Stop = true;
            m_Handle.join();
        }
    };
    
    std::vector<Thread> m_Threads;
    std::queue<std::function<void()>> m_TaskQueue;
    std::mutex m_QMutex;

public:
    ThreadPool_t() = default;

    explicit ThreadPool_t(int base_thread_counts) {
        // TODO: handle the edge-case of 0
        m_Threads.reserve(base_thread_counts);
        while (base_thread_counts--)
            m_Threads.emplace_back(*this);
    }
    
    template <typename Fn>
    void addTask(Fn callable) {
        m_TaskQueue.emplace(std::move(callable));
    }

    void shutdown() {
        for (auto& thread : m_Threads)
            thread.stop();
    }
    
    std::optional<std::function<void()>> getTask() {
        std::lock_guard guard{m_QMutex};
    
        if (!m_TaskQueue.empty()) {
            auto ret = m_TaskQueue.front();
            m_TaskQueue.pop();
            return ret;
        } return std::nullopt;
    }
};

inline int minEditDistance(const std::string_view word1, const std::string_view word2) {
    const std::size_t m = word1.size();
    const std::size_t n = word2.size();

    std::vector dp(m + 1, std::vector(n + 1, 0));

    for (int i = 0; i <= m; ++i)
        dp[i][0] = i;

    for (int j = 0; j <= n; ++j)
        dp[0][j] = j;

    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            if (word1[i - 1] == word2[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            } else {
                dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
            }
        }
    }

    return dp[m][n];
}

