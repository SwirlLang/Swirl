"""
Lambda-Code
===========================================
A fantastic, compiled, high-level and object-oriented
programming language. This file contains unexpected
EOF/EOL(s) checkers and pre-processors.
Also integrates every scattered components(such as parsers)
into a single language file.
"""

import os
import sys
import pathlib
import argparse


sys.tracebacklimit = 0  # Removes the annoying traceback text

# the command line interface for the compiler
arg_parser = argparse.ArgumentParser(
    prog="LCC",
    description="Compiler for lambda code",
    formatter_class=argparse.RawDescriptionHelpFormatter,
    epilog="""
Report Bugs At https://github.com/Lambda-Code-Organization/Lambda-Code/issues
""",
)


arg_parser.add_argument(
    "file",
    type=str,
    help="Input File Name")

arg_parser.add_argument(
    "-o",
    "--output",
    nargs="?",
    help="Output File Name",
    type=str)

parsed_args = arg_parser.parse_args()


def pre_process(source_: str, flags: str = "") -> None:
    """
    Deals with statements that needs to be
    handled right after the compiler started
    :return: None
    """

    "Parsing import statements, RIP regular expressions"
    imports = []
    with open(source_, 'r') as i_source:  # i stands for imports
        for i_line in i_source:
            i_strip_ln = i_line.strip()
            if '#' in i_strip_ln:
                i_helper_list = i_strip_ln.split()
                try:
                    i_helper_list.remove('#')
                    i_helper_list.remove('import')
                    imports.append(''.join(i_helper_list))
                except ValueError: pass

    "Pasting contents from the imported file into the target file"
    for _import in imports:
        cls_imported = _import.split('.')[-1]
        "In case a single file is imported"
        if len(_import.split('.')) == 1:
            try:
                module_content = open(_import, 'r').read()
            except FileNotFoundError:
                if 'win' in sys.platform:
                    module_path = f"{pathlib.Path.home()}roaming{os.sep}lpm{os.sep}packages{os.sep}{_import}"
                    if os.path.isfile(module_path):
                        module_content = open(module_path, 'r').read()
                        #TODO
                else:
                    module_path = f"{pathlib.Path.home()}.lpm{os.sep}packages{os.sep}{_import}"
                    if os.path.isfile(module_path):
                        module_content = open(module_path, 'r').read()
                        # TODO


class Error(Exception):
    __module__ = "builtins"  # Removes the annoying "__main__." before errors


def binary_search(indices: list, start: int, end: int, index: int) -> int:
    high = end
    low = start
    while high >= low:
        middle = (low + high) >> 1
        if index > indices[middle][-1]:
            low = middle + 1
        elif index < indices[middle][0]:
            high = middle - 1
        else:
            return middle
    return -1


FILE_NAME = parsed_args.file
output_filename = parsed_args.output

source = open(FILE_NAME, "r")
readed_file = source.read()
size = len(readed_file)

