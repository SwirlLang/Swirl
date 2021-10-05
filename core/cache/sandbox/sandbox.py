""" Just a simple dir and file for testing new stuffs, not meant for end-users """


def func_parser(snippet: str) -> list:
    __main_dict__ = []
    _funcs = snippet.split("func ")
    _types = ["string", "int", "float", "array"]
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

        __main_dict__.append(
            {
                "name": function.split('(')[0],
                "params": [_param_name, _param_types]
            }
        )

    return __main_dict__


test = '''
func helloWorld(param1: string, param2: int)
    print("hello world!")
endfunc

func byeWorld(param1: string, param2: int)
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
