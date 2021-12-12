import os
import sys
import pathlib


def pre_process(source_: str, flags: str = "") -> None:
    """
    Deals with statements that needs to be
    handled right after the compiler started
    :return: None
    """

    "Dealing with import statements"
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
                except ValueError:
                    pass


pre_process('../test.lc', 'string')
