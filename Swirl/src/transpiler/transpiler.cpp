#include <iostream>
#include <exception>
#include <memory>
#include <map>
#include <fstream>

#include <parser/parser.h>
#include <definitions/definitions.h>

#define _debug true

void Transpile(AbstractSyntaxTree& _ast, const std::string& _buildFile) {

//    defs definitions{};
//    try {
//        while (!_tokenStream.eof()) {
//            auto tok = _tokenStream.next();
//            const char* type = tok[0];
//            const char* v_str = tok[1];
//
//            std::cout << "TYPE: " << type << "\t" << "VAL: " << v_str << "\n";
//
//        }
//    } catch ( std::exception& e ) {}
    // TODO: write directly to the file
    std::string compiled_source = "#include <iostream>\n"
                                  "\n"
                                  "template < typename Const >\n"
                                  "void print(Const __Obj, const std::string& __End = \"\\n\", bool __Flush = true) {\n"
                                  "    if (__Flush) std::cout << __Obj << __End << std::flush;\n"
                                  "    else std::cout << __Obj << __End;\n"
                                  "}\n"
                                  "\n"
                                  "int main() {\n";
//    std::ifstream bt_buf;
//    bt_buf.open("Swirl/src/cppranspiler/builtins.cpp");
//    if (bt_buf.is_open()) {
//        std::string bt_cr_ln;
//        while (std::getline(bt_buf, bt_cr_ln))
//            compiled_source += bt_cr_ln + "\n";
//    }

    for (auto const& child : _ast.chl) {
        if (child.type == "CALL") {
            compiled_source += child.ident + "(";
            int args_count = 0;
            for (const auto& arg : child.args) {
                if (arg.type == "STRING")
                    compiled_source += "\"" + arg.value + "\"";
                else
                    compiled_source += arg.value;
                args_count++;
                if (args_count != child.args.size()) compiled_source += ",";
            }
            compiled_source += ");";
            compiled_source += "\n";
        }
    }

    std::ofstream o_file_buf(_buildFile);
    compiled_source += "\n}";
    o_file_buf << compiled_source;
}