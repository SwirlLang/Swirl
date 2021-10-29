import strutils, strformat, tlib, os, httpclient, asyncdispatch, json, regex

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

            {blue}install{def()}, -i {white}<package-name>{def()}      - Install the provided package
            {blue}remove{def()}, -r  {white}<package-name>{def()}      - Remove the given package
            {blue}query{def()}, -q   {white}<search>{def()}            - Searches threw the database
            {blue}info{def()}, -s    {white}<package-name>{def()}      - List information about a package
            {blue}list{def()}, -l                        - List all installed packages along with their version
            

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

proc query(query: string): Future[string] {.async.} =
    let body = await client.getContent("https://github.com/Lambda-Code-Organization/Lambda-code-Central-repository")
    # regex for packages href="/Lambda-Code-Organization/Lambda-code-Central-repository/tree/main/.*"
    return body


proc info(arg: string) =
    stdout.write &"{blue}[INFO]{def()} {arg}\n"

proc success(arg: string) =
    stdout.write &"{green}[SUCCESS]{def()} {arg}\n"

proc package(name: string) =
    stdout.write &"{green}[INSTALLED]{def()} {name}\n"

proc exit(code: int)=
    chan.close()
    quit(code)

proc pkg_info(pkg: string)=
    var 
        data: JsonNode
        status: string
        url = "https://github.com/Lambda-Code-Organization/Lambda-code-Central-repository/tree/main/" & pkg
    if os.fileExists(path & pkg & "/metadata.json"):
        data = parseJson(readFile(path & pkg & "/metadata.json"))
        status = "INSTALLED"
    else:
        let client = newHttpClient("lpm/v0.1")
        let body = client.getContent(&"https://raw.githubusercontent.com/Lambda-Code-Organization/Lambda-code-Central-repository/main/{pkg}/metadata.json")
        client.close()
        data = body.parseJson()
        status = "NOT INSTALLED"



    echo &"""
            PACKAGE INFORMATIONS
{blue}Name:                 {white}{data["name"].getStr()}{def()}
{blue}Installation Name:    {white}{pkg}{def()}
{blue}Description:          {white}{data["description"].getStr()}{def()}
{blue}Version:              {white}{data["version"].getStr()}{def()}
{blue}Author:               {white}{data["author"].getStr()}{def()}
{blue}License:              {white}{data["license"].getStr()}{def()}
{blue}URL:                  {white}{italic()}{url}{def()}
{blue}LC-version:           {white}{data["lc-version"].getStr()}{def()}
{blue}Updated on:           {white}{data["last_updated"].getStr()}{def()}
{blue}Status:               {white}{status}{def()}
"""


proc get_meta(pkg: string): Future[string] {.async.} =
    let body = await client.getContent(&"https://raw.githubusercontent.com/Lambda-Code-Organization/Lambda-code-Central-repository/main/{pkg}/metadata.json")
    client.close()
    let data = body.parseJson()
    writeFile(&"{path}/{pkg}/metadata.json", body)
    return data["main"].getStr()

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
       
        of "query", "-q":
            if paramCount() < 2:
                error "No query provided"
            let search = paramStr(2)
            let body = waitFor query(search)
            for m in body.findAll(re"""href="/Lambda-Code-Organization/Lambda-code-Central-repository/tree/main/.*""""):
              let htm = (body[m.boundaries]).split('/')
              let pkg = $(htm[len(htm)-1])
              if search in pkg[0..len(pkg)-2]:
                  pkg_info pkg[0..len(pkg)-2]
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
                info &"{num_of_pkgs} pakages are installed on your system"
                exit(0)
            else:
                info "No packages are installed on your system"
                exit(0)
        
        of "info", "-s", "status":
            if paramCount() < 2:
                error "No package(s) provided"
            
            for arg in 2..paramCount():
                let pkg = paramStr(arg)
                if not waitFor packageExists(pkg):
                    error &"Package {pkg} does not exists"
                pkg_info(pkg)
            
            quit(0)

        of "install", "-i":
            if paramCount() < 2:
                error "No package(s) provided"
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
                error "No package(s) provided"
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
