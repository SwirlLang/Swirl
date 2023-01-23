#include <optional>
#include <parser/parser.h>

#ifndef SWIRL_TRANSPILER_H
#define SWIRL_TRANSPILER_H

extern std::string compiled_source;
std::optional<std::unordered_map<std::string, std::string>> Transpile( std::vector<Node>&,
                const std::string&,
                std::string& _dest = compiled_source,
                bool onlyAppend = false,
                bool returnSymbolTable = false );

#endif
