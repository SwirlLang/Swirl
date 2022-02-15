"""Class indexing and EOL/EOF checking of class definitions"""
with open('../test.lc', 'r') as c_target_file:  # c stands for class, a convention in this context
    t_lines = c_target_file.readlines()  # t: target
    h_cls_index = []
    two_multiples = ['2', '4', '6', '8', '0']
    for c_line in t_lines:
        if 'class' in c_line:
            h_cls_index.append(t_lines.index(c_line) + 1)
    len_cls_index = str(len(h_cls_index))
    print(len_cls_index)
    if len_cls_index in two_multiples: pass
    else:
        raise Exception("Incomplete class definition")
