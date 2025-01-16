#include <print>
#include <sstream>
#include <string>
#include <tokenizer/InputStream.h>

std::size_t prev_col_state = 0;

InputStream::InputStream(std::string source): m_Source(std::move(source)) {}

char InputStream::peek() const {
    return m_Source.at(Pos);
}

char InputStream::getCurrentChar() const {
    return m_CurrentChar;
}

char InputStream::next() {
    const char chr = m_Source.at(Pos);
    m_CurrentChar = chr;
    Pos++;

    static std::size_t line_size = 0;

    if (chr == '\n') {
        prev_col_state = Col;
        m_LineTable[Line] = {.from = Pos - line_size, .line_size = line_size};
        std::println("adding line: {} ", Line);
        line_size = 0;
        m_CurrentLine.clear();
        Line++;
        Col = 0;

    } else {
        Col++;
        line_size++;
        m_CurrentLine += chr;
    }

    return chr;
}

void InputStream::setReturnPoint() {
    cache = {Pos, Line, Col};
}

void InputStream::restoreCache() {
    std::tie(Pos, Line, Col) = cache;
}

std::string InputStream::getLineAt(const std::size_t line) {
    auto [from, line_size] = m_LineTable.at(line);
    return m_Source.substr(from, line_size);
}

void InputStream::reset() {
    Col = Pos = 0; Line = 1;
}

bool InputStream::eof() const {
    return Pos == m_Source.size();
}

std::string InputStream::getCurrentLine() const {
    return m_CurrentLine;
}



