__all__ = "class_parser"


def class_parser(snippet: str) -> list:
    __parsed_data__ = []
    _types = ["string", "int", "float", "array"]

    # just for easiness ...
    _t = snippet.split()
    splitter = snippet.split("class ")
    for item in splitter:
        if item == "\n":
            splitter.remove(item)

    # Iterating through and adding every class present, in the the list by using dicts
    for CLASS in splitter:
        name = ""
        super_classes = []
        constructor_params = []
        content = ''
        n_helper = CLASS.split()

        # if the class is a child class ...
        if 'inherits' in n_helper:
            name = n_helper[0]
            super_classes = "".join(
                ("".join(CLASS.split("inherits"))).split("(")[0]
            ).split()
            if "and" in super_classes:
                super_classes.remove("and")
            if name in super_classes:
                super_classes.remove(name)
            name_helper = "".join(("".join("".join(CLASS.split("(")[1])).split(")")[0]).split(",")).split()
            type_helper = "".join(("".join("".join(CLASS.split("(")[1])).split(")")[0]).split(",")).split()
            for _type_ in type_helper:
                if _type_ not in _types:
                    type_helper.remove(_type_)
            for _name_ in name_helper:
                if _name_ in _types:
                    name_helper.remove(_name_)
            constructor_params = [name_helper, type_helper]

        else:
            name = CLASS.split("(")[0]
            p_helper = (" ".join(CLASS.split('('))).split(")")
            Ptypes = ("".join(p_helper[0])).split()
            Ptypes.remove(Ptypes[0])
            Pnames = ("".join(p_helper[0])).split()
            Pnames.remove(Pnames[0])
            for _item in Pnames:
                if _item in _types:
                    Pnames.remove(_item)
            for _item_ in Ptypes:
                if _item_ not in _types:
                    Ptypes.remove(_item_)
            constructor_params = [Pnames, Ptypes]

        try:
            content = _t[_t.index(f"{constructor_params[0][-1]})"): _t.index("endclass")]
            for itm in content:
                if itm == f"{constructor_params[0][-1]})":
                    content.remove(itm)
        except Exception as err:
            raise err

        __parsed_data__.append(
            {
                "name": name,
                "super_classes": super_classes,
                "constructor_params": constructor_params,
                "content": content
            }
        )

    return __parsed_data__
