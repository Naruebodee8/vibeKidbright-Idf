//! Run on a real Mac: installs the product's native SDK and compiles firmware.
#![allow(dead_code)]
#[path="../src/macos_idf.rs"] mod macos_idf;
#[path="../src/device_program.rs"] mod device_program;
#[path="../src/simulation_compiler.rs"] mod simulation_compiler;
#[path="../src/simulation_validation.rs"] mod simulation_validation;

#[cfg(not(target_os="macos"))]
fn main() { eprintln!("This integration test requires a real macOS host."); std::process::exit(2); }

#[cfg(target_os="macos")]
fn main() {
    if let Err(error)=verify() {eprintln!("macOS ESP-IDF integration failed: {error}");std::process::exit(1);}
}

#[cfg(target_os="macos")]
fn verify()->Result<(),String> {
    use std::{path::PathBuf,sync::atomic::AtomicBool,time::Duration};
    let args:Vec<_>=std::env::args_os().collect();
    let root=PathBuf::from(args.get(1).ok_or("Supply SDK root and a NEW fixture directory")?);
    let projects=PathBuf::from(args.get(2).ok_or("Supply fixture directory")?);
    macos_idf::check_path(&projects,"Smoke-test project directory")?;
    std::fs::create_dir(&projects).map_err(|e|format!("Use a new fixture directory: {e}"))?;
    let cancel=AtomicBool::new(false);
    let sdk=macos_idf::install(&root,&cancel,&|pct,line|println!("[SDK {pct}%] {line}"))?;
    let ready=macos_idf::validate(&sdk,&cancel)?;
    let outputs=vec![
        ("matrix_buttons_buzzer",serde_json::json!({"version":1,"steps":[{"op":"repeat","count":2,"body":[{"op":"output","port":"usb","value":{"kind":"sensor","name":"s1"}},{"op":"number","value":{"kind":"sensor","name":"temperature"}},{"op":"tone","hz":1000,"ms":100},{"op":"scroll","text":"Hello e AZ","ms":80}]}]}),device_program::Hardware::default()),
        ("analog_adc",serde_json::json!({"version":1,"steps":[{"op":"repeat","count":2,"body":[{"op":"number","value":{"kind":"sensor","name":"analog1"}},{"op":"output","port":"out1","value":{"kind":"sensor","name":"input2"}},{"op":"delay","ms":100}]}]}),device_program::Hardware{profile:"kidbright32_v15_analog_bypass".into(),..Default::default()}),
    ];
    for (name,program,hardware) in outputs {
        let bundle=device_program::compile(&program.to_string(),hardware)?;
        let project=projects.join(name);std::fs::create_dir_all(project.join("main")).map_err(|e|e.to_string())?;
        let cmake=format!("cmake_minimum_required(VERSION 3.16)\ninclude($ENV{{IDF_PATH}}/tools/cmake/project.cmake)\nproject({name})\n");
        std::fs::write(project.join("CMakeLists.txt"),cmake).map_err(|e|e.to_string())?;
        std::fs::write(project.join("main/CMakeLists.txt"),"idf_component_register(SRCS \"main.c\" INCLUDE_DIRS \".\")\n").map_err(|e|e.to_string())?;
        std::fs::write(project.join("main/main.c"),bundle.firmware).map_err(|e|e.to_string())?;
        std::fs::create_dir(project.join("simulation")).map_err(|e|e.to_string())?;
        std::fs::write(project.join("simulation/main.sim.js"),bundle.source).map_err(|e|e.to_string())?;
        let cmd=macos_idf::build_command(&sdk,&ready,&project,&["build".into()])?;
        macos_idf::run(cmd,name,&cancel,Duration::from_secs(1800),&|line|println!("[{name}] {line}"))?;
        if !project.join(format!("build/{name}.bin")).is_file(){return Err(format!("Missing {name}.bin"));}
    }
    let result=serde_json::json!({"host":std::env::consts::OS,"arch":std::env::consts::ARCH,"idf":macos_idf::VERSION,"native_installer":true,"python_dependencies_checked":true,"projects_built":["matrix_buttons_buzzer","analog_adc"],"firmware_bins_verified":true,"flashed":false});
    std::fs::write(projects.join("verification.json"),serde_json::to_string_pretty(&result).unwrap()).map_err(|e|e.to_string())?;
    println!("{result}");Ok(())
}
