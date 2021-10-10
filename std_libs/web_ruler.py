import sys
import os
import pathlib
import logging

if sys.argv.__len__() > 0:
    try:
        os.mkdir(f"{pathlib.Path.home()}{sys.argv[2]}")
    except Exception as outOfIndex:
        logging.error("[Error]\tYou have not specified the name of your project, web_ruler new <project_name> <path("
                      "optional)>")
