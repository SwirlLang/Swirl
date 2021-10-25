import strutils, strformat, tlib, os, httpclient

let
    blue = rgb(11, 121, 255)
    red = rgb(255, 77, 64)
    white = rgb(255,255,255)

let help = &"""
Welcome to LPM, the LambdaCode Package Manager

                                AVAILABLE COMMANDS

            {blue}install{def()} {white}<package-name>{def()}      - Install the provided package
            {blue}remove{def()}  {white}<package-name>{def()}      - Remove the given package
            {blue}query{def()}   {white}<search>{def()}            - Searches threw the database
            {blue}list{def()}                        - List all installed packages along with their version

"""

proc show_help()=
    stdout.write help

proc error(arg: string) =
    stdout.write &"{red}[ERROR]{def()} {arg}"
    quit(1)

if os.paramCount() == 0:
    show_help()
    error "Missing command"

for i in 1..paramCount():
    let current_param = paramStr(i)
    case current_param
        of "-h", "--help":
            show_help()
            quit(0)
        
        of "install", "-i":
            var packages: seq[string]
            for arg in 2..paramCount(): packages.add(paramStr(arg))
            # Packages name are now in memory, need to install them now
        
        of "remove", "-r":
            for package in 2..paramCount():
                when defined(windows):
                    if dirExists(&"{os.getEnv(\"APPDATA\")}\\roaming\\lcm\\{paramStr(package)}"):
                        # Need to remove the directory and its files now
                        discard
                    # Needs error handling in case no package is found
                else:
                    # Need full unix support
                    echo paramStr(package)


        