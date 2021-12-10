import re


def pre_process(source_: str, flags: str = None) -> None:
    """
    Deals with statements that need to be
    handles right after the compiler started
    :return: None
    """
    source_ = open(source_).read() if flags != "string" else source_

    "Dealing with imports"
    i_pattern = r'#[\s+]?import[\s+]*[\S+]+'
    import_find_results = re.findall(i_pattern, source_)
    print(import_find_results)


t = '''
# import stuffs
# import stuffs.moreStuffs.MORE_STUFFS
'''

pre_process(t, 'string')
