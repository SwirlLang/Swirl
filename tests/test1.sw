import dir::mod;
import mod::{another_function as af};

fn main(): i32 {
    mod::sum(32, 53);
    af();
    return 0;
}