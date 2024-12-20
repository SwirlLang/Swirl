#pragma once
#include <print>
#include <thread>
#include <tokenizer/Tokenizer.h>

extern std::thread::id MAIN_THREAD_ID;

class ErrorManager {
    TokenStream& m_TokStream;
    std::string m_Message;
    bool m_IsErroneous;

public:
    explicit ErrorManager(TokenStream& token_stream): m_TokStream(token_stream), m_IsErroneous(false) {}

    void newError(const std::string& message, std::optional<StreamState> at = std::nullopt) {
        m_IsErroneous = true;
        if (!at.has_value()) at = m_TokStream.getStreamState();
        m_Message += LineTable[at->Line] + std::string(at->Col - 1, ' ') + "^ " + message + LINE;
    }

    void newWarning(const std::string& message, std::optional<StreamState> at = std::nullopt) {
        m_IsErroneous = true;
        if (!at.has_value()) at = m_TokStream.getStreamState();
        m_Message += LineTable[at->Line] + std::string(at->Col - 1, ' ') + "^ " + message + LINE;
    }

    void newSafetyError(const std::string& message, std::optional<StreamState> at = std::nullopt) {
        m_IsErroneous = true;
        if (!at.has_value()) at = m_TokStream.getStreamState();
        m_Message += LineTable[at->Line] + std::string(at->Col - 1, ' ') + "^ " + message + LINE;
    }


    void raiseAll() const {
        std::println(stderr, "{}", m_Message);
        if (std::this_thread::get_id() != MAIN_THREAD_ID) {
            std::exit(1);
        }
    }

    [[nodiscard]] bool hasErrors() const {
        return m_IsErroneous;
    }

private:
    static constexpr char LINE[] = "\n----------------------------------------\n\n";
};
