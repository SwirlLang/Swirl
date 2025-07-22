#pragma once
#include <string>
#include <vector>
#include <functional>
#include <lexer/Tokens.h>
#include <managers/SourceManager.h>
#include <utils/utils.h>


class TokenStream {
    StreamState                 m_Cache;    // For caching stream state
    SourceManager&              m_Stream;

    std::filesystem::path       m_Path;     // a copy of the path for our ErrorManager friend =)

    bool m_isPreviousTokIdent = false;

    struct Filter {
        bool  is_active = false;
        bool  only_type = false;
        std::vector<Token> expected_tokens;
        std::vector<TokenType> expected_types;
    }                           m_Filter;

    static bool isKeyword(const std::string& _str);
    static bool isDigit( char chr);
    static bool isHexDigit( char chr);
    static bool isOctalDigit( char chr);
    static bool isBinaryDigit( char chr);
    static bool isIdStart( char chr);
    static bool isId( char chr);
    static bool isOpChar( char _chr);

    std::string readEscaped( char _end);
    Token readString( char del);

    template <typename Fn> requires std::invocable<Fn, char> 
        && std::same_as<std::invoke_result_t<Fn, char>, bool>
    std::string readWhile(const Fn pred) {
        std::string ret(1, m_Stream.getCurrentChar());

        while (!m_Stream.eof() && pred(m_Stream.peek()))
            ret += m_Stream.next();
        return ret;
    }
    
    template <typename PredFn, typename FilterFn> requires std::invocable<PredFn, char> 
        && std::same_as<std::invoke_result_t<PredFn, char>, bool>
        && std::invocable<FilterFn, char>
        && std::same_as<std::invoke_result_t<FilterFn, char>, bool>
    std::string readWhile(const PredFn pred, FilterFn filter) {
        std::string ret(1, m_Stream.getCurrentChar());

        while (!m_Stream.eof() && pred(m_Stream.peek()))
            if (filter(m_Stream.peek())) m_Stream.next();
            else ret += m_Stream.next();
        return ret;
    }

    Token readOperator();
    Token readNextTok();

public:
    Token CurTok;
    Token PeekTok;

    explicit TokenStream(SourceManager& src_man);

    void setReturnPoint();
    void restoreCache() const;

    /// what token *types* are expected next
    void expectTypes(std::initializer_list<TokenType>&& types);

    /// when particular tokens are expected next
    void expectTokens(std::initializer_list<Token>&& tokens);

    Token next(bool modify_cur_tk = true);
    Token peek();

    [[nodiscard]] StreamState getStreamState() const;
    [[nodiscard]] bool eof() const;
};
