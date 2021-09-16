""" The core of the compiler """

import re
import argparse
import sys

sys.tracebacklimit = 0

parser = argparse.ArgumentParser(
    prog="compiler", description="Compiler for lambda code"
)
file_arg = parser.add_argument(
    "file",
    type=str,
)
parsed = parser.parse_args()


class CompilerError(Exception):
    __module__ = "builtins"


filename = parsed.file

source = open(filename, "r")
readed_file = source.read()
functions = []
index = readed_file.find("func")
fi2 = readed_file.find("endfunc")
comment_index = readed_file.find("///")
gvariables = {}

while index != -1:
    row = readed_file[:index].count("\n") + 1
    col = index + 1 - (readed_file[:index].rfind("\n") + 1)
    fi2 = readed_file.find("endfunc", index)
    if index == fi2 + 3:
        raise CompilerError(
            f"Line {row}, column {col}: missing func to corresponding endfunc"
        )

    elif fi2 == -1:
        print(index)
        print(fi2)
        raise CompilerError(
            f"Line {row}, column {col}: unfinished function declaration"
        )

    tmp = readed_file[index + 4 : fi2]
    tmp = tmp.replace('\\"', "")
    tmp = tmp.replace("\\'", "")
    string_index = tmp.find('"')
    si2 = tmp.find('"', string_index + 1)
    if si2 == -1:
        raise CompilerError(
            f"Line {row}, column {col} unfinished string inside this function"
        )
    if "func" in tmp or "class" in tmp:
        raise CompilerError(
            f'Line {row}, column {col}: Declaration Error: cannot declare a class or function inside another function, eight "bruh"s for you'
        )

    nextfunc = readed_file[index + 4 : fi2].replace(" ", "")
    for func in functions:
        func = func.replace(" ", "")
        if nextfunc[: nextfunc.find(":")] == func[: func.find(":")]:
            raise CompilerError(
                f"Line {row}, column {col}: declaration of duplicate function"
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
