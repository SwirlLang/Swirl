""" The core of the compiler """

import os
import re
import argparse
import sys
import pathlib

sys.tracebacklimit = 0

# contains the debug dir path of the current project in which exe's have to be produced
env_debug_path_to_exe = None  # always acess this var after calling the compile function

parser = argparse.ArgumentParser(
    prog="compiler", description="Compiler for lambda code"
)
file_arg = parser.add_argument(
    "file",
    type=str,
)
parsed = parser.parse_args()


class CompilerError(Exception):
    """Class for compiler's errors"""
    __module__ = "builtins"  # Removes the annoying "__main__." before the error


filename = parsed.file

source = open(filename, "r")
readed_file = source.read()
functions = []
index = readed_file.find("func")
string_indices = []
s_index1 = readed_file.find("'")
s_index2 = readed_file.find('"')
while s_index1 != -1 != s_index2:
    tmp_index = min(s_index1, s_index2)
    row = readed_file[: tmp_index].count("\n")
    col = tmp_index - readed_file[: tmp_index].rfind("\n")
    if readed_file[tmp_index - 1] == '\\' != readed_file[tmp_index - 2]:
        raise CompilerError(f"Line {row}, column {col - 1}: unexpected backslash")

    elif s_index1 < s_index2:
        singlei = readed_file.find("'", s_index2 + 1)
        if singlei == -1:
            raise CompilerError(f"Line {row}, column {col}: unfinished string")
        while readed_file[singlei - 1] == "\\" != readed_file[singlei - 2]:
            singlei = readed_file.find("'", singlei + 1)
            if singlei == -1:
                raise CompilerError(f"Line {row}, column {col}: unfinished string")
        string_indices.append((s_index1, singlei))
        singlei += 1
        s_index1 = readed_file.find("'", singlei)
        if s_index2 < singlei:
            s_index2 = readed_file.find('"', singlei)

    else:
        doublei = readed_file.find('"', s_index2 + 1)
        if doublei == -1:
            raise CompilerError(f"Line {row}, column {col}: unfinished string")
        while readed_file[doublei - 1] == "\\" != readed_file[doublei - 2]:
            doublei = readed_file.find('"', doublei + 1)
            if doublei == -1:
                raise CompilerError(f"Line {row}, column {col}: unfinished string")
        string_indices.append((s_index2, doublei))  # fun fact: this string indices list is already sorted on its own
        doublei += 1
        s_index2 = readed_file.find('"', doublei)
        if s_index1 < doublei:
            s_index1 = readed_file.find("'", doublei)

while s_index1 != -1:
    row = readed_file[: s_index1].count("\n")
    col = s_index1 - readed_file[: s_index1].rfind("\n")
    if readed_file[s_index1 - 1] == '\\' != readed_file[s_index1 - 2]:
        raise CompilerError(f"Line {row}, column {col - 1}: unexpected backslash")
    singlei = readed_file.find("'", s_index1 + 1)
    if singlei == -1:
        raise CompilerError(f"Line {row}, column {col}: unfinished string")
    while readed_file[singlei - 1] == "\\" != readed_file[singlei - 2]:
        singlei = readed_file.find("'", singlei + 1)
        if singlei == -1:
            raise CompilerError(f"Line {row}, column {col}: unfinished string")
    string_indices.append((s_index1, singlei))
    s_index1 = readed_file.find("'", singlei + 1)

while s_index2 != -1:
    row = readed_file[: s_index2].count("\n")
    col = s_index2 - readed_file[: s_index2].rfind("\n")
    if readed_file[s_index2 - 1] == '\\' != readed_file[]


while index != -1:
    row = readed_file[:index].count("\n") + 1
    col = index - readed_file[:index].rfind("\n")
    fi2 = readed_file.find("endfunc", index - 3)
    if index == fi2 + 3:
        raise CompilerError(
            f"Line {row}, column {col - 3}: missing func to corresponding endfunc"
        )

    elif fi2 == -1:
        raise CompilerError(
            f"Line {row}, column {col}: unfinished function declaration"
        )

    tmp = readed_file[index + 4 : fi2]
    tmp = tmp.replace('\\"', "")
    tmp = tmp.replace("\\'", "")
    string_index = tmp.find('"')
    si2 = tmp.find('"', string_index + 1)
    if si2 == -1:
        raise CompilerError(f"Line {row}, column {col} unfinished string inside this fun")

    if "func " in tmp:
        raise CompilerError(f'Line {row}, column {col}: cannot declare a function inside another function, eight "bruh"s for you')

    if "class" in tmp:
        raise CompilerError(f"Line {row}, column {col}: cannot declare a class inside a function")

    nextfunc = readed_file[index + 4 : fi2].replace(" ", "")
    nextfunc = nextfunc[: nextfunc.find(":")]
    if nextfunc == "main":
        raise CompilerError(f"Line {row}, column {col}: cannot call a function \"main\" because the main function is placed in the global scope")
    
    for func in functions:
        func = func[0].replace(" ", "")
        if nextfunc == func[: func.find(":")]:
            raise CompilerError(
                f"Line {row}, column {col}: declaration of duplicate function"
            )
    functions.append((readed_file[index + 4 : fi2], row))
    index = readed_file.find("func", fi2 + 7)

# for func in functions:
#     main_func = main_func.replace(func, "")

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
 
 
class RePattern:
    """ Class defining patterns for LambdaCode (later used for pattern matching)"""

    int_regex = "int \\w* = \\d"
    float_regex = "(float \\w* = \\w*\\.\\w*)|(float \\w* = \\w*)"
    string_regex = "string\[\d*\].*"
    array_regex = (
        "(array \\w*\\[\\w*,\\d\\] = \\d.*)|(array \\w*\\[\\w*,\\d\\] = \\(\\d.*)"
    )
    class_regex = r"[a-z A-Z] [inherits]? [a-z A-Z]()\w.endclass{1}"  #TODO unstable, needs work 


print(re.findall(RePattern.int_regex, example_code))
print(
    [
        fnumber
        for result in re.findall(RePattern.float_regex, example_code)
        for fnumber in result
        if fnumber != ""
    ]
)
# this is just for printing stuff and we'll remove them anyway okay :thumbsup:
print(re.findall(RePattern.string_regex, example_code))
print(
    [
        ar
        for result in re.findall(RePattern.array_regex, example_code)
        for ar in result
        if ar != ""
    ]
)

# def precompile(snippet: str) -> str:
#     """ Precompiles the code before translating it to CPP (e.g. removing comments and pasting imported stuffs) """
#    # we can only ignore comments, not remove them, because we need the number of c


def _compile(data: str) -> str:
    """ Compiles the snippet passed into the data param into C++ """
    _home_dir = pathlib.Path.home()
    try:
        os.mkdir(f"{_home_dir}/debug")
        os.mkdir(f"{_home_dir}/debug/{(parsed.file.split('.'))[0]}")
        debug_dir = f"{_home_dir}/debug/{(parsed.file.split('.'))[0]}"
        env_debug_path_to_exe = debug_dir
    except Exception: 
        pass
    return ''


_compile("ello ello")


def debug():
    """ Only meant for debugging purposes """
    os.system(f"cd {pathlib.Path.home()}/Lambda-Code/core/")
    os.system("python3 compiler.py C:/Users/Ashok Kumar/Lambda-Code/core/print_statement.lc")


debug()
