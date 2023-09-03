# The Idea

This file will provide you with an idea of the overall design.

**This is an entirely new design compared to [what was followed](https://github.com/SwirlLang/Swirl/blob/main/old_spec.md) till version 0.0.5. The old design was inconsistent and not very compatible with our overall policy.**

## Variables
The `var` keyword is used for their definitions, the compiler is supposed to be able to infer the type in case it is not specified
(with the obvious condition that the variable must be initialized for its type to be inferred).
```c
// with type
var string_a: str = "hello world!" 

// type can be inferred
var string_b = "hello world!"
```

You can assign function-like scopes to variables, the return value of
that scope will be the assigned value of the variable.

```c
// demo with a quadratic expression
var a = int(input("coefficient of x^2: "))
var b = int(input("coefficient of x:   "))
var c = int(input("the constant c:     "))

// what is the nature of the roots/zeroes/x-intercepts?
var nature_of_root: str = {
    var discriminant = b**2 - 4*a*c
    
    if discriminant < 0
        return "imaginary"
    else
        return "real"
}
```

 the fundamental types are:

| Types | Description           |
|-------|-----------------------|
| bool  | boolean               |
| str   | string                | 
| int   | 64-bit integer        |
| float | 64 bit floating point |


