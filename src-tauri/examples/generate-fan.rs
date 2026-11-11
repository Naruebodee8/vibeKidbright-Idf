// Offline reference generator used by integration tests. No file/network writes.
#[path = "../src/fan_recipe.rs"]
mod fan_recipe;
#[path = "../src/simulation_compiler.rs"]
mod simulation_compiler;
#[path = "../src/simulation_validation.rs"]
mod simulation_validation;
use std::io::Read;
fn main() {
    let mut input = String::new();
    std::io::stdin().read_to_string(&mut input).unwrap();
    let port: fan_recipe::FanPort = serde_json::from_str(input.trim()).unwrap();
    let program = fan_recipe::program(port);
    let source = simulation_compiler::compile(&program.to_string()).unwrap();
    println!(
        "{}",
        serde_json::json!({"firmware":fan_recipe::firmware(port),"program":program,"source":source})
    );
}
