import strutils, strformat, tlib, os, httpclient

let
    blue = rgb(11, 121, 255)
    red = rgb(255, 77, 64)
    green = rgb(0, 255, 127 )
    white = rgb(255,255,255)

let help = &"""
Welcome to {green}L{red}P{blue}M{def()}, the {green}LambdaCode{def()} {red}Package{def()} {blue}Manager{def()}

                                AVAILABLE COMMANDS

            {blue}install{def()}, -i {white}<package-name>{def()}      - Install the provided package                           {red}[PARTIALLY IMPLEMENTED]{def()}
            {blue}remove{def()}, -r  {white}<package-name>{def()}      - Remove the given package                               {green}[IMPLEMENTED]{def()}
            {blue}query{def()}, -q   {white}<search>{def()}            - Searches threw the database                            {red}[NOT IMPLEMENTED]{def()}
            {blue}list{def()}, -l                        - List all installed packages along with their version   {green}[IMPLEMENTED]{def()}

"""

proc show_help()=
    stdout.write help

proc error(arg: string) =
    stdout.write &"{red}[ERROR]{def()} {arg}\n"
    quit(1)

proc info(arg: string) =
    stdout.write &"{blue}[INFO]{def()} {arg}\n"

proc success(arg: string) =
    stdout.write &"{green}[SUCCESS]{def()} {arg}\n"

proc package(name: string) =
    stdout.write &"{green}[INSTALLED]{def()} {name}\n"

if os.paramCount() == 0:
    show_help()
    error "Missing command"

for i in 1..paramCount():
    let current_param = paramStr(i)
    case current_param
        of "-h", "--help":
            show_help()
            quit(0)
        
        of "list", "-l":
            var 
                path: string
                split_char: char
            when defined(windows): path = os.getEnv("APPDATA") & "\\roaming\\lcm\\";split_char = '\\' else: path = os.getEnv("HOME") & "/.lpm/";split_char = '/'
            if dirExists(path):
                var num_of_pkgs: int
                for obj, path in walkDir(path):
                    num_of_pkgs.inc()
                    if obj == pcDir:
                        let splited = path.split(split_char)
                        package splited[len(splited)-1]
                if num_of_pkgs == 0: info "No packages are installed on your system";quit(0)
                info &"{num_of_pkgs} are installed on your system"
                quit(0)
            else:
                info "No packages are installed on your system"
                quit(0)

        of "install", "-i":
            if paramCount() < 2:
                error "No packages provided"
            for arg in 2..paramCount():
                let p = paramStr(arg)
                # just a show
                for i in 0..100:
                    let a = parseInt((($(i/2)).split(".")[0]))
                    stdout.writeLine("Downloading: "& p & "\t"*4, " [",rgb(255-i,i+150,0), "⸺"*a, def(), ' '*(53-a), "]", ' ', i, "%")
                    sleep 40
                    moveCursorUp 1
                rmLine()
                echo "Downloaded: " & p & "\t"*4, " [",rgb(100,250,100),"⸺"*51,def(), "  ]", ' ', &"{rgb(0,255,0)}Done{def()}"
            quit(0)
            # Packages name are now in memory, need to install them now

        of "remove", "-r":
            if paramCount() < 2:
                error "No packages provided"
            for package in 2..paramCount():
                var path: string
                let pkg_name = paramStr(package)

                when defined(windows):
                    path = os.getEnv("APPDATA") & "\\roaming\\lcm\\" & pkg_name
                else:
                    path = os.getEnv("HOME") & "/.lpm/" & pkg_name

                if dirExists(path):
                    os.removeDir(path)
                    success &"Removed {pkg_name} package"

                else:
                    error &"Package {pkg_name} not found !"
            quit(0)
        else:
            show_help()
            error &"Unknow command {current_param}"



        