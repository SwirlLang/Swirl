""" Loops parser """


def parse_loops(ranges: list, file: str, flags: str = ""):
    with open(file) as target:
        t_list = target.readlines()

        for range_ in ranges:
            s_li = []
            for _line in t_list:
                if t_list.index(_line) in range_:
                    s_li.append(_line)

            l_string = "".join(s_li)
            print(s_li)


parse_loops([range(11, 13)], r"C:\Users\Ashok Kumar\Lambda-Code\tests\project1\__lc_cache__\main.lc")
