import strutils, strformat, tlib, os, httpclient, asyncdispatch, json

let
    blue = rgb(11, 121, 255)
    red = rgb(255, 77, 64)
    green = rgb(0, 255, 127 )
    white = rgb(255,255,255)
    client = newAsyncHttpClient("lpm/v0.1")

var chan: Channel[string]
chan.open()

let help = &"""
Welcome to {green}L{red}P{blue}M{def()}, the {green}LambdaCode{def()} {red}Package{def()} {blue}Manager{def()}

                                AVAILABLE COMMANDS

            {blue}install{def()}, -i {white}<package-name>{def()}      - Install the provided package                           {green}[IMPLEMENTED]{def()}
            {blue}remove{def()}, -r  {white}<package-name>{def()}      - Remove the given package                               {green}[IMPLEMENTED]{def()}
            {blue}query{def()}, -q   {white}<search>{def()}            - Searches threw the database                            {red}[NOT IMPLEMENTED]{def()}
            {blue}list{def()}, -l                        - List all installed packages along with their version   {green}[IMPLEMENTED]{def()}

"""

var 
    path: string
    split_char: char
when defined(windows): path = os.getEnv("APPDATA") & "\\roaming\\lcm\\packages\\";split_char = '\\' else: path = os.getEnv("HOME") & "/.lpm/packages/";split_char = '/'

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

proc exit(code: int)=
    chan.close()
    quit(code)

proc get_meta(pkg: string): Future[string] {.async.} =
# {
#     "name": "Celestial",
#     "version": "1.0.0",
#     "description": "A test package for lambda code",
#     "author": "Mrinmoy Haloi",
#     "license": "MIT",
#     "main": "test.lc",
#     "lc-version": "1.0.0",
#     "dependencies": {
#         "python-api": "1.0.3"
#     },
#     "last_updated": "2019-01-01"
# }
    let body = await client.getContent(&"https://raw.githubusercontent.com/Lambda-Code-Organization/Lambda-code-Central-repository/main/{pkg}/metadata.json")
    let data = body.parseJson()
    let
        pkg_name = data["name"].getStr()
        pkg_ver = data["version"].getStr()
        pkg_desc = data["description"].getStr()
        pkg_author = data["author"].getStr()
        pkg_license = data["license"].getStr()
        pkg_main = data["main"].getStr()
        pkg_lc_ver = data["lc-version"].getStr()
        pkg_update = data["last_updated"].getStr()

    client.close()
    writeFile(&"{path}/{pkg}/metadata.json", body)
    return pkg_main

proc packageExists(pkg: string): Future[bool] {.async.} =
    let resp = await client.request(&"https://raw.githubusercontent.com/Lambda-Code-Organization/Lambda-code-Central-repository/main/{pkg}/metadata.json")
    client.close()
    if resp.status != "200 OK": return false else: return true

proc update_bar(total, progress, speed: int64) {.async.}=
    let percentage = progress * 100 div total
    var pkg = chan.recv()
    echo pkg
    let a = percentage div 4
    moveCursorUp 1
    stdout.writeLine("Downloading: " & pkg & "\t"*2, " [",rgb(255-percentage,percentage+150,0), "⸺"*(a+1), def(), ' '*(28-a), "]", ' ', percentage, "%")

proc download_pkg(pkg: string, path: string, main: string) {.async.}=
    client.onProgressChanged = update_bar
    writeFile(&"{path}/{pkg}/{main}", await client.getContent(&"https://raw.githubusercontent.com/Lambda-Code-Organization/Lambda-code-Central-repository/main/{pkg}/{main}"))

if os.paramCount() == 0:
    show_help()
    error "Missing command"

for i in 1..paramCount():
    let current_param = paramStr(i)
    case current_param
        of "-h", "--help":
            show_help()
            exit(0)
        
        of "list", "-l":
            if dirExists(path):
                var num_of_pkgs: int
                for obj, path in walkDir(path):
                    num_of_pkgs.inc()
                    if obj == pcDir:
                        let splited = path.split(split_char)
                        package splited[len(splited)-1]
                if num_of_pkgs == 0: info "No packages are installed on your system";exit(0)
                info &"{num_of_pkgs} are installed on your system"
                exit(0)
            else:
                info "No packages are installed on your system"
                exit(0)

        of "install", "-i":
            if paramCount() < 2:
                error "No packages provided"
            for arg in 2..paramCount():
                let pkg = paramStr(arg)
                if not waitFor packageExists(pkg):
                    error &"Package {pkg} does not exists"
                os.createDir(path & pkg)
                let main  = waitFor get_meta(pkg)
                chan.send(pkg)
                waitFor download_pkg(pkg, path, main)
                package pkg
            exit(0)

        of "remove", "-r":
            if paramCount() < 2:
                error "No packages provided"
            for package in 2..paramCount():
                let pkg_name = paramStr(package)
                let pkg_path = path & pkg_name

                if dirExists(pkg_path):
                    os.removeDir(pkg_path)
                    success &"Removed {pkg_name} package"
                else:
                    error &"Package {pkg_name} not found !"
            exit(0)
        else:
            show_help()
            error &"Unknow command {current_param}"
