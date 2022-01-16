""" 
Copyright (C) 2022 Lambda Code Organization

This file is part of the Lambda Code programming language

Lambda Code is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Lambda Code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. 
"""
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


def parse_functions(ranges: list, file: str = '', flags: str = '') -> list:
    """
    Parses functions and returns a Syntax tree at the end
    :param ranges: ranges of functions(list, start: end)
    :param file: target file to look for the ranges
    :param flags: optional available flags, available flags
    are ['debug', 'string --is-list']
    """

    __ast__ = []
    if 'string --is-list' not in flags:
        with open(file, 'r') as file:
            file = file.readlines()
            for _range in ranges:
                "Extracting the string from range"
                rvec_x = int(_range.split(':')[0]) - 1  # r = range
                rvec_y = int(_range.split(':')[-1])
                func_string = ''.join(file[rvec_x: rvec_y])

                "Extracting the name of the function from the string"
                func_name = ''.join(func_string.split('func')).split()[0]
                if '(' in func_name:
                    func_name = func_name.split('(')[0]

                "Checking the return type"
                return_type = 'void'
                endl_sep0 = func_string.split('\n')[0]
                if ':' in endl_sep0:
                    return_type = endl_sep0.split(':')[-1].lstrip()

                "Parameters, the lengthy part"
                param_list_comma_sep = endl_sep0.split('(')[-1].split(')')[0].split(',')
                final_param_tree = []

                for _param in param_list_comma_sep:

                    rem_t_spaces = _param.lstrip()
                    param_name = None
                    p_default = None if '=' not in rem_t_spaces else rem_t_spaces.split('=')[-1]
                    p_type = rem_t_spaces.split()[0]

                    if not p_default:
                        param_name = rem_t_spaces.split()[-1]
                    else:
                        param_name = rem_t_spaces.split('=')[0].split()[-1].lstrip()
                    final_param_tree.append(
                        {
                            'name': param_name,
                            'default': p_default,
                            'type': p_type
                        }
                    )

                __ast__.append(
                    {
                        'name': func_name,
                        'returns': return_type,
                        'params': final_param_tree
                    }
                )

    else:
        for function in ranges:
            pass

    return __ast__


# print(parse_functions(['10:13'], '../../test.lc'))
