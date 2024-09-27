#include <sstream>
#include <string>
#include <tokenizer/InputStream.h>

std::size_t prev_col_state = 0;

InputStream::InputStream(const std::string_view _source): m_Source(_source) {}

char InputStream::peek() const {
    return m_Source.at(Pos);
}

char InputStream::getCurrentChar() const {
    return m_CurrentChar;
};

char InputStream::next() {
    const char chr = m_Source.at(Pos);
    m_CurrentChar = chr;
    Pos++;

    if (chr == '\n') {
        prev_col_state = Col;

        LineMap[Line] = m_CurrentLine;
        m_CurrentLine.clear();
        Line++;
        Col = 0;

    } else {
        Col++;
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

void InputStream::reset() {
    Col = Pos = 0; Line = 1;
}

bool InputStream::eof() const {
    return Pos == m_Source.size();
}

std::string InputStream::getCurrentLine() const {
    return m_CurrentLine;
}



