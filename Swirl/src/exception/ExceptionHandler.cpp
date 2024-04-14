#include <format>
#include <exception/ExceptionHandler.h>

extern std::string SW_FED_FILE_PATH;


void ExceptionHandler::newException(EXCEPTION_TYPE type, std::size_t line, std::size_t from, std::size_t to,
                                    const std::string& line_str, const std::string& msg) {
    if (type == ERROR) {
        m_HasErrors = true;

        if (!m_Errors.empty())
            m_Errors.append(EXCEPTION_STR_LINE);

        std::string err_lines = line_str + "\n";
        err_lines.append(std::string(from, '~') + std::string(to - from, '^'));

        m_Errors.append(
                std::format("[ERROR]: In file {}\nLine {}, Column {}\n{}\n{}",
                            SW_FED_FILE_PATH,
                            line + 1,
                            from,
                            err_lines,
                            msg));
        NumberOfErrors++;
    }

    // type == WARNING
    else {
        if (!m_Warnings.empty())
            m_Warnings.append(EXCEPTION_STR_LINE);

        std::string w_lines = line_str + "\n";
        w_lines.append(std::string(from, '~') + std::string(to - from, '^'));

        m_Warnings.append(
                std::format("[WARNING]: In file {}\nLine {}, Column {}\n{}\n{}",
                            SW_FED_FILE_PATH,
                            line,
                            from,
                            w_lines,
                            msg));

    }
}

void ExceptionHandler::raiseAll() {
    if (m_HasErrors) {
        std::cerr << m_Errors << std::endl;
        std::cerr << "\n" << NumberOfErrors << " error(s) in total detected" << std::endl;
        std::exit(1);
    } else {

    }
}

