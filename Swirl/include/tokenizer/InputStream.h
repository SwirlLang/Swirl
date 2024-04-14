#include <iostream>
#include <unordered_map>
#include <vector>

#ifndef INPUT_STREAM_H_Swirl
#define INPUT_STREAM_H_Swirl


class InputStream {
    std::tuple<size_t, size_t, size_t> cache;
    std::string m_CurrentLine{};
public:
    std::size_t Pos = 0, Line = 0, Col = 0;
    std::unordered_map<std::size_t, std::string> LineMap{};

    explicit InputStream(std::string& _source);

    /** @brief Returns the next value without discarding it */
    char peek();

    /** @brief returns the next value and discards it */
    char next();

    /** @brief back off by one char **/
    void backoff();

    /** @brief returns true if no more chars are left in the m_Stream */
    bool eof();

    /** @brief saves the current state */
    void setReturnPoint();

    /** @brief restores cache */
    void restoreCache();

    /** @brief resets the state of the m_Stream */
    void reset();

    std::string getCurrentLine() const;

private:
    std::string m_Source{};
};

#endif