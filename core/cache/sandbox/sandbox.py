""" Just a simple dir and file for testing new stuffs, not meant for end-users """


def class_parser(snippet: str) -> dict:
    """
     :param snippet: the piece of code to be parsed
    """
    _types = ["string", "int", "float", "array"]
    parsed_dict = {
        "name": "Class", "params": [], "super_class": None,
    }

    def get_name() -> str:
        name = snippet.split()
        return name[1]

    def get_super_classes() -> list:
        try:
            _inheritance = ((" ".join((snippet.split("inherits")[1]).split())).split("(")[0]).split()
            _inheritance.remove("and")
            return _inheritance
        except IndexError:
            pass

    def get_params() -> list:
        _params = (snippet.split("(")[1]).split(")")[0]
        __names = " ".join((" ".join(_params.split(','))).split(":")).split()
        __types = " ".join((" ".join(_params.split(','))).split(":")).split()
        for item in __types:
            """ Iterates and removes every item except the data types """
            if not item in _types:
                __types.remove(item)
        for _item in __names:
            """ Iterates over __names and removes data types """
            if _item in _types:
                __names.remove(_item)
        return [__names, __types]

    parsed_dict["name"] = get_name()
    parsed_dict["super_class"] = get_super_classes()
    parsed_dict["params"] = get_params()
    return parsed_dict


test = '''
class HelloWorld(int param1, string param2, float param3)
    func main()
        print("Hello world!")
    endfunc
endclass
'''

print (class_parser(test))
