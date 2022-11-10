#include <sstream>
#include <tokenizer/InputStream.h>

InputStream::InputStream(std::string& _source): m_Source(_source) {
    std::stringstream src_strm(_source);
    for (std::string cr_ln; std::getline(src_strm, cr_ln); m_TotalLns++ ) {}
}

char InputStream::peek() {
    return m_Source.at(m_Pos);
}

char InputStream::next() {
    char chr = m_Source.at(m_Pos++);
    return chr;
}

bool InputStream::eof() {
    return peek() == '\0';
}

void InputStream::raiseException(const char* message) {
    // TODO
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
