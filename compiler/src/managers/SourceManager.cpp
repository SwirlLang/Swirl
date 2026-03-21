#include <print>
#include <string>
#include <fstream>
#include <array>

#include "CompilerInst.h"
#include "managers/SourceManager.h"
#include "utils/FileSystem.h"


SourceManager::SourceManager(sw::FileHandle* file_handle): m_SourcePath(file_handle->getPath()) {
    std::size_t pos = 0;
    auto stream = std::move(file_handle->getStream());

    for (std::string line; std::getline(*stream, line); ) {
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
        // line_size = 0;
        m_CurrentLine.clear();
        if (Pos != m_Source.size()) {
            Line++;
        } Col = 0;

    } else {
        // line_size++;
        Col++;
        m_CurrentLine += chr;
    }

    return chr;
}


std::string SourceManager::getLineAt(const std::size_t line) const {
    auto [from, line_size] = m_LineOffsets[line - 1];
    return m_Source.substr(from, line_size);
}

std::optional<std::string> SourceManager::getEnumeratedLine(std::size_t at, std::size_t max_line_no) {
    return {};
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

StreamState SourceManager::getStreamState() const {
    return {.Line = Line, .Pos = Pos, .Col = Col};
}

void SourceManager::setStreamState(const StreamState& to) {
    Pos = to.Pos;
    Col = to.Col;
    Line = to.Line;
}


void SourceManager::switchSource(const std::string& source) {
    std::size_t pos = Pos;
    std::istringstream iss(source);

    for (std::string line; std::getline(iss, line); ) {
        line += '\n';
        m_LineOffsets.push_back({pos, line.size()});
        m_Source += line;
        pos += line.size();
    }
}


void SourceManager::switchSource(const SourceLocation& loc) {
    switchSource(m_Source.substr(loc.from.Pos, loc.to.Pos - loc.from.Pos));
}

void SourceManager::switchSource(const std::size_t from, const std::size_t to) {
    std::string new_source;
    for (auto i = from; i <= to; i++) {
         new_source += getLineAt(i);
    } switchSource(new_source);
    // reset();
}

