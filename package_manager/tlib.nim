import strformat, os, strutils#, osproc

##[
 # Tlib
 ## The library that provides you with cursor movement, and color management for your terminal
]##

proc moveCursorUp*(n: int)=
    ## Move the cursor up n lines
    stdout.write(&"\e[{n}A")

proc moveCursorDown*(n: int)=
    ## Move the cursor down n lines
    stdout.write(&"\e[{n}B")

proc moveCursorRight*(n: int)=
    ## Move the cursor left n chars
    stdout.write(&"\e[{n}C")

proc moveCursorLeft*(n: int)=
    ## Move the cursor right n chars
    stdout.write(&"\e[{n}D")

proc saveCursorPos*()=
    ## Save the current cursor position, can be restored later
    stdout.write("\e[s")

proc restorCursorPos*()=
    ## Restor previously saved cursor position
    stdout.write("\e[u")

proc rgb*(r:Natural, g:Natural, b:Natural): string=
    ## Return ansi escape code for the provided RGB values
    result = &"\e[38;2;{r};{g};{b}m"

proc read*(args: string): string =
    ## Read input from stdin, same as input in pyhton
    stdout.writeLine(args)
    result = stdin.readline()

proc def*():string =
    ## return reset ansi escape code
    result = "\e[0m"

proc italic*():string =
    ## return italic ansi escape code
    result = "\e[3m"

proc bold*():string =
    ## return bold ansi escape code
    result = "\e[21m"

proc rmLine*()=
    ## Remove the line the cursor is one
    stdout.writeLine("\e[2K")
    moveCursorUp(1)

proc trimRight*()=
    ## Remove everything from the cursor to the end of the line
    stdout.writeLine("\e[K")

proc putCursorAt*(x:int, y:int)=
    ## Put the cursor at x and y coordinates
    stdout.write(&"\e[{y};{x}H")

proc `*`*(str: string, n:Natural): string=
    ## Repeate n times the string str
    for _ in 1..n:
        result &= str

proc `*`*(ch: char, n:Natural): string=
    ## Repeate n times the char ch
    for _ in 1..n:
        result &= ch

proc `**`*(n:int, z:int): int=
    result = n*z

proc `**`*(n:float, z:float): float=
    result = n*z

when defined(windows):
    import windows
    proc readWithoutEcho*(prompt: string): string=
        ## Same as read but doesn't show what is typed, usefull for passwords (windows version)
        var inputHandle = GetStdHandle(STD_INPUT_HANDLE)
        var conMode: DWORD
        discard GetConsoleMode(inputHandle, conMode)
        stdout.write(prompt)
        discard SetConsoleMode(inputHandle, conMode - ENABLE_ECHO_INPUT)
        var input = stdin.readLine()
        discard SetConsoleMode(inputHandle, conMode)
        return input
else:
    
    proc readWithoutEcho*(prompt: cstring) : cstring {.header: "<unistd.h>", importc: "getpass".} ## Same as read but doesn't show what is typed, usefull for passwords (*nix version)

proc clear*()=
    ## Clear the screen
    var cmd: string    
    when defined(windows): cmd = cls else: cmd = "clear"
    discard os.execShellCmd(cmd)

when isMainModule:
    echo "This can't be used by itself"
    quit(1)

when not isMainModule and defined(windows):
    let regi = execCmdEx("reg query HKCU\Console /v VirtualTerminalLevel")[0]
    if "0x1" in regi:
        discard os.execShellCmd("reg add HKCU\Console /v VirtualTerminalLevel /t REG_DWORD /d 00000001")
