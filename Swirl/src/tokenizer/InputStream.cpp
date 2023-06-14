#include <sstream>
#include <string>
#include <tokenizer/InputStream.h>

InputStream::InputStream(std::string& _source): m_Source(_source) {}

char InputStream::peek() {
    return m_Source.at(Pos);
}

char InputStream::next() {
//    if (_noIncrement) {
//        char chr = m_Source.at(Pos + 1);
//        return chr;
//    }

    char chr = m_Source.at(Pos++);
    if (chr == '\n') { Line++; Col = 0;}
    else {
        Col++;
    }

    return chr;
}

void InputStream::reset() {
    Col = Pos = 0; Line = 1;
}

bool InputStream::eof() {
    return Pos == m_Source.size();
}

