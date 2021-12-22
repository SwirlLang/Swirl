__all__ = 'loops_parser'

import sys


def loops_parser(snippet: str, flags: str = '') -> list:
    __ast__ = []
    for_split = snippet.split('endfor')
    for _item in for_split:
        if _item == '\n':
            for_split.remove('\n')
    for loops in for_split:
        target = ''.join(loops.split('in')[0]).split()
        for item in target:
            if item == 'for':
                target.remove('for')
        _object = loops.split('in')[1].split('\n')[0]
        __ast__.append(
            {
                "target": ''.join(target),
                "object": _object,
                "content": loops[int(loops.index(''.join(target))):]
            }
        )

    return __ast__


t = '''
for i in range(12, 198)
    print(i)
    print('h')
    print('h')
endfor

for g in range(12, 198)
    print(i)
    print('h')
    print('h')
    print('h')
endfor

for h in range(12, 198)
    print(i)
    print('h')
endfor

for k in range(12, 198)
    print(i)
    print('h')
endfor
'''

print(loops_parser(t, 'debug'))
