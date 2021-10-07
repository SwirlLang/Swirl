""" The core of the compiler """

import os
import re
import argparse
import sys
import pathlib

sys.tracebacklimit = 0  # Removes the annoying traceback text

# contains the debug dir path of the current project in which exe's have to be produced
env_debug_path_to_exe = None  # always access this var after calling the compile function

parser = argparse.ArgumentParser(
    prog="LCC", description="Compiler for lambda code"
)
file_arg = parser.add_argument(
    "file",
    type=str,
)
parsed = parser.parse_args()


class Error(Exception):
    """ Class for compiler's error """
    __module__ = "builtins"  # Removes the annoying "__main__." before errors


filename = parsed.file

source = open(filename, "r")
readed_file = source.read()
size = len(readed_file)
if not size:
    raise Error("empty file")
translation = True
valid = True
functions = []
func_indices = []
ifunc = 0
index = readed_file.find("func")
string_indices = []
s_index1 = readed_file.find("'")
s_index2 = readed_file.find('"')
bscount = 0
comment_indices = []
c_index1 = readed_file.find("//")
c_index2 = readed_file.find("///")
row = 0
tmp_index = 0
while (s_index1 + s_index2 + c_index1 + c_index2) != -4:
    min_index = min([x for x in [s_index1, s_index2, c_index1, c_index2] if x != -1])
    row += readed_file[tmp_index : min_index].count("\n")
    col = min_index - readed_file[: min_index].rfind("\n")
    tmp_index = min_index
    if min_index == c_index1:
        if c_index1 == c_index2:
            multi_c = readed_file.find("///", c_index2 + 3)
            if multi_c == -1:
                raise Error(f"Line {row}, column {col}: unfinished multi-line comment")
            multi_c += 3
            comment_indices.append(range(c_index2, multi_c))
            c_index1 = readed_file.find("//", multi_c)
            c_index2 = readed_file.find("///", multi_c)
            if -1 != s_index1 < multi_c:
                s_index1 = readed_file.find("'", multi_c)
            if -1 != s_index2 < multi_c:
                s_index2 = readed_file.find('"', multi_c)
        else:
            single_c = readed_file.find("\n", c_index1 + 2)
            if single_c == -1:
                comment_indices.append(range(c_index1, size))
                break
            single_c += 2
            comment_indices.append(range(c_index1, single_c))
            c_index1 = readed_file.find("//", single_c)
            if -1 != c_index2 < single_c:
                c_index2 = readed_file.find("///", single_c)
            if -1 != s_index1 < single_c:
                s_index1 = readed_file.find("'", single_c)
            if -1 != s_index2 < single_c:
                s_index2 = readed_file.find('"', single_c)
    elif min_index == s_index1:
        if readed_file[s_index1 - 1] == '\\':
            translation = False
            print(f"Error: Line {row}, column {col - 1}: unexpected backslash")
        singlei = readed_file.find("'", s_index1 + 1)
        if singlei == -1:
            raise Error(f"Line {row}, column {col}: unfinished string")
        while readed_file[singlei - bscount - 1] == '\\':
            bscount += 1
        while bscount & 1:
            bscount = 0
            singlei = readed_file.find("'", singlei + 1)
            if singlei == -1:
                raise Error(f"Line {row}, column {col}: unfinished string")
            while readed_file[singlei - bscount - 1] == '\\':
                bscount += 1
        bscount = 0
        singlei += 1
        string_indices.append(range(s_index1, singlei))
        s_index1 = readed_file.find("'", singlei)
        if -1 != s_index2 < singlei:
            s_index2 = readed_file.find('"', singlei)
        if -1 != c_index1 < singlei:
            c_index1 = readed_file.find("//", singlei)
        if -1 != c_index2 < singlei:
            c_index2 = readed_file.find("///", singlei) 
    else:
        if readed_file[s_index2 - 1] == "\\":
            translation == False
            print(f"Error: Line {row}, column {col - 1}: unexpected backslash")
        doublei = readed_file.find('"', s_index2 + 1)
        if doublei == -1:
            raise Error(f"Line {row}, column {col}: unfinished string")
        while readed_file[doublei - bscount - 1] == '\\':
            bscount += 1
        while bscount & 1:
            bscount = 0
            doublei = readed_file.find('"', doublei + 1)
            if doublei == -1:
                raise Error(f"Line {row}, column {col}: unfinished string")
            while readed_file[doublei - bscount - 1] == '\\':
                bscount += 1
        bscount = 0            
        doublei += 1
        string_indices.append(range(s_index2, doublei))  # fun fact: this string indices list is already sorted on its own
        s_index2 = readed_file.find('"', doublei)
        if -1 != s_index1 < doublei:
            s_index1 = readed_file.find("'", doublei)
        if -1 != c_index1 < doublei:
            c_index1 = readed_file.find("//", doublei)
            if readed_file[c_index1 + 2] == "/":
                c_index2 = c_index1
        if -1 != c_index2 < doublei:
            c_index2 = readed_file.find("///", doublei)
            
            
            
            
        
