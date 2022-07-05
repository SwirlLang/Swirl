#include <iostream>
#include <vector>
#include <sstream>

class ExceptionHandler {
public:
    static void raise(const char *_message, const char *_source, const char *_feeded_file_path,
                      std::size_t _line, std::size_t _col_range[2], int _exit_code = 1) {

        std::istringstream source_buf(_source);
        std::size_t ln_count = 0;
        std::string err_ln;
        std::string bn_str;

        for (std::string src_current_ln; std::getline(source_buf, src_current_ln);) {
            ln_count++;
            if (ln_count == _line) {
                std::size_t col_substr_size = src_current_ln.substr(_col_range[0], _col_range[1] - _col_range[0]).length();
                std::string tilden_str = "";
                for (std::size_t t_i = 0; t_i < _col_range[0]; t_i++)
                    tilden_str += " ";
                for (std::size_t t_i_ = 0; t_i_ < col_substr_size; t_i_++)
                    tilden_str += "^";

                err_ln = src_current_ln;
                bn_str = tilden_str;
            }
        }

        std::cout << "Error in file " << _feeded_file_path << "\n"
                  << "at line: " << _line << ", column: " << _col_range[0] << "\n\n"
                  << err_ln << "\n"
                  << bn_str << "\n"
                  << "Error: " << _message << std::endl;
        exit(_exit_code);
    }
};