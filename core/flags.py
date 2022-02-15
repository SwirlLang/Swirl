from enum import Enum


class Flags(Enum):
    NONE: int = 0,
    DEBUG: str = "debug",
    STRING: str = "string",
    RETURN_STATUS: str = "return-status",
    VERBOSE: str = "verbose"
    SPLIT_WITH_ENDL: str = "\n"
