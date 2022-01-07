# with open('../test.lc', 'r') as c_target_file:  # c stands for class, a convention in this context
#     t_lines = c_target_file.readlines()  # t: target
#     h_cls_index = []
#     two_multiples = ['2', '4', '6', '8', '0']
#     for c_line in t_lines:
#         if 'class' in c_line:
#             h_cls_index.append(t_lines.index(c_line) + 1)
#     len_cls_index = str(len(h_cls_index))
#     if len_cls_index in two_multiples: pass
#     else:
#         raise Error("Incomplete class definition")

import re
import sys
import string
from typing import NoReturn

from functions import parse_functions


class Error(Exception):
    __module__ = 'builtins'


def gen_cls_parse_tree(name: str, c_params: list, supers: list) -> str:
    cls_def = f"""
stmt
    ClassDef
        ident {name}
"""
    for _dict in c_params:
        cls_def += f"""
        ParamDef
            ident {_dict["name"]}
            type {_dict["type"]}
            default {_dict["default"] if _dict["default"] is not None else "NO_DEFAULT_ARGUMENT_GIVEN_64"}
"""

    for _class in supers:
        cls_def += f"""
        Super
            ident {_class}
"""
    return cls_def


def parse_classes(ranges: list, file: str) -> str:
    """
    pareses classes in the provided ranges of the file
    :param ranges: ranges(start: end)
    :param file path pointing to the file to read from
    :return: Abstract syntax tree
    """
    __ast__ = []
    chars = [char for char in string.ascii_lowercase] + ['1', '2', '3', '4', '5', '6', '7', '8', '9', '0']
    with open(file, 'r') as file:
        file = file.readlines()
        for _range in ranges:
            "Separating ranges from a string"
            rvec_x = int(_range.split(':')[0]) - 1  # r = range
            rvec_y = int(_range.split(':')[-1])
            class_string = "".join(file[rvec_x: rvec_y])

            "Extracting the name"
            name = ''.join(class_string.split('class')).split()[0]
            if '(' in name:
                name = name.split('(')[0]
            for n_char in name:
                if n_char.lower() not in chars:
                    raise Error(f"File {file}\ninvalid character '{n_char}' in class name {name}")
                if n_char.lower() in ['1', '2', '3', '4', '5', '6', '7', '8', '9', '0']:
                    if name.index(n_char) < len(name):
                        raise Error(f"number {n_char} in between of a the class name {name}")

            "Extracting constructor parameters"
            h_param0 = class_string.split(name)[-1].split(')')[0].split('(')[-1]
            params_pairs = h_param0.split(',')
            final_params = []
            for _param in params_pairs:
                param_name = None
                default = None
                if '=' in _param:
                    # has default values
                    default = _param.split('=')[-1]
                if not default:
                    param_name = _param.split()[-1]
                else:
                    param_name = _param.split('=')
                final_params.append(
                    {
                        'name': param_name,
                        'default': default,
                        'type': _param.split()[0]
                    }
                )

            "Checking for and extracting super class(s)"
            if 'inherits' in class_string:
                h_inheritance = class_string.split('inherits')[-1].split('\n')[0].split(',')
                inheritance = []
                for super_class in h_inheritance:
                    inheritance.append(super_class.lstrip())

            "And finally appending everything into the syntax tree"
            __ast__.append(gen_cls_parse_tree(name, final_params, inheritance))

    return ''.join(__ast__)


# print(parse_classes(['4:8'], '../../../tests/project1/main.lc'))

print(gen_cls_parse_tree("ello", [{"name": "yay", "default": "2323", "type": "int"}], ['wooHoo', 'thatThing']))
