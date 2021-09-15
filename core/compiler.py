""" The core of the compiler """

import re
import argparse

parser = argparse.ArgumentParser(
    prog="compiler", description="Compiler for lambda code"
)
file_arg = parser.add_argument(
    "file",
    type=str,
)
parsed = parser.parse_args()


class CompilerError(Exception):
    pass


filename = ""

filename = parsed.file

source = open(filename, "r")
readed_file = source.read()
functions = []
index = readed_file.find("func")
fi2 = readed_file.find("endfunc")
comment_index = readed_file.find("///")
gvariables = {}

while comment_index != -1:
    ci2 = readed_file.find("///", comment_index + 3)
    if ci2 == -1:
        row = readed_file[:comment_index].count("\n") + 1
        col = comment_index + 1 - (readed_file[:comment_index].rfind("\n") + 1)
        raise CompilerError(f"{row}:{col}: error: unfinished multi-line comment")
    readed_file = readed_file.replace(readed_file[comment_index : ci2 + 3], "")

while index != -1:
    row = readed_file[:index].count("\n") + 1
    col = index + 1 - (readed_file[:index].rfind("\n") + 1)
    fi2 = readed_file.find("endfunc", index)
    if index == fi2 - 3:
        raise CompilerError(f"{row}:{col}: missing func to corresponding endfunc")

    elif fi2 == -1:
        raise CompilerError(f"{row}:{col}: unfinished function declaration")
	
    tmp = readed_file[index + 4 : fi2]
    tmp = tmp.replace("\\\"", "")
    tmp = tmp.replace("\\'", "")
    string_index = tmp.find("\"")
    si2 = tmp.find("\"", string_index + 1)
    if si2 == -1:
        raise CompilerError(f"{row}:{col} unfinished string inside this function")
    if "func" in tmp or "class" in tmp:
        raise CompilerError(
            f"{row}:{col}: cannot declare a class or function inside another function, bruh"
        )

    functions.append(readed_file[index + 4 : fi2])
    index = readed_file.find("func", fi2 + 7)

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
