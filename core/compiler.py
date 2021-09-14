""" The core of the compiler """

import re
import argparse

parser = argparse.ArgumentParser(
    prog="compiler", description="Compiler for lambda code"
)
file_arg = parser.add_argument("file", type=str,)
parsed = parser.parse_args()

class CompilerError(Exception):
    pass

filename = ""

filename = parsed.file
print(filename)

source = open(filename, "r")
readed_file = source.read()
functions = []
index = readed_file.find("func")
comment_index = readed_file.find("///")
gvariables = {}

while comment_index != -1:
    ci2 = readed_file.find("///", comment_index + 3)
    if ci2 == -1:
        row = readed_file[:comment_index].count("\n") + 1
        col = 1 + comment_index - readed_file[:comment_index].rfind("\n")                     
        raise CompilerError(f"{row}:{col}: error: unfinished multi-line comment")
    readed_file = readed_file.replace(readed_file[comment_index : ci2 + 3], "")

while index != -1:
    if readed_file.find("func", index + 4) == -1:
        pass #gonna make this later
    functions.append()
    index = readed_file.find("func", index)

main_func = readed_file

for func in functions:
    main_func = main_func.replace(func, "")

print(functions)
source.close()

example_code = """
int abc = 9
int b = 10
float c = 0.9
float c = 9
string[99] name = "mrinmoy"
array arr[int,5] = (1,2,2,3,3,4)
array arr[int,5] = 1, 2, 2, 3 , 3,4
// hello world this is a comment
"""


class re_patterns:
    int_regex = "int \\w* = \\d"
    float_regex = "(float \\w* = \\w*\\.\\w*)|(float \\w* = \\w*)"
    string_regex = "string\[\d*\].*"
    array_regex = (
        "(array \\w*\\[\\w*,\\d\\] = \\d.*)|(array \\w*\\[\\w*,\\d\\] = \\(\\d.*)"
    )


print(re.findall(re_patterns.int_regex, example_code))
print(
    [
        fnumber
        for result in re.findall(re_patterns.float_regex, example_code)
        for fnumber in result
        if fnumber != ""
    ]
)
print(re.findall(re_patterns.string_regex, example_code))
print(
    [
        ar
        for result in re.findall(re_patterns.array_regex, example_code)
        for ar in result
        if ar != ""
    ]
)
