import sys


def log(text: str):
    ANSI_RED = "\u001B[31m"

    sys.stdout.write(ANSI_RED + text + ANSI_RED)