if not size:
    raise Error("Null File")

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
class_indices: list


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
                sys.stdout.write(f"Error: Line {row}, column {col}: unfinished multi-line comment")
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
            sys.stdout.write(f"Error: Line {row}, column {col - 1}: unexpected character - backslash")
        singlei = readed_file.find("'", s_index1 + 1)
        if singlei == -1:
            translation = False
            sys.stdout.write(f"Error: Line {row}, column {col}: unfinished string")#
            break
        while readed_file[singlei - bscount - 1] == "\\":
            bscount += 1
        while bscount & 1:
            bscount = 0
            singlei = readed_file.find("'", singlei + 1)
            if singlei == -1:
                translation = False
                valid = True
                sys.stdout.write(f"Error: Line {row}, column {col}: unfinished string")
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
            sys.stdout.write(f"Error: Line {row}, column {col - 1}: unexpected character - backslash")
        doublei = readed_file.find('"', s_index2 + 1)
        if doublei == -1:
            translation = False
            sys.stdout.write(f"Error: Line {row}, column {col}: unfinished string")
            break
        while readed_file[doublei - bscount - 1] == "\\":
            bscount += 1
        while bscount & 1:
            bscount = 0
            doublei = readed_file.find('"', doublei + 1)
            if doublei == -1:
                translation = False
                valid = True
                sys.stdout.write(f"Error: Line {row}, column {col}: unfinished string")
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
        raise Error(f"Line {row}, column {col}: function declaration incomplete")

    elif index == fi2 + 3:
        translation = False
        raise Error(f"Line {row}, column {col - 3}: \"endfunc\" cannot be used without \"func\"")

    else:
        for sindex in string_indices:
            if fi2 in sindex:
                fi2 = readed_file.find("endfunc", sindex[-1] + 1)
                continue

    if fi2 == -1:
        translation = False
        raise Error(f"Line {row}, column {col}: function declaration incomplete")

    index += 4

    rType = readed_file.find(":", index, fi2)
    if rType == -1:
        translation = False
        index = readed_file.find("func", fi2 + 7)
        sys.stdout.write(f"Error: Line {row}, column {col}: missing function return type")
        continue
    for sindex in string_indices:
        if rType in sindex:
            rType = readed_file.find(":", sindex[-1] + 1, fi2)
            break
    if rType == -1:
        translation = False
        index = readed_file.find("func", fi2 + 7)
        sys.stdout.write(f"Error: Line {row}, column {col}: missing function return type")
        continue

    parbracket = readed_file.find("(", rType, fi2)
    if parbracket == -1:
        translation = False
        index = readed_file.find("func", fi2 + 7)
        sys.stdout.write(f"Error: Line {row}, column {col}: missing parameters syntax in this function")
        continue
    for sindex in string_indices:
        if parbracket in sindex:
            parbracket = -1
            break
    if parbracket == -1:
        translation = False
        index = readed_file.find("func", fi2 + 7)
        sys.stdout.write(f"Error: Line {row}, column {col}: missing parameters syntax in this function")
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
            sys.stdout.write(f'Error: Line {row1}, column {col}: cannot declare a function within a function')
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
            sys.stdout.write(f"Error: Line {row2}, column {col}: cannot declare a class inside a function")
            break

    nextfunc = readed_file[rType + 1 : parbracket].replace(" ", "")
    if nextfunc == "main":
        translation = False
        sys.stdout.write(f'Error: Line {row}, column {col}: cannot call a function "main" because it is placed in the global scope')
        break

    for func in functions:
        func = func[0].replace(" ", "")
        if nextfunc == func[: func.find(":")]:
            translation = False
            valid = False
            sys.stdout.write(f"Error: Line {row}, column {col}: duplicate function declared")
    if valid:
        func_indices.append(range(index - 4, fi2 + 8))
        functions.append((readed_file[index: fi2], row))
    index = readed_file.find("func", fi2 + 7)
    valid = True

"Class indexing..."
with open(FILE_NAME, 'r') as c_target_file:  # c stands for class, a convention in this context
    f_content = c_target_file.read().split('\n')

source.close()

with open('../test.lc', 'r') as c_target_file:  # c stands for class, a convention in this context
    t_lines = c_target_file.readlines()  # t: target
    h_cls_index = []
    two_multiples = ['2', '4', '6', '8', '0']
    for c_line in t_lines:
        if 'class' in c_line:
            h_cls_index.append(t_lines.index(c_line) + 1)
    len_cls_index = str(len(h_cls_index))
    sys.stdout.write(len_cls_index)
    if len_cls_index in two_multiples: pass
    else:
        raise Exception("Incomplete class definition")

AST = list

# TODO
# def func_parser(ranges: list) -> AST:
#     """
#     Parses functions at provided ranges and produces an AST
#     :param ranges: a list of ranges (begin: end) of functions
#     :return: Abstract syntax tree AKA parse or syntax tree
#     """
#
#     __ast__ = []  # the final syntax tree
#     for f_range in ranges:
#         with open(FILE_NAME) as target_file:
#             _source = target_file.readlines()
#             function = _source[]
#
#     return __ast__


def _compile(func_ast: str, variables_ast: str, classes_ast: str) -> int:
    """
    Generates a C++ source file in respect with the AST(s) provided
    :param func_ast: AST of functions in x file
    :param variables_ast: AST of variables in x file
    :param classes_ast: AST of classes in x file
    """
    return 0


if translation:
    pass  # TODO
else:
    sys.exit(1)
