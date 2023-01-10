#include <parser/parser.h>

#ifndef SWIRL_TRANSPILER_H
#define SWIRL_TRANSPILER_H

extern std::string compiled_source;
void Transpile(std::vector<Node>&, const std::string&, std::string& _dest = compiled_source, bool onlyAppend = false);

#endif
