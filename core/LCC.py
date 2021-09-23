""" The core of the compiler """

import os
import re
import argparse
import sys
import pathlib

sys.tracebacklimit = 0 #  Removes the annoying traceback text

# contains the debug dir path of the current project in which exe's have to be produced
env_debug_path_to_exe = None  # always acess this var after calling the compile function

parser = argparse.ArgumentParser(
    prog="LCC", description="Compiler for lambda code"
)
file_arg = parser.add_argument(
    "file",
    type=str,
)
parsed = parser.parse_args()


class Error(Exception):
    """Class for compiler's errors"""
    __module__ = "builtins"  #  Removes the annoying "__main__." before errors


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
while s_index1 != -1 != s_index2:
    tmp_index = min(s_index1, s_index2)
    row = readed_file[: tmp_index].count("\n") + 1
    col = tmp_index - readed_file[: tmp_index].rfind("\n")
    if readed_file[tmp_index - 1] == '\\':
        translation = False
        print(f"Error: Line {row}, column {col - 1}: unexpected backslash")

    if s_index1 < s_index2:
        singlei = readed_file.find("'", s_index1 + 1)
        if singlei == -1:
            string_indices.append(range(s_index1, size))
            s_index1, s_index2 = -1, -1
            translation = False
            print(f"Error: Line {row}, column {col}: unfinished string")
            break
        while readed_file[singlei - bscount - 1] == '\\':
            bscount += 1
        while bscount & 1:
            bscount = 0
            singlei = readed_file.find("'", singlei + 1)
            if singlei == -1:
                translation = False
                print(f"Error: Line {row}, column {col}: unfinished string")
                break
            while readed_file[singlei - bscount - 1] == '\\':
                bscount += 1
        bscount = 0
        if singlei != -1:    
            singlei += 1
            string_indices.append(range(s_index1, singlei))
            s_index1 = readed_file.find("'", singlei)
            if s_index2 < singlei:
                s_index2 = readed_file.find('"', singlei)
        else:
            string_indices.append(range(s_index1, size))
            s_index1, s_index2 = -1, -1
            break
                
    else:
        doublei = readed_file.find('"', s_index2 + 1)
        if doublei == -1:
            string_indices.append(range(s_index2, size))
            s_index1, s_index2 = -1, -1
            translation = False
            print(f"Error: Line {row}, column {col}: unfinished string")
            break
        else:
            while readed_file[doublei - bscount - 1] == '\\':
                bscount += 1
            while bscount & 1:
                bscount = 0
                doublei = readed_file.find('"', doublei + 1)
                if doublei == -1:
                    translation = False
                    print(f"Error: Line {row}, column {col}: unfinished string")
                    break
                while readed_file[doublei - bscount - 1] == '\\':
                    bscount += 1
            bscount = 0
            if doublei != -1:            
                doublei += 1
                string_indices.append(range(s_index2, doublei))  # fun fact: this string indices list is already sorted on its own
                s_index2 = readed_file.find('"', doublei)
                if s_index1 < doublei:
                    s_index1 = readed_file.find("'", doublei)
            else:
                string_indices.append(range(s_index2, size))
                s_index1, s_index2 = -1, -1
                break



while s_index1 != -1:
    row = readed_file[: s_index1].count("\n") + 1
    col = s_index1 - readed_file[: s_index1].rfind("\n")
    if readed_file[s_index1 - 1] == '\\':
        translation = False
        print(f"Error: Line {row}, column {col - 1}: unexpected backslash")
    singlei = readed_file.find("'", s_index1 + 1)
    if singlei == -1:
        translation = False
        string_indices.append(range(s_index1, size))
        s_index2 = -1
        print(f"Error: Line {row}, column {col}: unfinished string")
        break
    while readed_file[singlei - bscount - 1] == '\\':
        bscount += 1
    while bscount & 1:
        bscount = 0
        singlei = readed_file.find("'", singlei + 1)
        if singlei == -1:
            translation = False
            print(f"Error: Line {row}, column {col}: unfinished string")
            break
        while readed_file[singlei - bscount - 1] == '\\':
            bscount += 1
    bscount = 0
    if singlei != -1:
        singlei += 1
        string_indices.append(range(s_index1, singlei))
        s_index1 = readed_file.find("'", singlei)
    else:
        string_indices.append(range(s_index1, size))
        s_index2 = -1
        break



while s_index2 != -1:
    row = readed_file[: s_index2].count("\n") + 1
    col = s_index2 - readed_file[: s_index2].rfind("\n")
    if readed_file[s_index2 - 1] == '\\':
        translation = False
        print(f"Error: Line {row}, column {col - 1}: unexpected backslash")
    doublei = readed_file.find('"', s_index2 + 1)
    if doublei == -1:
        translation = False
        string_indices.append(range(s_index2, size))
        print(f"Error: Line {row}, column {col}: unfinished string")
        break
    while readed_file[doublei - bscount - 1] == '\\':
        bscount += 1
    while bscount & 1:
        bscount = 0
        doublei = readed_file.find('"', doublei + 1)
        if doublei == -1:
            translation = False
            print(f"Error: Line {row}, column {col}: unfinished string")
            break
        while readed_file[doublei - bscount - 1] == '\\':
            bscount += 1
    bscount = 0
    if doublei != -1:
        doublei += 1
        string_indices.append(range(s_index2, doublei))
        s_index2 = readed_file.find('"', doublei)
    else:
        string_indices.append(range(s_index2, size))
        break





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
        raise Error(f"Line {row}, column {col - 3}: missing keyword 'func' to corresponding keyword 'endfunc'")

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
            row1 += readed_file[rIndex1 : errIndex1].count("\n")
            col = errIndex1 - readed_file[: errIndex1].rfind("\n")
            rIndex1 = errIndex1
            errIndex1 = readed_file.find("func", errIndex1 + 4, fi2)
            translation = False
            valid = False
            print(f'Error: Line {row1}, column {col}: cannot declare a function inside another function, eight "bruh"s for you')
    while errIndex2 != -1:
        for sindex in string_indices:
            if errIndex2 in sindex:
                errIndex2 = readed_file.find("class", errIndex2 + 5, fi2)
                break
        else:
            row2 += readed_file[rIndex2 : errIndex2].count("\n")
            col = errIndex2 - readed_file[: errIndex2].rfind("\n")
            rIndex2 = errIndex2
            errIndex2 = readed_file.find("class", errIndex2 + 5, fi2)
            translation = False
            valid = False
            print(f"Error: Line {row2}, column {col}: cannot declare a class inside a function")
    
    nextfunc = readed_file[rType + 1 : parbracket].replace(" ", "")
    if nextfunc == "main":
        translation = False
        print(f"Error: Line {row}, column {col}: cannot call a function \"main\" because the main function is placed in the global scope")
        break
    
    for func in functions:
        func = func[0].replace(" ", "")
        if nextfunc == func[: func.find(":")]:
            translation = False
            valid = False
            print(f"Error: Line {row}, column {col}: declaration of duplicate function")
    if valid:
        functions.append((readed_file[index : fi2], row))
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
    class_regex = r"[a-z A-Z] [inherits]? [a-z A-Z]()\w.endclass{1}"  #TODO unstable, needs work 


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


#_compile("ello ello")


def debug():
    """ Only meant for debugging purposes """
    os.system(f"cd {pathlib.Path.home()}/Lambda-Code/core/")
    os.system("python3 compiler.py C:/Users/Ashok Kumar/Lambda-Code/core/print_statement.lc")
#debug()
