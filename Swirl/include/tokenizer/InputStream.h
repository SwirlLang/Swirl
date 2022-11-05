#include <iostream>
#include <vector>

#ifndef INPUT_STREAM_H_Swirl
#define INPUT_STREAM_H_Swirl


class InputStream {
    std::string m_Source;
    std::size_t m_TotalLns = 0, m_LstChars = 0, m_Pos = 0, m_Line = 0, m_Col = 0;
public:
    explicit InputStream(std::string& _source);

    /** @brief Returns the next value without discarding it */
    char peek();

    /** @brief returns the next value and discards it */
    char next();

    /** @brief returns true if no more chars are left in the stream */
    bool eof();

    /** @brief raises an exception */
    void raiseException(const char* message);

    std::size_t getPos() const;
    std::size_t getLine() const;
    std::size_t getCol() const;
};

#endif