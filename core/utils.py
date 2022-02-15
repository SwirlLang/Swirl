import sys


def log(text: str):
    ANSI_RED: str = "\u001B[31m"

    sys.stdout.write(ANSI_RED + text + ANSI_RED)

def read_in_chunks(file, size=1024):
    # Lazy function to read a file piece by piece.

    while True:
        data = file.read(size)
        if not data:
            break
        yield data
