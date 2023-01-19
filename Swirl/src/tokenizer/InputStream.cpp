#include <sstream>
#include <tokenizer/InputStream.h>

InputStream::InputStream(std::string& _source): m_Source(_source) {
    std::stringstream src_strm(_source);
}

char InputStream::peek() {
    return m_Source.at(m_Pos);
}

char InputStream::next(bool _noIncrement) {
    if (_noIncrement) {
        char chr = m_Source.at(m_Pos + 1);
        return chr;
    }

    char chr = m_Source.at(m_Pos++);
    if (chr == '\n') { m_Line++; m_Col = 0; }
    else { m_Col++; }
    return chr;
}

void InputStream::reset() {
    m_Col = m_Pos = 0; m_Line = 1;
}

bool InputStream::eof() {
    return m_Pos == m_Source.size();
}

std::size_t InputStream::getPos() const {
    return m_Pos;
}

std::size_t InputStream::getLine() const {
    return m_Line;
}

std::size_t InputStream::getCol() const {
    return m_Col;
}
