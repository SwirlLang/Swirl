#pragma once
#include <print>
#include <thread>
#include <tokenizer/TokenStream.h>

extern std::thread::id MAIN_THREAD_ID;


class ErrorManager {
    TokenStream* m_TokStream;
    std::string m_Message;
    int m_Errors = 0;  // the no. of errors

public:
    explicit ErrorManager(TokenStream* token_stream): m_TokStream(token_stream) {
        std::println("Error Manager registered with path: {}", m_TokStream->m_Path.string());
    }

    void raiseUnexpectedEOF() {
        m_Errors++;
        m_Message += "In " + m_TokStream->m_Path.string() + '\n' + "Error: unexpected EOF (end of file)!\n";
        raiseAll();
    }

    void newSyntaxError(const std::string& message, std::optional<StreamState> at = std::nullopt) {
        m_Errors += 1;
        if (!at.has_value()) at = m_TokStream->getStreamState();

        m_Message += "In " + m_TokStream->m_Path.string() + ':' + std::to_string(at->Line) + ":" + std::to_string(at->Col) + ':' + '\n' +
            m_TokStream->m_Stream.getLineAt(at->Line)
        + '\n' + std::string(at->Col - 1, ' ')
        + "^ Syntax error! " + message
        + LINE;
    }

    void newError(const std::string& message, std::optional<StreamState> at = std::nullopt) {
        m_Errors += 1;
        if (!at.has_value()) at = m_TokStream->getStreamState();
        m_Message += "In " + m_TokStream->m_Path.string() + ':' + std::to_string(at->Line) + ":" + std::to_string(at->Col) + ':' + '\n' +
            m_TokStream->m_Stream.getLineAt(at->Line) + '\n' + std::string(at->Col - 1, ' ') + "^ " + message + LINE;
    }

    void newWarning(const std::string& message, std::optional<StreamState> at = std::nullopt) {
        m_Errors += 1;
        if (!at.has_value()) at = m_TokStream->getStreamState();
        m_Message += "In " + m_TokStream->m_Path.string() + ':' + std::to_string(at->Line) + ":" + std::to_string(at->Col) + ':' + '\n' +
            m_TokStream->m_Stream.getLineAt(at->Line) + std::string(at->Col - 1, ' ') + "^ " + message + LINE;
    }


    void newSafetyError(const std::string& message, std::optional<StreamState> at = std::nullopt) {
        m_Errors += 1;
        if (!at.has_value()) at = m_TokStream->getStreamState();
        m_Message += m_TokStream->m_Stream.getLineAt(at->Line) + std::string(at->Col - 1, ' ') + "^ " + message + LINE;
    }

    void raiseAll() {
        if (m_Errors) {
            std::println(stderr, "{}\n{} error(s) in total reported", m_Message, m_Errors);
            if (std::this_thread::get_id() == MAIN_THREAD_ID) {
                std::exit(1);
            }
        }
    }

    [[nodiscard]] bool hasErrors() const {
        return m_Errors > 0;
    }

private:
    static constexpr char LINE[] = "\n\n";
};
