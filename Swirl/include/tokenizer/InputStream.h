#include <iostream>
#include <vector>

#ifndef INPUT_STREAM_H_Swirl
#define INPUT_STREAM_H_Swirl


class InputStream {
    std::tuple<size_t, size_t, size_t> cache;

public:
    std::size_t Pos = 0, Line = 1, Col = 0;

    explicit InputStream(std::string& _source);

    /** @brief Returns the next value without discarding it */
    char peek();

    /** @brief returns the next value and discards it */
    char next();

    /** @brief back off by one char **/
    void backoff();

    /** @brief returns true if no more chars are left in the stream */
    bool eof();

    /** @brief saves the current state */
    void setReturnPoint();

    /** @brief restores cache */
    void restoreCache();

    /** @brief resets the state of the stream */
    void reset();

private:
    std::string m_Source{};
};

#endif