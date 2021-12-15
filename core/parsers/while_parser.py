__all__ = 'while_parser'

def while_parser(snippet: str, flag: str = '')-> list:
    __ast__ = []
    while_split = snippet.splitlines()
    while_split.remove('')
    while_split.remove('endwhile')
    _condition = ''.join(while_split[0].split('while '))
    _content = while_split[1:]
    #print(while_split)
    __ast__.append(
        {
            "condition": _condition,
            "content": _content
        }
    )
    return __ast__

code = """
while i != 1
    print("test")
    more test
    even more test
    just more testing
    even more testing
    i think i should stop
    bruh ok
endwhile
"""
print(while_parser(code))