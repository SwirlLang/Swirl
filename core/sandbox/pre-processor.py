import os
import sys


def pre_process(source: str, flags: str = None) -> None:
    """
    Pre-processor, deals with statements that needs to be
    handled right after the compiler started
    :param source: must be a path pointing to a .lc file to process
    :return: None
    """

    try:
        os.mkdir(f"{os.path.dirname(source)}{os.sep}__lc_cache__")
        open(f"{os.path.dirname(source)}{os.sep}__lc_cache__{os.sep}{source.split(os.sep)[-1]}", 'x')
    except FileExistsError:
        pass

    ret = f"{os.path.dirname(source)}{os.sep}__lc_cache__{os.sep}{source.split(os.sep)[-1]}"

    if os.path.isfile(ret):
        pass
    else:
        open(f"{os.path.dirname(source)}{os.sep}__lc_cache__{os.sep}{source.split(os.sep)[-1]}", 'x')

    "Parsing import statements, RIP regular expressions"
    imports = []
    with open(ret, 'r') as __source:  # i stands for imports
        _source = __source.readlines()
        print(_source)

        "Removing comments"
        for i_line in _source:
            if '#' in i_line:
                # TODO check if the stmt is present in a string or a comment
                i_helper_list = i_line.split()
                try:
                    i_helper_list.remove('#')
                    i_helper_list.remove('import')
                    imports.append(''.join(i_helper_list))
                except ValueError:
                    pass

        print(imports)
        module_path_list = [module.replace('.', f'{os.sep}') for module in imports]
        print(module_path_list)


pre_process(r"C:\Users\Ashok Kumar\Lambda-Code\tests\project1\main.lc")
