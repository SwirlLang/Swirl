#include <iostream>
#include <vector>

#ifndef INPUT_STREAM_H_Swirl
#define INPUT_STREAM_H_Swirl


class InputStream {
public:
    std::size_t Pos = 0, Line = 1, Col = 0;
    explicit InputStream(std::string& _source);

    /** @brief Returns the next value without discarding it */
    char peek();

    /** @brief returns the next value and discards it */
    char next(bool _noIncrement = false);

    /** @brief returns true if no more chars are left in the stream */
    bool eof();

    /** @brief resets the state of the stream */
    void reset();
private:
    std::string m_Source{};
};

#endif