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

## Functions

```
func <name>([...]): <type> {
    // code
}
```

## Builtin functions

| Function | Description                                                                              |
| -------- | ---------------------------------------------------------------------------------------- |
| print    | Output the string provided.                                                              |
| input    | Read a string from the standard input.                                                   |
| range    | Return a list of integers from start to end, with step.                                  |
| find     | Return the index of the first occurrence of a substring provided in a string.            |
| count    | Return the number of non-overlapping occurrences of a substring provided in a string.    |
| findall  | Return a list of all occurrences of a substring provided in a string.                    |
| string   | Return a string consisting of the characters in the specified sequence.                  |
| int      | Convert the argument to an integer.                                                      |
| float    | Convert the argument to a floating-point number.                                         |
| bool     | Convert the argument to a Boolean value.                                                 |
| len      | Return the length of the string argument.                                                |
| exit     | Exit the program with the provided exit code.                                            |

## Data types

| Type   | About                     |
| ------ | --------------------------|
| string | Instance of Swirl::String |
| int    | Primitive type (keyword)  |
| float  | Primitive type (keyword)  |
| bool   | Primitive type (keyword)  | 

## Type Qualifiers

`const` ~ marks the variable as constant  
`var` ~ makes the compiler checks for the type itself

## Conditional and Loop statements

```
if <condition> {
    // code
}
```

```
elif <condition> {
    // code
}
```

```
else {
    // code
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
