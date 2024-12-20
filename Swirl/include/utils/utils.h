#pragma once
#include <string>

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

