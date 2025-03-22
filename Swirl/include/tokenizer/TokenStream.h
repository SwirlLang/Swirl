#pragma once
#include <string>
#include <vector>
#include <functional>
#include <tokenizer/Tokens.h>
#include <tokenizer/InputStream.h>
#include <utils/utils.h>

class ErrorManager;

class TokenStream {
    StreamState                 m_Cache;    // For caching stream state
    InputStream                 m_Stream;   // InputStream instance

    std::filesystem::path       m_Path;     // a copy of the path for our ErrorManager friend =)

    friend class ErrorManager;

    struct Filter {
        bool  is_active = false;
        bool  only_type = false;
        std::vector<Token> expected_tokens;
        std::vector<TokenType> expected_types;
    }                           m_Filter;

    static bool isKeyword(const std::string& _str);
    static bool isDigit( char chr);
    static bool isNumeric( char chr);
    static bool isIdStart( char chr);
    static bool isId( char chr);
    static bool isOpChar( char _chr);
    static bool isWhiteSpace( char _chr);

    std::string readEscaped( char _end);
    Token readString( char del);
    std::string readWhile(const std::function<bool(char)>& pred);
    Token readOperator();
    Token readNextTok();

public:
    Token CurTok;
    Token PeekTok;
    ErrorManager*  ErrMan = nullptr;

    explicit TokenStream(const fs::path& file_path);

    void setReturnPoint();
    void restoreCache();

    /// what token *types* are expected next
    void expectTypes(std::initializer_list<TokenType>&& types);

    /// when particular tokens are expected next
    void expectTokens(std::initializer_list<Token>&& tokens);

    Token next(bool modify_cur_tk = true);
    Token peek();

    StreamState getStreamState() const;
    bool eof() const;
};
