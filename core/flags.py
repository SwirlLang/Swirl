from enum import Enum


class Flags(Enum):
    NONE = 0,
    DEBUG = "debug",
    STRING = "string",
    RETURN_STATUS = "return-status",
    VERBOSE = "verbose"
    SPLIT_WITH_ENDL = "\n"
