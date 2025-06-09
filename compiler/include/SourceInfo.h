#pragma once
#include <array>
#include <stdexcept>
#include <string>
#include <unordered_map>


class IdentInfo;
class Type;

class SourceInfo {
    const std::string& m_Source;  // a reference to the source-string owned by SourceManager

    // This table maps line no. (1-indexed) to their {start position, size}
    std::unordered_map<std::size_t, std::array<std::size_t, 2>> m_LineTable;

    // This table maps decls' (e.g. functions, variables) identifiers to their:
    // start-line (inclusive), end-line (exclusive (past the span))
    std::unordered_map<IdentInfo*, std::array<std::size_t, 2>> m_DeclSpanTable;


public:
    explicit
    SourceInfo(const std::string& source): m_Source(source) {}

    void registerDeclSpan(IdentInfo* id, const std::size_t start, const std::size_t end) {
        if (m_DeclSpanTable.contains(id)) {
            throw std::runtime_error("SourceInfo::registerDeclSpan: double registration of id!");
        } m_DeclSpanTable[id] = {start, end};
    }

    std::unordered_map<std::size_t, std::array<std::size_t, 2>>&
        getLineTableRef() { return m_LineTable; }
};

