#pragma once
#include <cassert>
#include <string>
#include <thread>
#include <condition_variable>

namespace fs = std::filesystem;

#if defined(_WIN32)
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

#define SW_PASTE_1_(x, y) x##y
#define SW_PASTE(x, y) SW_PASTE_1_(x, y)
#define GET_UNIQUE_NAME(base) SW_PASTE(base, __COUNTER__)


std::string getWorkingDirectory(const std::string& _path);


template <typename... Args>
constexpr std::size_t combineHashes(Args... hashes) {
    return (hashes ^ ...);
}


/// A helper to define a visitor (for the very few uses of `std::visit`).
template <typename... T>
struct VisitorHelper: T... {
    using T::operator()...;
};


/// Respects the radices during conversion.
inline std::size_t toInteger(const std::string& str) {
    if (str.size() <= 2) return std::stoi(str);
    switch (str[1]) {
        case 'x':
            return std::stoi(str.substr(2), nullptr, 16);
        case 'o':
            return std::stoi(str.substr(2), nullptr, 8);
        case 'b':
            return std::stoi(str.substr(2), nullptr, 2);
        default:
            return std::stoi(str);  // base-10
    }
}


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
