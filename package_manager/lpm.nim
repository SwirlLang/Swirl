#[ 
Copyright (C) 2022 Lambda Code Organization

This file is part of the Lambda Code programming language

Lambda Code is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Lambda Code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>. 
]#

import strutils, strformat, tlib, os, httpclient, asyncdispatch, json, regex

# Setting up some colors to avoid
# calling 'rgb' to much time
let
    blue        = rgb(11, 121, 255)
    red         = rgb(255, 77, 64)
    green       = rgb(0, 255, 127 )
    white       = rgb(255,255,255)
    client      = newAsyncHttpClient("lpm/v0.1")
# Opening a channel to comunicate with
# async procedures
var chan: Channel[string]
chan.open()

let help = &"""
Welcome to {green}L{red}P{blue}M{def()}, {green}Lambdacode{def()} {red}Package{def()} {blue}Manager{def()}

{white}AVAILABLE COMMANDS{def()}

{green}install{def()}, {green}-i{def()} {white}<package-name>{def()}      - Install the provided package
{green}remove{def()},  {green}-r{def()} {white}<package-name>{def()}      - Remove the given package
{green}query{def()},   {green}-q{def()} {white}<search>{def()}            - Searches through the database
{green}info{def()},    {green}-s{def()} {white}<package-name>{def()}      - Show information about the package
{green}list{def()},    {green}-l{def()}                     - List all installed packages along with their version
{green}help{def()},    {green}-h{def()}                     - Show this help message


"""

# Path related
var
    path:               string
    split_char:         char
    cfg_path:           string
    pkg_path_global:    string

# paths assignements
when defined(windows):
    path            = os.getEnv("APPDATA") & "\\local\\lpm\\packages\\"
    cfg_path        = os.getEnv("APPDATA") & "\\local\\lpm\\config.ini"
    split_char      = '\\'
    pkg_path_global = os.getEnv("APPDATA") & "\\roaming\\lpm\\packages"

else:
    path            = os.getEnv("HOME") & "/.lpm/packages/"
    cfg_path        = &"""/home/{os.getenv("USER")}/.config/lpm/config"""
    split_char      = '/'
    pkg_path_global = "/usr/lib/lpm/packages"

# Configs
var
    compiler:   string  = "gpp"
    version :   string  = "0.0.1"

proc show_help()=
    ## Write the help to stdout, kinda useless procedure poluting namespace lol
    stdout.write help

proc error(arg: string) =
    ## Append [ERROR] to what ever string you give and exit with an exit code of 1
    stdout.write &"{red}[ERROR]{def()} {arg}\n"
    quit(1)

proc query(query: string): Future[string] {.async.} =
    ## Return back the content of the github page for the central repo
    let body = await client.getContent("https://github.com/Lambda-Code-Organization/Lambda-code-Central-repository/tree/main/packages")
    # regex for packages href="/Lambda-Code-Organization/Lambda-code-Central-repository/tree/main/packages.*"
    return body


proc info(arg: string) =
    stdout.write &"{blue}[INFO]{def()} {arg}\n"

proc success(arg: string) =
    stdout.write &"{green}[SUCCESS]{def()} {arg}\n"

proc package(name: string) =
    stdout.write &"{green}[INSTALLED]{def()} {name}\n"

proc exit(code: int)=
    ## Close the channel and exit with the given 'code'
    chan.close()
    quit(code)

proc pkg_info(pkg: string, pkg_query: bool)=
    ## Check if a 'pkg' exists on the system, if found return the formated metadata otherwise
    ## check the Centre Repo for it and return it's metadata if found
    var
        data: JsonNode
        status: string
        url = "https://github.com/Lambda-Code-Organization/Lambda-code-Central-repository/tree/main/packages/" & pkg
    if os.fileExists(path & pkg & "/metadata.json"):
        data = parseJson(readFile(path & pkg & "/metadata.json"))
        status = "INSTALLED"
    else:
        let client = newHttpClient("lpm/v0.1")
        let body = client.getContent(&"https://raw.githubusercontent.com/Lambda-Code-Organization/Lambda-code-Central-repository/main/packages/{pkg}/metadata.json")
        client.close()
        data = body.parseJson()
        status = "NOT INSTALLED"
    
    if pkg_query == true:
        echo &"""
            PACKAGE INFORMATIONS
{blue}Name:                 {white}{data["name"].getStr()}{def()}
{blue}Installation Name:    {white}{pkg}{def()}
{blue}Description:          {white}{data["description"].getStr()}{def()}
{blue}Version:              {white}{data["version"].getStr()}{def()}
{blue}Author:               {white}{data["author"].getStr()}{def()}
{blue}Updated on:           {white}{data["last_updated"].getStr()}{def()}
{blue}License:              {white}{data["license"].getStr()}{def()}
"""
    else:
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
    ## Get data from the Central Repo, install it then return it
    let body = await client.getContent(&"https://raw.githubusercontent.com/Lambda-Code-Organization/Lambda-code-Central-repository/main/packages/{pkg}/metadata.json")
    client.close()
    let data = body.parseJson()
    writeFile(&"{path}/{pkg}/metadata.json", body)
    return data["main"].getStr()

