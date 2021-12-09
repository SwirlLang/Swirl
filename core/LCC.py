"""
Lambda-Code
===========================================
A fantastic, compiled high-level programming language.
This file contains unexpected EOF/EOL(s) checkers and
pre-processors. Also integrates every scattered components(such as parsers)
into a single language file.
Copyright Lambda Code Organization 2021
"""

import os
import argparse
import sys
import termcolor
import pathlib
import logging


sys.tracebacklimit = 0  # Removes the annoying traceback text

# the command line interface for the compiler
arg_parser = argparse.ArgumentParser(
    prog="LCC",
    description="Compiler for lambda code",
    formatter_class=argparse.RawDescriptionHelpFormatter,
    epilog="""
Copyright Lambda code foundation 2021.
report bugs at https://github.com/Lambda-Code-Organization/Lambda-Code
""",
)

arg_parser.add_argument(
    "file",
    type=str,
    help="Filename of the input file")
arg_parser.add_argument(
    "-o",
    "--output",
    nargs="?",
    help="Filename of the output file",
    type=str)

parsed_args = arg_parser.parse_args()


class Error(Exception):
    __module__ = "builtins"  # Removes the annoying "__main__." before errors


def binary_search(indices: list, index: int) -> bool:
    middle = len(indices)
    if middle > 1:
        if middle == 2:
            if index in indices[0] or index in indices[1]:
                return True
            else:
                return False
        middle //= 2
        offset = middle // 2 + 1
        while offset != 0:
            if index < indices[middle][0]:
                middle -= offset
                offset //= 2
            elif index >= indices[middle][1]:
                middle += offset
                offset //= 2
            else:
                return True
        if index in indices[middle]:
            return True
        else:
            return False
    elif middle == 0:
        return False
    else:
        if index in indices[0]:
            return True
        else:
            return False


filename = parsed_args.file
output_filename = parsed_args.output

source = open(filename, "r")
readed_file = source.read()
size = len(readed_file)

if not size:
    raise Error("empty file")

translation = True
valid = False
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
row = 1
tmp_index = 0

print(functions)
print(string_indices)
print()

"""Checks for errors and produces two list of indexes of where strings or comments start and end at"""
while (s_index1 + s_index2 + c_index1 + c_index2) != -4:
    min_index = min([x for x in (s_index1, s_index2, c_index1, c_index2) if x != -1])
    row += readed_file[tmp_index:min_index].count("\n")
    col = min_index - readed_file[:min_index].rfind("\n")
    tmp_index = min_index
    if min_index == c_index1:
        if c_index1 == c_index2:
            multi_c = readed_file.find("///", c_index2 + 3)
            if multi_c == -1:
                translation = False
                print(f"Error: Line {row}, column {col}: unfinished multi-line comment")
                break
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
        if readed_file[s_index1 - 1] == "\\":
            translation = False
            print(f"Error: Line {row}, column {col - 1}: unexpected backslash")
        singlei = readed_file.find("'", s_index1 + 1)
        if singlei == -1:
            translation = False
            print(f"Error: Line {row}, column {col}: unfinished string")#
            break
        while readed_file[singlei - bscount - 1] == "\\":
            bscount += 1
        while bscount & 1:
            bscount = 0
            singlei = readed_file.find("'", singlei + 1)
            if singlei == -1:
                translation = False
                valid = True
                print(f"Error: Line {row}, column {col}: unfinished string")
                break
            while readed_file[singlei - bscount - 1] == "\\":
                bscount += 1
        if valid:
            break
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
            translation = False
            print(f"Error: Line {row}, column {col}: unfinished string")
            break
        while readed_file[doublei - bscount - 1] == "\\":
            bscount += 1
        while bscount & 1:
            bscount = 0
            doublei = readed_file.find('"', doublei + 1)
            if doublei == -1:
                translation = False
                valid = True
                print(f"Error: Line {row}, column {col}: unfinished string")
            while readed_file[doublei - bscount - 1] == "\\":
                bscount += 1
        if valid:
            break
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

valid = True

while index != -1:
    for sindex in string_indices:
        if index in sindex:
            index = readed_file.find("func", sindex[-1] + 1)
            continue

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
        for sindex in string_indices:
            if fi2 in sindex:
                fi2 = readed_file.find("endfunc", sindex[-1] + 1)
                continue

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
    for sindex in string_indices:
        if rType in sindex:
            rType = readed_file.find(":", sindex[-1] + 1, fi2)
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
            row1 += readed_file[rIndex1:errIndex1].count("\n")
            col = errIndex1 - readed_file[:errIndex1].rfind("\n")
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
            row2 += readed_file[rIndex2:errIndex2].count("\n")
            col = errIndex2 - readed_file[:errIndex2].rfind("\n")
            rIndex2 = errIndex2
            errIndex2 = readed_file.find("class", errIndex2 + 5, fi2)
            translation = False
            valid = False
            print(f"Error: Line {row2}, column {col}: cannot declare a class inside a function")
            break

    nextfunc = readed_file[rType + 1 : parbracket].replace(" ", "")
    if nextfunc == "main":
        translation = False
        print(f'Error: Line {row}, column {col}: cannot call a function "main" because the main function is placed in the global scope')
        break

    for func in functions:
        func = func[0].replace(" ", "")
        if nextfunc == func[: func.find(":")]:
            translation = False
            valid = False
            print(f"Error: Line {row}, column {col}: declaration of duplicate function")
    if valid:
        func_indices.append(range(index - 4, fi2 + 8))
        functions.append((readed_file[index : fi2], row))
    index = readed_file.find("func", fi2 + 7)
    valid = True

source.close()


def pre_process() -> None:
    """
    Pre-processes 
    :return: None
    """


def _compile(func_ast: str, variables_ast: str, classes_ast: str) -> int:
    """
    Generates a C++ source file in respect with the AST(s) provided
    :param func_ast: AST of functions in x file
    :param variables_ast: AST of variables in x file
    :param classes_ast: AST of classes in x file
    """
    return 0  # indicates process finished with no errors


if translation:
    pass  # TODO
else:
    sys.exit(1)
