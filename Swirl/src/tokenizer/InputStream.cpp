#include <sstream>
#include <string>
#include <tokenizer/InputStream.h>

std::size_t prev_col_state = 0;


InputStream::InputStream(std::string& _source): m_Source(_source) {}

char InputStream::peek() {
    return m_Source.at(Pos);
}

char InputStream::next() {
    char chr = m_Source.at(Pos++);

    if (chr == '\n') {
        prev_col_state = Col;

        LineMap[Line] = m_CurrentLine;

        // TODO: an alternative to mapping each line
        m_CurrentLine.clear();

        Line++;
        Col = 0;

    } else {
        Col++;
        m_CurrentLine += chr;
    }

    return chr;
}

void InputStream::backoff() {
    char chr = m_Source.at(--Pos);
    if (chr == '\n') {
        Line--;
        Col = prev_col_state;
    } else {
        Col--;
    }
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

bool InputStream::eof() {
    return Pos == m_Source.size();
}

std::string InputStream::getCurrentLine() const {
    return m_CurrentLine;
}