while index != -1:
    while True:
        for sindex in string_indices:
            if index in sindex:
                index = readed_file.find("func", sindex[-1] + 1)
                continue
        else:
            break

    if index == -1:
        break

    row = readed_file[:index].count("\n") + 1
    col = index - readed_file[:index].rfind("\n")
    fi2 = readed_file.find("endfunc", index - 3)
    if fi2 == -1:
        translation = False
        raise Error(f"Line {row}, column {col}: unfinished function declaration")

    elif index == fi2 + 3:
        translation = False
        raise Error(f"Line {row}, column {col - 3}: missing func to corresponding endfunc")

    else:
        while True:
            for sindex in string_indices:
                if fi2 in sindex:
                    fi2 = readed_file.find("endfunc", sindex[-1] + 1)
                    continue
            else:
                break
    if fi2 == -1:
        translation = False
        raise Error(f"Line {row}, column {col}: unfinished function declaration")

    index += 4

    rType = readed_file.find(":", index, fi2)
    if rType == -1:
        translation = False
        index = readed_file.find("func", fi2 + 7)
        print(f"Error: Line {row}, column {col}: missing function return type")
        continue
    while True:
        for sindex in string_indices:
            if rType in sindex:
                rType = readed_file.find(":", sindex[-1] + 1, fi2)
                break
        else:
            break
    if rType == -1:
        translation = False
        index = readed_file.find("func", fi2 + 7)
        print(f"Error: Line {row}, column {col}: missing function return type")
        continue

    parbracket = readed_file.find("(", rType, fi2)
    if parbracket == -1:
        translation = False
        index = readed_file.find("func", fi2 + 7)
        print(f"Error: Line {row}, column {col}: missing parameters syntax in this function")
        continue
    for sindex in string_indices:
        if parbracket in sindex:
            parbracket = -1
            break
    if parbracket == -1:
        translation = False
        index = readed_file.find("func", fi2 + 7)
        print(f"Error: Line {row}, column {col}: missing parameters syntax in this function")
        continue

    errIndex1 = readed_file.find("func", index, fi2)
    rIndex1 = index
    row1 = row
    errIndex2 = readed_file.find("class", index, fi2)
    rIndex2 = index
    row2 = row
    while errIndex1 != -1:
        for sindex in string_indices:
            if errIndex1 in sindex:
                errIndex1 = readed_file.find("func", sindex[-1] + 1, fi2)
                break
        else:
            row1 += readed_file[rIndex1: errIndex1].count("\n")
            col = errIndex1 - readed_file[: errIndex1].rfind("\n")
            rIndex1 = errIndex1
            errIndex1 = readed_file.find("func", errIndex1 + 4, fi2)
            translation = False
            valid = False
            print(
                f'Error: Line {row1}, column {col}: cannot declare a function inside another function, eight "bruh"s for you')
    while errIndex2 != -1:
        for sindex in string_indices:
            if errIndex2 in sindex:
                errIndex2 = readed_file.find("class", errIndex2 + 5, fi2)
                break
        else:
            row2 += readed_file[rIndex2: errIndex2].count("\n")
            col = errIndex2 - readed_file[: errIndex2].rfind("\n")
            rIndex2 = errIndex2
            errIndex2 = readed_file.find("class", errIndex2 + 5, fi2)
            translation = False
            valid = False
            print(f"Error: Line {row2}, column {col}: cannot declare a class inside a function")

    nextfunc = readed_file[rType + 1: parbracket].replace(" ", "")
    if nextfunc == "main":
        translation = False
        print(
            f"Error: Line {row}, column {col}: cannot call a function \"main\" because the main function is placed in the global scope")
        break

    for func in functions:
        func = func[0].replace(" ", "")
        if nextfunc == func[: func.find(":")]:
            translation = False
            valid = False
            print(f"Error: Line {row}, column {col}: declaration of duplicate function")
    if valid:
        functions.append((readed_file[index: fi2], row))
        func_indices.append(range(index - 4, fi2 + 8))
        index = readed_file.find("func", fi2 + 7)
    valid = True

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
    class_regex = r"[a-z A-Z] [inherits]? [a-z A-Z]()\w.endclass{1}"  # TODO unstable, needs work


