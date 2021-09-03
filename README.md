## import statement
` #import "filename.lc" `

## comments
```
** Comment
*** multi  
line
Comment ***
```
## defining functions
```
func name(arg1, arg2)
  print(arg1, arg2)
  return var
endfunc

arrowfunc = () =>
  **logic**
  return var
endfunc
```
## data types

string
int16
int32
int64

float16
float32
float64

Array[length, Type]

## if, elseif, for and while statements
```
if code == code2
logic
endif
```
```
elseif code == code2
logic
endif
```
```
for 1 in var
logic
endfor
```
```
while true
logic
endwhile
```

## Operators

== equal <br>
!= not equal <br>
<= lower than or equal <br>
>= greater than or equal <br>
< lower than <br>
> greater than <br>

= assignement <br>

++ increment by 1 <br>
-- increment by -1 <br>

+= <br>
-= <br>
*= <br>

## structure (structs)
```
structure name
  int16 arg1
  string arg2
endstructure
```
