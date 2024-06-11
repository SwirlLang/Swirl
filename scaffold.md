# Scaffolding

### Basically like C, but with additions including but not limited to:
- Scopes can return values
- Pattern Matching
- Generics
- Range based loops
- Rust inspired traits like system to allow dynamic dispatch
- Increased type + memory safety (RAII || Reference counting?)
- Modules based "include" system


<hr>

Syntactic sample:
```cpp

// generics, with constraints (if any)
int func<T: constraint1 | constraint2> (T x, T y) {
    return x + y;
};


int main() {
    int var = 12 *  {
        return 12 & 1;
    };  // rogue scopes are akin to functions, var = 0
    

    const str my_string = "hello world";
    
    for (i : 1..12) {
    }
}
```