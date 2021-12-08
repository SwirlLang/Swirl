__all__ = "func_parser"


def func_parser(snippet: str) -> list:
    """
    Function parser; Parses ranges(start: end) of the functions,
    and returns a formatted syntax tree
    :param snippet: The snippet to parse(str)
    """

    __ast__ = []  # final abstract syntax tree
    _funcs = snippet.split("func ")
    _types = ["string", "int", "float", "array"]
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

        __ast__.append(
            {"name": function.split("(")[0], "params": [_param_name, _param_types]}
        )

    return __ast__
