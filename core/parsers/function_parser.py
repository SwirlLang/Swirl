__all__ = "func_parser"

import string
import sys


def func_parser(snippet: str, flags: str = None) -> list:
    """
    Function parser: Parses ranges(start: end) of the functions,
    and returns a formatted syntax tree
    :param snippet: The snippet to parse(str)
    :param flags: available flags: 'debug', returns
    a parse tree with improved readability, for
    development use only.
    """

    __ast__ = []  # final abstract syntax tree
    _funcs = snippet.split("func ")
    _types = ["string", "int", "float", "array"]
    ascii = string.ascii_lowercase
    _valid_naming_chars = list(ascii)
    _param_name = []
    _param_types = []

    for item in _funcs:
        if item == "\n":
            _funcs.remove(item)
    for function in _funcs:
        helper = (("".join(" ".join(function.split(function.split("(")[0])).split(")"))).split("\n")[0]).split("(")
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

        "Bug fixes and error checking in the function name"
        for dicts_ in __ast__:
            if len(dicts_["params"][0]) != len(dicts_["params"][1]) \
                    and len(dicts_["params"][0]) == 1:
                dicts_["params"][0] = []
            for type__ in dicts_['params'][1]:
                if type__ not in _types:
                    dicts_['params'][1].remove(type__)
            for name__ in dicts_['params'][0]:
                if name__ == '=':
                    dicts_['params'][0].remove(name__)

        __ast__.append(
            {
                "name": function.split("(")[0],
                "params": [_param_name, _param_types],
                "returns": ""
            }
        )

        "Checking if the name of the function is invalid"
        for e_function in __ast__:
            if e_function['name'][0].lower() not in _valid_naming_chars:
                raise Exception(
                    f"Function {e_function['name']} begins with an invalid character '{e_function['name'][0]}'"
                )

        if flags == 'debug':
            print(1)
            for dicts in __ast__:
                sys.stdout.write(f"""
            \t\t {dicts['name']}
                     |
                     |-- params
                            |
                            |---[[names], [types]] = {dicts['params']}      
                """
                                 )
    return __ast__


t = '''
func hello2332ooo(): int
    print("statement")
endfunc
'''

print(func_parser(t, 'debug'))
