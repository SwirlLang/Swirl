# Specification of Swirl

## using an external namespace

```
import package.dir.module  // injects the entire namespace
import package.dir.module.Scope  // only injects the scope being referenced
```

## comments

```
// single line comment

///
multi
line
Comment ///
```

## Example function definition

```
func demo() {
    print("Hello world!")
}
```

## Builtin functions

| Function | Description                                                                              |
| -------- | ---------------------------------------------------------------------------------------- |
| print    | Outputs the string provided to the standard output.                                      |
| input    | Reads a string from the standard input.                                                  |
| range    | Returns a list of integers from start to end, with step.                                 |
| find     | Returns the index of the first occurrence of the substring provided in a string.         |
| count    | Returns the number of non-overlapping occurrences of the substring provided in a string. |
| findall  | Returns a list of all occurrences of the substring provided in a string.                 |
| string   | Returns a string consisting of the characters in the specified sequence.                 |
| int      | Converts the argument to an integer.                                                     |
| float    | Converts the argument to a floating-point number.                                        |
| bool     | Converts the argument to a Boolean value.                                                |
| len      | Returns the length of the string argument.                                               |
| exit     | Exits the program with the exit code provided.                                           |

## Data types

```
string name = "rick"  // char[]
```

```
int name = 7  // dynamic integer blob (arbitrary precision)
```

```
float my_float = 0.1  // dynamic float blob
```

```
list my_cool_list = [1, "2", 3.0]
```

## Type Qualifiers

`const` ~ marks the variable as constant
`var` ~ optional weak typing, makes the compiler check for the type.

## Conditional and Loop statements

```
if code == code2 {
    print("Hello World")
}
```

```
elif code == code2 {
    print("Hello World")
}
```

```
else {
    print("Hello World")
}
```

```
for i in range(10, 50) {
    print(i)
}
```

```
while true {
    print("Hello World")
}
```

## Operators

### Comparison Operators

| Operator | description                  |
| -------- | ---------------------------- |
| ==       | equal to<br>                 |
| !=       | not equal to<br>             |
| <=       | lower than or equal to<br>   |
| \>=      | greater than or equal to<br> |
| <        | lower than <br>              |
| \>       | greater than <br>            |

### Assignment Operators

| Operator | description                                |
| -------- | ------------------------------------------ |
| =        | assignment <br>                            |
| ++       | increment by 1 <br>                        |
| --       | decrement by 1 <br>                        |
| +=       | increment by the number specified<br>      |
| -=       | decrement by the number specified<br>      |
| \*=      | multiplication by the number specified<br> |
| /=       | division by the number specified<br>       |

### Arithmetic Operators

| Operator | description        |
| -------- | ------------------ |
| \+       | Addition<br>       |
| \-       | Subtraction<br>    |
| \*       | Multiplication<br> |
| \/       | Division<br>       |
| %        | Modulus            |

### Bitwise Operators

| Operator | description                                                                                             |
| -------- | ------------------------------------------------------------------------------------------------------- |
| &        | Sets each bit to 1 if both bits are 1<br>                                                               |
| \|       | Sets each bit to 1 if one of two bits is 1<br>                                                          |
| ^        | Sets each bit to 1 if only one of two bits is 1<br>                                                     |
| ~        | Inverts all the bits<br>                                                                                |
| <<       | Shift left by pushing zeros in from the right and let the leftmost bits fall off<br>                    |
| \>>      | Shift right by pushing copies of the leftmost bit in from the left, and let the rightmost bits fall off |

### Logical Operators

| Operator | description |
| -------- | ----------- |
| and      | Logical AND |
| or       | Logical OR  |
| not      | Logical NOT |
