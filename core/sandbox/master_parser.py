# DEPRECATED in favour of pattern matching

def master_parser(file: str) -> None:
    """
    :param file: the file to parse (slice and dispatch them to their respective parsers)
    """
    splitter = file.split()
    classes = []
    functions = []

    for TOKEN in splitter:
        if TOKEN == "class":
            classes.append(
                splitter[splitter.index(TOKEN): splitter.index("endclass")]
            )
            print(classes)
            continue


test = '''
class Test()
    func test_2:void()
        print("hello world from class")
    endfunc
endclass

func test:void()
    print("hello world from a function") 
endfunc

class Test2()
    func test_2:void()
        print("hello world from class")
    endfunc
endclass
'''

master_parser(test)
