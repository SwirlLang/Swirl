""" Just a simple dir and file for testing new stuffs, not meant for end-users """

import os


class ConventionError(Exception):
    __module__ = "builtins"


def func_parser(snippet: str) -> list:
    __main_dict__ = []
    _funcs = snippet.split("func ")
    _types = ["string", "int", "float", "array"]
    invalid_chars = [
        "!", "@", "#", "$", "%", "^", "&", "*", "(", ")"
        "~", "`", "<", ">", "?", "/", "{", "}", "[", "]",
        "%s" % os.pathsep, "|"
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
        if func_name[0] in invalid_chars:
            raise ConventionError(
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

func %yeWorld(param1: string, param2: int)
    print("bye world")
endfunc

func byeWorld3(param1: string, param2: int)
    print("bye world")
endfunc

func byeWorld4(param1: string, param2: int)
    print("bye world")
endfunc
'''

print(func_parser(test))
