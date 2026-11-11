#![allow(dead_code)]
// Offline compiler probe: reads behavior/configuration JSON, prints artifacts, never writes files.
#[path = "../src/device_program.rs"]
mod device_program;
#[path = "../src/simulation_compiler.rs"]
mod simulation_compiler;
#[path = "../src/simulation_validation.rs"]
mod simulation_validation;
use std::io::Read;
fn main() {
    let mut input = String::new();
    std::io::stdin().read_to_string(&mut input).unwrap();
    let request: serde_json::Value = serde_json::from_str(&input).unwrap();
    let hardware = serde_json::from_value(
        request
            .get("hardware")
            .cloned()
            .unwrap_or_else(|| serde_json::json!({})),
    )
    .unwrap();
    match device_program::compile(&request["program"].to_string(), hardware) {
        Ok(bundle) => println!(
            "{}",
            serde_json::json!({"firmware":bundle.firmware,"source":bundle.source,"features":bundle.features,"outputs":bundle.outputs})
        ),
        Err(error) => {
            eprintln!("{error}");
            std::process::exit(1);
        }
    }
}
