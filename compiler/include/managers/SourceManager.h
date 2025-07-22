#pragma once
#include <unordered_map>
#include <filesystem>


class IdentInfo;

class SourceManager {
    std::tuple<size_t, size_t, size_t> cache;
    std::string m_CurrentLine;
    std::string m_Source;
    std::size_t m_SourceSize{};

    std::filesystem::path m_SourcePath;

    std::unordered_map<std::size_t, std::array<std::size_t, 2>> m_LineOffsets;  // line no. : {its starting pos, size}


public:
    std::size_t Pos = 0, Line = 1, Col = 0;
    std::unordered_map<std::size_t, std::string> LineMap{};

    explicit SourceManager(const std::filesystem::path& file_path);

    /** @brief Returns the next value without discarding it */
    char peek() const;

    char peekDeeper() const;

    /** @brief returns the next value and discards it */
    char next();

    char getCurrentChar() const;

    bool almostEOF() const;

    /** @brief returns true if no more chars are left in the m_Stream */
    bool eof() const;

    /** @brief resets the state of the m_Stream */
    void reset();

    /** @brief returns a const-ref to the source's path */
    const std::filesystem::path& getSourcePath() const {
        return m_SourcePath;
    }

    std::string getLineAt(std::size_t) const;
    std::string getCurrentLine() const;

private:
    char m_CurrentChar{};
};
