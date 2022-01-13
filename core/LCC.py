"""
Lambda-Code
===========================================
A compiled, high-level and object-oriented
programming language. This file contains unexpected
EOF/EOL(s) checkers and pre-processors.
Also integrates every scattered components(such as parsers)
into a single language file.

Copyright (C) 2022 Lambda Code Organization

This file is part of the Lambda Code programming language

Lambda Code is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Lambda Code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
"""

import os
import sys
import pathlib
import argparse
from parsers.core import functions


sys.tracebacklimit = 0  # Removes the annoying traceback text

# the command line interface for the compiler
arg_parser = argparse.ArgumentParser(
    prog="LCC",
    description="Compiler for lambda code",
    formatter_class=argparse.RawDescriptionHelpFormatter,
    epilog="""
Report Bugs At https://github.com/Lambda-Code-Organization/Lambda-Code/issues
""",
    usage="LCC [options] file",
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


class Error:
    def __init__(self, message: str = "", exit_status: int = 1) -> None:
        """
        Writes the message to CLI/Console and exits with the specified error message
        with defaults to 1

        :param message: The message to display, "Error" is automatically added at the
        start of the string literal
        :param exit_status: the exist status to exit the interpreter with, defaults to 1
        """
        sys.stdout.write(f"Error: {message}")
        sys.exit(exit_status)


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
    Error("Null File")

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
for_indices = []
while_indices = []


"Checks for errors and produces two list of indexes of where strings or comments start and end at"
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
            sys.stdout.write(f"Error: Line {row}, column {col}: unfinished string")
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
            translation = False
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
        string_indices.append(range(s_index2, doublei))  # Fact: this string indices list is already sorted on its own
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
        Error(f"Line {row}, column {col}: function declaration incomplete")

    elif index == fi2 + 3:
        translation = False
        Error(f"Line {row}, column {col - 3}: \"endfunc\" cannot be used without \"func\"")

    else:
        for sindex in string_indices:
            if fi2 in sindex:
                fi2 = readed_file.find("endfunc", sindex[-1] + 1)
                continue

    if fi2 == -1:
        translation = False
        Error(f"Line {row}, column {col}: function declaration incomplete")

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

    nextfunc = readed_file[rType + 1: parbracket].replace(" ", "")
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

with open(parsed_args.file) as c_target_file:
    t_lines = c_target_file.readlines()
    h_cls_index = []
    two_multiples = ['2', '4', '6', '8', '0']

    for c_line in t_lines:
        if 'class' in c_line:
            h_cls_index.append(t_lines.index(c_line) + 1)
    len_cls_index = str(len(h_cls_index))
    if len_cls_index in two_multiples:
        pass
    else:
        Error("Incomplete class definition")

    "For and while loop indexing in a single iteration"
    for lp_line in t_lines:
        # TODO
        if 'for' in lp_line:
            lp_index = t_lines.index(lp_line)
            for s_range in string_indices:
                if lp_index in s_range:
                    pass
                else:
                    for_indices.append(lp_index)
        if 'endfor' in lp_line:
            for_indices.append(t_lines.index(lp_line))
        if 'while' in lp_line:
            while_indices.append(t_lines.index(lp_line))
        if 'endwhile' in lp_line:
            while_indices.append(t_lines.index(lp_line))


def cache(__file: str) -> str:
    """
    Creates a cache if it does not already exist and returns a path to the cached file
    :return: path of the cache file(str)
    """

    rl = open(__file).readlines()  # rl stands for read-lines

    try:
        os.mkdir(f"{os.path.dirname(__file)}{os.sep}__lc_cache__")
        with open(f"{os.path.dirname(__file)}{os.sep}__lc_cache__{os.sep}{__file.split(os.sep)[-1]}", 'x') \
                as translation_unit:
            for t_line in rl:
                translation_unit.write(t_line)
    except FileExistsError:
        pass

    ret = f"{os.path.dirname(__file)}{os.sep}__lc_cache__{os.sep}{__file.split(os.sep)[-1]}"

    if os.path.isfile(ret):
        return ret
    else:
        with open(f"{os.path.dirname(__file)}{os.sep}__lc_cache__{os.sep}{__file.split(os.sep)[-1]}", 'x') \
                as translation_unit:
            for t_line in rl:
                translation_unit.write(t_line)
        return ret


def pre_process(source: str) -> None:
    """
    Pre-processor, deals with statements that needs to be
    handled right after the compiler started
    :param source: must be a path pointing to a .lc file to process
    :return: None
    """

    "Parsing import statements, RIP regular expressions"
    imports = []
    with open(source, 'r') as i_source:  # i stands for imports
        for i_line in i_source:
            i_strip_ln = i_line.strip()
            if '#' in i_strip_ln:
                i_helper_list = i_strip_ln.split()
                try:
                    i_helper_list.remove('#')
                    i_helper_list.remove('import')
                    imports.append(''.join(i_helper_list))
                except ValueError:
                    pass

        "Pasting contents from the imported file into the namespace calling it"
        for _import in imports:
            cls_imported = _import.split('.')[-1]
            "In case a single file is imported"
            if 'win' in sys.platform:
                if os.path.isfile(f"{pathlib.Path.home()}roaming{os.sep}lpm{os.sep}\
                        packages{os.sep}{_import.split('.')[-1]}.lc"):
                    try:
                        module_content = open(_import, 'r').read()
                    except FileNotFoundError:
                        module_path = f"{pathlib.Path.home()}roaming{os.sep}lpm{os.sep}packages{os.sep}\
                                            {_import.replace('.', os.sep)}"
                        if os.path.isfile(module_path):
                            module_content = open(module_path, 'r').read()
                            # TODO use an alternative to .read() to save RAM
                            i_source.read().replace(f'import {_import}', module_content)
                        else:
                            sys.stdout.write(f"Error: file {source}\n no module named {_import}")
                            sys.exit(1)
            else:
                if os.path.isfile(f"{pathlib.Path.home()}.lpm{os.sep}packages{os.sep}{_import.split('.')[-1]}.lc")\
                        or os.path.isfile(f"{_import.replace('.', os.sep)}"):
                    try:
                        module_content = open(_import, 'r').read()
                    except FileNotFoundError:
                        module_path = f"{pathlib.Path.home()}.lpm{os.sep}packages{os.sep}{_import.replace('.', os.sep)}"
                        if os.path.isfile(module_path):
                            module_content = open(module_path, 'r').read()
                            i_source.read().replace(f"import {_import}", module_content)
                        else:
                            Error(f"file {source}\n module not found, no module named {_import}")

    return  # just to be on the safe side :)


pre_process(cache(str(FILE_NAME)))


def _compile() -> int:
    """
    Invokes parsers and generates a C++ source file, return type (int) defines the
    exit status
    :return: ExitStatus
    """
    return 0

def _execute() -> None:
    """
    Invokes the C/C++ compiler present on the user's system and executes the C++
    source generated by _compile
    :return: NoReturn
    """
    return

if translation:
    pass  # TODO
else:
    sys.exit(1)
