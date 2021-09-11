""" Compiler of the programming language """

import re
import sys


try:
    filename = (sys.argv[1], False)
except IndexError:
    print("Error: no input file specified")
    filename = ('', True)

if filename[1]:
    quit()

filename = filename[0]
source = open(filename, "r").read()
functions = []
index = 0

while source.find('func ', index) != -1:
    functions.append(source[source.find('func ', index):source.find('end', index) + 3])
    index = source.find('end', index) + 3


