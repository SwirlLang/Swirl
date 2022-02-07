import sys


def log(text: str):
    ANSI_RED: str = "\u001B[31m"

    sys.stdout.write(ANSI_RED + text + ANSI_RED)

