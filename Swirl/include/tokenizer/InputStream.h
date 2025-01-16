#pragma once
#include <unordered_map>

class InputStream {
    std::tuple<size_t, size_t, size_t> cache;
    std::string m_CurrentLine;
    std::string m_Source;

    struct m_LineMeta_t { std::size_t from{}, line_size{}; };
    std::unordered_map<std::size_t, m_LineMeta_t> m_LineTable;


public:
    std::size_t Pos = 0, Line = 1, Col = 0;
    std::unordered_map<std::size_t, std::string> LineMap{};

    explicit InputStream(std::string);

    /** @brief Returns the next value without discarding it */
    char peek() const;

    /** @brief returns the next value and discards it */
    char next();

    char getCurrentChar() const;

    /** @brief returns true if no more chars are left in the m_Stream */
    bool eof() const;

    /** @brief saves the current state */
    void setReturnPoint();

    /** @brief restores cache */
    void restoreCache();

    /** @brief resets the state of the m_Stream */
    void reset();

    std::string getLineAt(std::size_t);
    std::string getCurrentLine() const;

private:
    char m_CurrentChar{};
};
