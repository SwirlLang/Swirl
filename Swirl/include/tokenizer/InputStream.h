#include <iostream>
#include <vector>

#ifndef INPUT_STREAM_H_Swirl
#define INPUT_STREAM_H_Swirl


class InputStream {
    std::string m_Source{};
    std::size_t m_Pos = 0, m_Line = 1, m_Col = 0;
public:
    explicit InputStream(std::string& _source);

    /** @brief Returns the next value without discarding it */
    char peek();

    /** @brief returns the next value and discards it */
    char next(bool _noIncrement = false);

    /** @brief returns true if no more chars are left in the stream */
    bool eof();

    /** @brief resets the state of the stream */
    void reset();

    std::size_t getPos() const;
    std::size_t getLine() const;
    std::size_t getCol() const;
};

#endif