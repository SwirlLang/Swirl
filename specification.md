# Syntax of Lambda Code
## import statement
```
#import library 
```

## comments
```
// Comment
/// multi  
line
Comment ///
```
## Defining functions
```
func name(int a, int b): int
  print("a + b is " + str(a + b))
  return a + b
endfunc
```
## Builtin functions

Function | Description   
--- | ---
print         | Outputs the string provided to the standard output.
input      | Reads a string from the standard input.      
range | Returns a list of integers from start to end, with step.
find | Returns the index of the first occurrence of the substring sub in the string s.
count | Returns the number of non-overlapping occurrences of substring sub in the string s.
findall | Returns a list of all occurrences of the substring sub in the string s.
string | Returns a string consisting of the characters in the specified sequence.
int | Converts the argument to an integer.
float | Converts the argument to a floating-point number.
bool | Converts the argument to a Boolean value.
len | Returns the length of the string argument.
exit | Exits the program with the exit code provided. 

## Data types
```
string name = "rick"
```
```
int name = 7  // 32bits int
```
```
long int name = 98739823544856  // 64bits int
```
```
float name = 0.1
```
```
list name = [1, "2", 3.0]
```
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
Operator | description
--- | ---
== | equal to<br>
!= | not equal to<br>
<= | lower than or equal to<br>
\>= | greater than or equal to<br>
< | lower than <br>
\> | greater than <br>

### Assignment Operators
Operator | description
--- | ---
= |assignment <br>
++ |increment by 1 <br>
-- |decrement by 1 <br>
+= |increment by the number specified<br>
-= |decrement by the number specified<br>
*= |multiplication by the number specified<br>
/= |division by the number specified<br>

### Arithmetic Operators
Operator | description
--- | ---
\+ |Addition<br>
\- |Subtraction<br>
\* |Multiplication<br>
\/ |Division<br>
% |Modulus

### Bitwise Operators
Operator | description
--- | ---
& |Sets each bit to 1 if both bits are 1<br>
| |Sets each bit to 1 if one of two bits is 1<br>
^ |Sets each bit to 1 if only one of two bits is 1<br>
~ |Inverts all the bits<br>
<< |Shift left by pushing zeros in from the right and let the leftmost bits fall off<br>
\>> |Shift right by pushing copies of the leftmost bit in from the left, and let the rightmost bits fall off

### Logical Operators
Operator | description
--- | ---
and | Logical AND
or | Logical OR

