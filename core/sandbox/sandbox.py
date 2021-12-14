import os
import sys
import pathlib


def pre_process(source_: str, flags: str = "") -> None:
    """
    Deals with statements that needs to be
    handled right after the compiler started
    :return: None
    """

    "Parsing import statements, RIP regular expressions"
    imports = []
    with open(source_, 'r') as i_source:  # i stands for imports
        for i_line in i_source:
            i_strip_ln = i_line.strip()
            if '#' in i_strip_ln:
                i_helper_list = i_strip_ln.split()
                try:
                    i_helper_list.remove('#')
                    i_helper_list.remove('import')
                    imports.append(''.join(i_helper_list))
                except ValueError: pass
    for _import in imports:
        cls_imported = _import.split('.')[-1]

        "In case a single file is imported"
        if len(_import.split('.')) == 1:
            if 'win' in sys.platform:
                module_path = f"{pathlib.Path.home()}roaming{os.sep}lpm{os.sep}packages{os.sep}{_import}"
                if os.path.isfile(module_path)
                    module_content = open(module_path, 'r').read()
            else:
                module_path = f"{pathlib.Path.home()}.lpm{os.sep}packages{os.sep}{_import}"
                if os.path.isfile(module_path):
                    module_content = open(module_path, 'r').read()


pre_process('../test.lc', 'string')
