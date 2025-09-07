// local file
import 

// global package
import package::module::symbol; 

import package::module::symbol as stuff;

import package::module::{ sym1, sym2 as other_stuff };

fn main(argc: type, argv: type): i32 {
    var b = [23, 34, 63];
    return b;  // this will be passed by value, it won't simply decay into a pointer
}

var a = 2323232323; // type deduction
const b = "232322211jfj"; // same

var c: i64 = 2323232121432323;  // explicit

if a > c {
    ...
} else { ... }

while a < c {
   ...
}

var a: [i32 | 5];  // explicit size mention is required when not initialized
var c = [43, 56, 622, 56];  // deduced to [i32 | 4]
var d = u32[3, 34, 52, 42];  // deduced to [u32 | 4]
let d = u32[3, 34, 52, 42];  // deduced to [u32 | 4]