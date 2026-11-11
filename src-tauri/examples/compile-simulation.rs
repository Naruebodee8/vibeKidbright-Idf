//! Offline verification utility: reads device behavior JSON on stdin, emits JS.
//! It has no filesystem writes, provider calls or firmware commands.
#[path = "../src/simulation_compiler.rs"]
mod simulation_compiler;
fn main() {
    use std::io::Read;
    let mut input = String::new();
    std::io::stdin().read_to_string(&mut input).unwrap();
    match simulation_compiler::compile(&input) {
        Ok(source) => print!("{source}"),
        Err(error) => {
            eprintln!("{error}");
            std::process::exit(1);
        }
    }
}
