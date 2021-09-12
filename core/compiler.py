""" The core of the compiler """

import re
import sys

filename = ''
try:
    filename = sys.argv[1]
except IndexError:
    print("Error: no input file specified")
    quit()


source = open(filename, "r")
readed_file = source.read()
functions = []
index = 0
gvariables = {}

while readed_file.find('func ', index) != -1:
    functions.append(readed_file[readed_file.find('func ', index):readed_file.find('end', index) + 3])
    index = readed_file.find('end', index) + 3

main_func = readed_file

for func in functions:
    main_func = main_func.replace(func, "")

print(functions)
source.close()

example_code = '''
int abc = 9
int b = 10
float c = 0.9
float c = 9
string[99] name = "mrinmoy"
array arr[int,5] = (1,2,2,3,3,4)
array arr[int,5] = 1, 2, 2, 3 , 3,4
// hello world this is a comment
'''

class re_patterns():
    int_regex = "int \\w* = \\d"
    float_regex = "(float \\w* = \\w*\\.\\w*)|(float \\w* = \\w*)"
    string_regex = "string\[\d*\].*"
    array_regex = "(array \\w*\\[\\w*,\\d\\] = \\d.*)|(array \\w*\\[\\w*,\\d\\] = \\(\\d.*)"


print(re.findall(re_patterns.int_regex,example_code))
print([fnumber for result in re.findall(re_patterns.float_regex, example_code) for fnumber in result if fnumber != ""])
print(re.findall(re_patterns.string_regex, example_code))
print([ar for result in re.findall(re_patterns.array_regex, example_code) for ar in result if ar != ""])
