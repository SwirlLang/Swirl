""" CLI for the package manager """

import os
import sys
import logging
import urllib3
import pathlib

err = """
packy install <package-name>
                  ^^^^ You forget to specify the name of the package that you want to install
"""


Help = '''
Welcome to Packy, the package manager of Lambda-Code

install ~ installs the package specified, ex: packy install foo    
uninstall ~ uninstalls the package specified, ex packy uninstall foo
'''


if sys.argv.__len__() > 2:
    print(sys.argv[1], sys.argv[2])

elif sys.argv.__len__() == 2:
    logging.error(err)

else:
    sys.stdout.write(Help)


def install_package(name: str, url: str) -> None:
    """ Installs the package name specified """
    try:
        urllib3.request.urlretrive(url, f"{pathlib.Path.home()}LambdaCode/{name}")
    except Exception as anonymousError:
        sys.stdout.write(f"{anonymousError}")


def uninstall_package(name: str):
    os.remove(f"{pathlib.Path.home()}packages/{name}")

