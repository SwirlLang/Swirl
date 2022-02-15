def transpile(file: str) -> None:
    patterns = {
        "class": r"^class[\s+]+[\S+]+[\s+]+[inherits+]*[\s+]+[\S+]+(, [\S+]+)+[\s+]*$",
        "var": r"[\S+]+[\s+]]"
    }

    with open(file, 'r') as target:
        l_target = target.readlines()
