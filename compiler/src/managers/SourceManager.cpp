#include <print>
#include <string>
#include <fstream>
#include <managers/SourceManager.h>
#include <array>

std::size_t prev_col_state = 0;

SourceManager::SourceManager(const std::filesystem::path& file_path): m_SourcePath(file_path) {
    std::ifstream file_stream{file_path};

    std::size_t pos = 0;

    for (std::string line; std::getline(file_stream, line); ) {
        line += '\n';
        m_LineOffsets.push_back({pos, line.size()});
        m_Source += line;
        pos += line.size();
    }
}

char SourceManager::peek() const {
    return m_Source.at(Pos);
}

char SourceManager::peekDeeper() const {
    return m_Source.at(Pos + 1);
}

char SourceManager::getCurrentChar() const {
    return m_CurrentChar;
}

char SourceManager::next() {
    const char chr = m_Source.at(Pos);
    m_CurrentChar = chr;

    // static std::size_t line_size = 0;
    Pos++;

    if (chr == '\n') {
        prev_col_state = Col;
        // line_size = 0;
        m_CurrentLine.clear();
        if (Pos != m_Source.size()) Line++;
        Col = 0;

    } else {
        // line_size++;
        Col++;
        m_CurrentLine += chr;
    }

    return chr;
}


std::string SourceManager::getLineAt(const std::size_t line) const {
    auto [from, line_size] = m_LineOffsets.at(line - 1);
    return m_Source.substr(from, line_size);
}


void SourceManager::reset() {
    Col = Pos = 0; Line = 1;
}

bool SourceManager::almostEOF() const {
    return Pos == m_Source.size() - 1;
}

bool SourceManager::eof() const {
    return Pos == m_Source.size();
}

std::string SourceManager::getCurrentLine() const {
    return m_CurrentLine;
}

void SourceManager::switchSource(const std::size_t from, const std::size_t to) {
    std::string new_source;
    for (auto i = from; i <= to; i++) {
        new_source += getLineAt(i);
    }

    m_Source = std::move(new_source);
    reset();
}

