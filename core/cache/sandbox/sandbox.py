""" Just a simple dir and file for testing new stuffs, not meant for end-users """

import os
import logging


class Error(Exception):
    """ Class for compiler's error """
    __module__ = "builtins"


def func_parser(snippet: str, check_for_conventions: bool = False) -> list:
    """
    :param snippet: the snippet to parse (str)
    :param check_for_conventions: if true, will return a warning for wrong conventions (bool). Defaults to False
    """

    __main_dict__ = []
    _funcs = snippet.split("func ")
    _types = ["string", "int", "float", "array"]
    invalid_chars = [
        "!", "@", "#", "$", "%", "^", "&", "*", "(", ")"
        "~", "`", "<", ">", "?", "/", "{", "}", "[", "]",
        "%s" % os.pathsep, "|", "1", "2", "3", "4", "5",
        "6", "7", "8", "9", "0"
    ]
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
        func_name = function.split('(')[0]

        if check_for_conventions:
            if func_name[0].isupper():
                logging.warning(f"The name of the function should start with lower case\nFunction '{func_name}'")
                pass

        if func_name[0] in invalid_chars:
            raise Error(
                f"The name of the function at index {_funcs.index(function)} is starting with an invalid "
                f"character '{func_name[0]}'\nCompilation Terminated "
            )

        __main_dict__.append(
            {
                "name": func_name,
                "params": [_param_name, _param_types]
            }
        )

    return __main_dict__


test = '''
func helloWorld(param1: string, param2: int)
    print("hello world!")
endfunc

func ByeWorld(param1: string, param2: int)
    print("bye world")
endfunc

func byeWorld3(param1: string, param2: int)
    print("bye world")
endfunc

func byeWorld4(param1: string, param2: int)
    print("bye world")
endfunc
'''

print(func_parser(test, True))