""" print(re.findall(RePattern.int_regex, example_code))
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
) """


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


def class_parser(snippet: str) -> dict:
    """
     :param snippet: the piece of code to be parsed
    """
    _types = ["string", "int", "float", "array"]
    parsed_dict = {
        "name": "Class", "params": [], "super_class": None,
    }

    name = snippet.split()

    try:
        _inheritance = ((" ".join((snippet.split("inherits")[1]).split())).split("(")[0]).split()
        _inheritance.remove("and")
    except IndexError:
        pass

    _params = (snippet.split("(")[1]).split(")")[0]
    __names = " ".join((" ".join(_params.split(','))).split(":")).split()
    __types = " ".join((" ".join(_params.split(','))).split(":")).split()
    for item in __types:
        """ Iterates and removes every item except the data types """
        if not item in _types:
            __types.remove(item)
    for _item in __names:
        """ Iterates over __names and removes data types """
        if _item in _types:
            __names.remove(_item)

    parsed_dict["name"] = name[1]
    parsed_dict["super_class"] = _inheritance
    parsed_dict["params"] = [__names, __types]
    return parsed_dict


class ConventionError(Exception):
    """ Exception raised when basic conventions are not followed appropriately, eg: When function name is starting
    with an invalid character """
    __module__ = "builtins"


def func_parser(snippet: str) -> list:
    """
    A basic function parser (might be unstable)
    :param snippet: The snippet to parse(str)
    """

    # TODO test the stability of the parser

    __main_dict__ = []
    _funcs = snippet.split("func ")
    _types = ["string", "int", "float", "array"]
    _param_name = []
    _param_types = []
    for item in _funcs:
        if item == "\n":
            _funcs.remove(item)
    for function in _funcs:
        helper = (
            (
                "".join(" ".join(function.split(function.split('(')[0])).split(")")
                        )
            ).split("\n")[0]).split("(")
        _helper = "".join("".join(("".join(helper[1])).split(":")).split(",")).split()
        for paramName in _helper:
            if paramName in _types:
                _helper.remove(paramName)
        _param_name = _helper
        _typeHelper = "".join("".join(("".join(helper[1])).split(":")).split(",")).split()
        for paramType in _typeHelper:
            if paramType in _param_name:
                _typeHelper.remove(paramType)
        _param_types = _typeHelper

        __main_dict__.append(
            {
                "name": function.split('(')[0],
                "params": [_param_name, _param_types]
            }
        )

    return __main_dict__
