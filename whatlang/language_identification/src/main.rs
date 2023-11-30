use whatlang::{detect};
use std::env;
use std::fs;
fn main() {
    let args: Vec<String> = env::args().collect(); 
    let text = fs::read_to_string(&args[1])
		    .expect("Should have been able to read this file.");
    let info = detect(&text).unwrap();
    println!("{}", info.lang());


}
