import os
from flags import Flags


def system(*args: str, flags: Flags = Flags.NONE):
    if flags == Flags.SPLIT_WITH_ENDL:
        for command in args[0].split('\n'):
            os.system(command)

    for command in args:
        os.system(command)


system("""

""")
