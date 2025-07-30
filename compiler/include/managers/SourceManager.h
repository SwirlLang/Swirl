#pragma once
#include <vector>
#include <filesystem>


class IdentInfo;

class SourceManager {
    std::tuple<size_t, size_t, size_t> cache;
    std::string m_CurrentLine;
    std::string m_Source;

    std::filesystem::path m_SourcePath;

    std::vector<std::array<std::size_t, 2>> m_LineOffsets;  // (line no. - 1) : {its starting pos, size}


public:
    std::size_t Pos = 0, Line = 1, Col = 0;

    explicit SourceManager(const std::filesystem::path& file_path);

    /** @brief Returns the next value without discarding it */
    [[nodiscard]] char peek() const;

    [[nodiscard]] char peekDeeper() const;

    /** @brief returns the next value and discards it */
    char next();

    [[nodiscard]] char getCurrentChar() const;

    [[nodiscard]] bool almostEOF() const;

    /** @brief returns true if no more chars are left in the m_Stream */
    [[nodiscard]] bool eof() const;

    /** @brief resets the state of the m_Stream */
    void reset();

    /** @brief returns a const-ref to the source's path */
    [[nodiscard]] const std::filesystem::path& getSourcePath() const {
        return m_SourcePath;
    }

    [[nodiscard]] std::string getLineAt(std::size_t) const;
    [[nodiscard]] std::string getCurrentLine() const;

private:
    char m_CurrentChar{};
};
