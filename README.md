# import statement
` #import "filename.lc" `

# comments
```
// Comment
/// multi  
line
Comment ///
```
# defining functions
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
# data types


string name = "rick"

int name = 7 (32bits int)

long int name = 98739823544856 (64bits int)

float name = 0.1

array arr[type,length] = 1,2,2,3,3,4

const string
# Conditional statements and iterations
```
if (code == code2)
  your code
endif
```
```
elseif (code == code2)
  your code
```
```
else
    your code
```
```
for 1 in var
  your code
endfor
```
```
while true
  your code
endwhile
```

## Operators

== equal <br>
!= not equal <br>
<= lower than or equal <br>
\>= greater than or equal <br>
< lower than <br>
\> greater than <br>

= assignment <br>

++ increment by 1 <br>
-- decrement by 1 <br>
+= increment by the number specified<br>
-= decrement by the number specified<br>
*= multiplication by the number specified<br>
/= division by the number specified<br>
### Logical operators
and

or

# Object Oriented Programming
## Classes:
```
class <Name> (params)
  <accessModifier> <type> attr = <default>
endclass
```
So as for simplicity, the parameters of the constructor is defined directly inside the parantheses.

## Encapsulation
### Access Modifiers
-> **Shield:** Can only be accessed within the class

-> **Global:** Can be accesed from anywhere
## Inheritance 
```
class Animal () 
  func eat () 
    print("I am eating some stuff")
  endfunc
endclass

class Dog inherits Animal () 
  func bark () 
    print("Woof woof!")
  endfunc
endclass
```  

We recommend VSCode for Lambda Code programming as it has support for lambda code. Install this extension before working. https://marketplace.visualstudio.com/items?itemName=MrinmoyHaloi.lc-lang-support&ssr=false#overview
