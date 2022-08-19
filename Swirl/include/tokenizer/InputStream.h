#include <iostream>
#include <vector>

#ifndef TOKENIZER_H_Swirl
#define TOKENIZER_H_Swirl


class InputStream {
    std::string m_Source;
    std::size_t m_Pos = 0, m_Line = 0, m_Col = 0;
public:
    explicit InputStream(std::string& _source): m_Source(_source) {}

    /** @brief Returns the next value without discarding it */
    char peek();

    /** @brief returns the next value and discards it */
    char next();

    /** @brief returns true if no more chars are left in the stream */
    uint8_t eof();

    /** @brief raises an exception */
    void raiseException(const char* message);
};


char InputStream::peek() {
    return m_Source.at(m_Pos);
}

char InputStream::next() {
    char chr = m_Source.at(m_Pos++);
    if (chr == '\n') m_Line++, m_Col = 0; else m_Col++;
    return chr;
}

uint8_t InputStream::eof() {
    return strlen(reinterpret_cast<const char *>(peek())) == 0;
}

void InputStream::raiseException(const char* message) {
    // TODO
}

#endif