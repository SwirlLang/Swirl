# Syntax of Lambda Code
## import statement
` #import library `

## comments
```
// Comment
/// multi  
line
Comment ///
```
## defining functions
```
func name(int a, int b): int
  print("a + b is ", a + b)
  return a + b
endfunc
```
## data types
string name = "rick"

int name = 7 //32bits int

long int name = 98739823544856 //64bits int

float name = 0.1

array arr[type] = 1,2,2,3,3,4

## Type Qualifiers
const

## Conditional and Loop statements
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
for i in var
  your code
endfor
```
```
while true
  your code
endwhile
```

## Operators

### Comparison Operators
== equal to<br>
!= not equal to<br>
<= lower than or equal to<br>
\>= greater than or equal to<br>
< lower than <br>
\> greater than <br>

### Assignment Operators
= assignment <br>
++ increment by 1 <br>
-- decrement by 1 <br>
+= increment by the number specified<br>
-= decrement by the number specified<br>
*= multiplication by the number specified<br>
/= division by the number specified<br>

### Arithmetic Operators
\+ Addition<br>
\- Subtraction<br>
\* Multiplication<br>
\/ Division<br>
% Modulus

### Bitwise Operators
& Sets each bit to 1 if both bits are 1<br>
| Sets each bit to 1 if one of two bits is 1<br>
^ Sets each bit to 1 if only one of two bits is 1<br>
~ Inverts all the bits<br>
<< Shift left by pushing zeros in from the right and let the leftmost bits fall off<br>
\>> Shift right by pushing copies of the leftmost bit in from the left, and let the rightmost bits fall off

### Logical Operators
and<br>
or