proc packageExists(pkg: string): Future[bool] {.async.} =
    ## Check if a package exists by checking the request's return code
    let resp = await client.request(&"https://raw.githubusercontent.com/Lambda-Code-Organization/Lambda-code-Central-repository/main/packages/{pkg}/metadata.json")
    client.close()
    if resp.status != "200 OK": return false else: return true

proc update_bar(total, progress, speed: int64) {.async.}=
    ## Procedure updating the progress bar with progress info
    let percentage = progress * 100 div total
    # Block until we receive a something through the channel
    var pkg = chan.recv()
    let a = percentage div 4
    moveCursorUp 1
    stdout.writeLine("Downloading: " & pkg & "\t"*2, " [",rgb(255-percentage,percentage+150,0), "⸺"*(a+1), def(), ' '*(28-a), "]", ' ', percentage, "%")

proc download_pkg(pkg: string, path: string, main: string) {.async.}=
    ## Write the 'main' file from the repo to the local package
    client.onProgressChanged = update_bar
    writeFile(&"{path}/{pkg}/{main}", await client.getContent(&"https://raw.githubusercontent.com/Lambda-Code-Organization/Lambda-code-Central-repository/main/packages/{pkg}/{main}"))

proc config(path: string, debug:bool) =
    # check if the config file exists
    if not os.fileExists(path): error "No config file found"
    let lines = readFile(path).strip().splitLines()
    for line in lines:
        let cfg = line.split(':')
        case cfg[0].strip()
            of "compiler":
                compiler = cfg[1].strip()
            of "version":
                version = cfg[1].strip()
            else:
                error &"Unknow config command line {lines.find(line)}: {cfg[0]}"
    
    if debug: info &"Using config file at '{path}'"

if os.paramCount() == 0:
    echo &"{green}[USAGE]{def()} lpm <command> [arguments]"
    error &"No command provided\n\nRun {green}help{def()} command for list of commands"

config(cfg_path, false)
var discard_next = false
for i in 1..paramCount():
    let current_param = paramStr(i)
    if discard_next:
        discard_next = false
        continue
    case current_param
        of "--config":
            if paramCount() < i+1:
                error "No path provided"
            config(paramStr(i+1), true)
            discard_next = true

        of "-h", "--help", "help":
            show_help()
            exit(0)

        of "query", "-q":
            if paramCount() < i+1:
                error "No query provided"
            let search = paramStr(i+1)
            let body = waitFor query(search)
            # Match everything starting with href="/Lambda-Code-Organization/Lambda-code-Central-repository/tree/main/packages/
            for m in body.findAll(re"""href="/Lambda-Code-Organization/Lambda-code-Central-repository/tree/main/packages/.*""""):
              # Take the body at current match boundaries, and spliting it on slashes
              let htm = (body[m.boundaries]).split('/')
              # Take the last element of the htm as package name
              let pkg = $(htm[len(htm)-1])
              if search in pkg[0..len(pkg)-2]:
                  # show package informations for package name minux the last chars
                  pkg_info pkg[0..len(pkg)-2], true
            exit(0)

        of "list", "-l":
            if dirExists(path):
                var num_of_pkgs: int
                for obj, path in walkDir(path):
                    num_of_pkgs.inc()
                    if obj == pcDir:
                        let splited = path.split(split_char)
                        package splited[len(splited)-1]
                if num_of_pkgs == 0: info "No packages are installed on your system"
                exit(0)
                info &"{num_of_pkgs} packages are installed on your system"
                exit(0)
            else:
                info "No packages are installed on your system"
                exit(0)

        of "info", "-s", "show":
            if paramCount() < i+1:
                error "No package(s) provided"

            for arg in (i+1)..paramCount():
                let pkg = paramStr(arg)
                if not waitFor packageExists(pkg):
                    error &"Package {pkg} does not exists"
                pkg_info(pkg, false)

            quit(0)

        of "install", "-i":
            if paramCount() < i+1:
                error "No package(s) provided"
            for arg in (i+1)..paramCount():
                let pkg = paramStr(arg)
                # check is 'pkg' exists
                if not waitFor packageExists(pkg):
                    error &"Package {pkg} does not exists"
                # Create the directory for the package
                os.createDir(path & pkg)
                # Install the metadata
                let main  = waitFor get_meta(pkg)
                # Send the pkg name to the channel for the Downloading process to get
                chan.send(pkg)
                waitFor download_pkg(pkg, path, main)
                package pkg
            exit(0)

        of "remove", "-r":
            if paramCount() < i+1:
                error "No package(s) provided"
            for package in (i+1)..paramCount():
                let pkg_name = paramStr(package)
                let pkg_path = path & pkg_name

                if dirExists(pkg_path):
                    os.removeDir(pkg_path)
                    success &"Removed {pkg_name} package"
                else:
                    error &"Package {pkg_name} not found !"
            exit(0)
        else:
            echo &"Run {green}help{def()} command for list of commands"
            error &"Unknown command {current_param}"
