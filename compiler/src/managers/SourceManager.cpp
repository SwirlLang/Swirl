#include <print>
#include <string>
#include <fstream>
#include <managers/SourceManager.h>

std::size_t prev_col_state = 0;

SourceManager::SourceManager(const std::filesystem::path& file_path) {
    m_SourceSize = file_size(file_path);
    std::ifstream file_stream{file_path};

    std::size_t ln_counter = 1;
    std::size_t pos = 0;

    for (std::string line; std::getline(file_stream, line); ) {
        line += '\n';
        m_LineOffsets[ln_counter] = {pos, line.size()};
        m_Source += line;
        pos += line.size();
        ln_counter++;
    }
}

char SourceManager::peek() const {
    return m_Source.at(Pos);
}

char SourceManager::getCurrentChar() const {
    return m_CurrentChar;
}

char SourceManager::next() {
    const char chr = m_Source.at(Pos);
    m_CurrentChar = chr;

    static std::size_t line_size = 0;
    Pos++;

    if (chr == '\n') {
        prev_col_state = Col;
        line_size = 0;
        m_CurrentLine.clear();
        Line++;
        Col = 0;

    } else {
        line_size++;
        Col++;
        m_CurrentLine += chr;
    }

    return chr;
}

void SourceManager::setReturnPoint() {
    cache = {Pos, Line, Col};
}

void SourceManager::restoreCache() {
    std::tie(Pos, Line, Col) = cache;
}

std::string SourceManager::getLineAt(const std::size_t line) {
    auto [from, line_size] = m_LineOffsets.at(line);
    return m_Source.substr(from, line_size);
}

void SourceManager::reset() {
    Col = Pos = 0; Line = 1;
}

bool SourceManager::eof() const {
    return Pos == m_SourceSize;
}

std::string SourceManager::getCurrentLine() const {
    return m_CurrentLine;
}



