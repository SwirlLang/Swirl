#include <format>
#include <exception/ExceptionHandler.h>

extern std::string SW_COMPLETE_FED_FILE_PATH;
extern std::unordered_map<std::size_t, std::string> LineTable;


void ExceptionHandler::newException(EXCEPTION_TYPE type, std::size_t line, std::size_t from, std::size_t to,
                                    const std::string& msg) {
    if (type == ERROR) {
        m_HasErrors = true;

        if (!m_Errors.empty())
            m_Errors.append(EXCEPTION_STR_LINE);

        std::string err_lines = LineTable[line] + "\n";
        err_lines.append(std::string(from, ' ') + std::string(to - from, '^'));

        m_Errors.append(
                std::format("File '{}'; line {}, col {}\n{}\n{}",
                            SW_COMPLETE_FED_FILE_PATH,
                            line,
                            from,
                            err_lines,
                            "Error: " + msg));
        NumberOfErrors++;
    }


    // type == ERROR
    // type == WARNING
    else {
        if (!m_Warnings.empty())
            m_Warnings.append(EXCEPTION_STR_LINE);

        std::string w_lines = LineTable[line] + "\n";
        w_lines.append(std::string(from, '~') + std::string(to - from, '^'));

        m_Warnings.append(
                std::format("[WARNING]: In file {}\nLine {}, Column {}\n{}\n{}",
                            SW_COMPLETE_FED_FILE_PATH,
                            line,
                            from,
                            w_lines,
                            msg));

    }
}

void ExceptionHandler::raiseAll() const {
    if (m_HasErrors) {
        std::cerr << m_Errors << std::endl;
        std::cerr << "\n" << NumberOfErrors << " error(s) in total detected." << std::endl;
        std::exit(1);
    }
    // TODO: Warnings
}

