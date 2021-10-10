def master_parser(file: str) -> None:
    """
    :param file: the file to parse (dispatch)
    """
    splitter = file.split()
    if "class" in splitter:
        print("Why ?")

test = '''
class Test()
    func test_2:void()
        print("hello world from class")
    endfunc
endclass

func test:void()
    prinr("hello world from a function") 
endfunc
'''
print(master_parser(test))  