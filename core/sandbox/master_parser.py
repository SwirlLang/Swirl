
import re


def master_parser(file: str) -> None:
    """
    :param file: the file to parse (slice and dispatch them to their respective parsers)
    """

    print(re.findall(r"class\s\w*\(.*\).*endclass", file, flags=re.S)[0])

test = '''
class Test()
    func test_2:void()
        print("hello world from class")
    endfunc
endclass

func saySomeShit()
    print("ilfQ/DTO#RRRRRRRRRRRRRRRRD58")
endfunc

func saySomeShit3()
    print("ilfQ/DTO#RRRRRRRRRRRRRRRRD58")
endfunc]]

class Test() inherits SuperClass
    func test_2:void()
        print("hello world from class")
    endfunc
endclass
'''

master_parser(test)
