#include <iostream>
#include <exception>
#include <memory>
#include <map>
#include <fstream>

#include <parser/parser.h>
#include <definitions/definitions.h>

#define _debug true


void Transpile(AbstractSyntaxTree& _ast, const std::string& _buildFile) {
    bool                       is_scp;
    std::ifstream              bt_fstream;
    std::string                tmp_str_cnst;
    std::size_t                last_scp_order;
    std::string                compiled_source;
    std::array<const char*, 3> vld_scopes = {"CONDITION", "FUNC", "CLASS"};

    bt_fstream.open("Swirl/src/transpiler/builtins.cpp");
    if (bt_fstream.is_open()) {
        std::string bt_cr_ln;
        while (std::getline(bt_fstream, bt_cr_ln))
            compiled_source += bt_cr_ln + "\n";
    }

    compiled_source += "int main() {\n";

    for (auto const& child : _ast.chl) {
        if (child.type == "BR_OPEN") {
            compiled_source += "{";
            continue;
        }
        if (child.type == "BR_CLOSE") {
            compiled_source += "}";
            continue;
        }

        if (child.type == "CONDITION") {
            tmp_str_cnst += "if (" + child.condition + ")";
            compiled_source += tmp_str_cnst;
            tmp_str_cnst.clear();
            continue;
        }

        else if (child.type == "CALL") {
            tmp_str_cnst += child.ident + "(";
            int args_count = 0;

            for (const auto& arg : child.args) {
                if (arg.type == "STRING")
                    tmp_str_cnst += "\"" + arg.value + "\"";
                else
                    tmp_str_cnst += arg.value;

                args_count++;
                if (args_count != child.args.size()) tmp_str_cnst += ",";
            }

            tmp_str_cnst += ")";
            compiled_source += tmp_str_cnst + ";";
            tmp_str_cnst.clear();
        }
    }

    std::ofstream o_file_buf(_buildFile);
    compiled_source += "}";
    o_file_buf << compiled_source;
}