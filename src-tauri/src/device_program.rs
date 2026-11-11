//! One checked device program -> ESP-IDF C and reference-interpreter JavaScript.
//! Explicit classic reference wiring, not an arbitrary C-to-JS translator.
use crate::simulation_compiler::{Expr, Expression, Program, Step};
use serde::{Deserialize, Serialize};
use serde_json::{json, Value};
use std::collections::BTreeSet;
pub const PREFIX: &str = "// @kidbright-device-program ";
pub const TOOL_DESCRIPTION:&str="Generate BOTH main/main.c ESP-IDF 5.5 classic-reference firmware and matching simulation/main.sim.js from one complete checked program_json (same behavior schema as generate_simulation). Covers matrix bitmap/number/ASCII text/scroll, temperature, calibrated relative light, SW1/SW2, digital IN1..4, raw analog1..4 with explicitly confirmed kidbright32_v15_analog_bypass wiring, distinct OUT1/OUT2/USB, buzzer/tone/note/volume, variables, arithmetic, conditions and loops. Read simulator_paired_programs.md first. Use for a fresh boilerplate or an unchanged owned generated program; existing custom firmware must be preserved using write_file + generate_simulation. Companion generation is automatic for the ESP32/KidBright32 target. Optional hardware_json {profile:classic_reference,lm73_address:77,light_bright:0,light_dark:4095}. Analog firmware requires hardware_json profile=kidbright32_v15_analog_bypass and real confirmation of the unbuffered/bypassed IN path; never select it for an unknown revision or V1.3 digital-buffer board. Raw readings are not moisture percent/gas ppm. Physical board revision/calibration remain unverified. Existing files use Keep/Undo; never claim Build/Flash without actual results.";
#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(default, deny_unknown_fields)]
pub struct Hardware {
    pub profile: String,
    pub lm73_address: u8,
    pub light_bright: u16,
    pub light_dark: u16,
}
impl Default for Hardware {
    fn default() -> Self {
        Self {
            profile: "classic_reference".into(),
            lm73_address: 0x4d,
            light_bright: 0,
            light_dark: 4095,
        }
    }
}
#[derive(Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
struct Description {
    version: u32,
    hardware: Hardware,
    program: Value,
}
pub struct Bundle {
    pub firmware: String,
    pub source: String,
    pub features: Vec<String>,
    pub outputs: Vec<String>,
}
fn sensor_id(name: &str) -> usize {
    [
        "temperature",
        "light",
        "s1",
        "s2",
        "input1",
        "input2",
        "input3",
        "input4",
        "out1",
        "out2",
        "usb",
        "analog1",
        "analog2",
        "analog3",
        "analog4",
    ]
    .iter()
    .position(|value| *value == name)
    .unwrap()
}
fn expr(value: &Expr) -> String {
    match value {
        Expr::Number(n) => format!("kb_num({})", serde_json::to_string(n).unwrap()),
        Expr::Bool(b) => format!("kb_bool({b})"),
        Expr::Node(node) => match node.as_ref() {
            Expression::Var { name } => format!("__kb_v_{name}"),
            Expression::Sensor { name } => match name.as_str() {
                "volume" => "kb_num(kb_volume)".into(),
                "matrix_idle" => "kb_bool(true)".into(),
                "analog1" | "analog2" | "analog3" | "analog4" => format!("kb_analog({})",sensor_id(name)),
                _ => format!("kb_num(kb_hw_read({}))", sensor_id(name)),
            },
            Expression::Unary { operator, value } => format!(
                "kb_unary({}, {})",
                ["!", "~", "+", "-"]
                    .iter()
                    .position(|op| *op == operator)
                    .unwrap(),
                expr(value)
            ),
            Expression::Binary {
                operator,
                left,
                right,
            } => {
                if operator == "&&" || operator == "||" {
                    return format!(
                        "{}({}, {})",
                        if operator == "&&" { "KB_AND" } else { "KB_OR" },
                        expr(left),
                        expr(right)
                    );
                }
                let id = [
                    "+", "-", "*", "/", "%", "<", "<=", ">", ">=", "==", "!=", "===", "!==", "&",
                    "|", "^", "<<", ">>", ">>>",
                ]
                .iter()
                .position(|op| *op == operator)
                .unwrap();
                format!("KB_BINARY({id}, {}, {})", expr(left), expr(right))
            }
        },
    }
}
#[derive(Default)]
struct Resources {
    features: BTreeSet<String>,
    outputs: BTreeSet<String>,
    vars: BTreeSet<String>,
}
impl Resources {
    fn expression(&mut self, value: &Expr) {
        if let Expr::Node(node) = value {
            match node.as_ref() {
                Expression::Sensor { name } => {
                    self.features.insert(name.clone());
                }
                Expression::Binary { left, right, .. } => {
                    self.expression(left);
                    self.expression(right);
                }
                Expression::Unary { value, .. } => self.expression(value),
                _ => {}
            }
        }
    }
    fn steps(&mut self, steps: &[Step]) {
        for step in steps {
            match step {
                Step::Tone { hz, .. } | Step::ToneStart { hz } => {
                    self.features.insert("buzzer".into());
                    self.expression(hz);
                }
                Step::Note { .. }
                | Step::NoteStart { .. }
                | Step::Rest { .. }
                | Step::BuzzerOff => {
                    self.features.insert("buzzer".into());
                }
                Step::Volume { value } => {
                    self.features.insert("buzzer".into());
                    self.expression(value);
                }
                Step::Bitmap { .. } | Step::Text { .. } | Step::Scroll { .. } | Step::Clear => {
                    self.features.insert("matrix".into());
                }
                Step::Char { value } | Step::Number { value } => {
                    self.features.insert("matrix".into());
                    self.expression(value);
                }
                Step::Output { port, value } => {
                    self.features.insert(port.clone());
                    self.outputs.insert(port.clone());
                    self.expression(value);
                }
                Step::Set { name, value } => {
                    self.vars.insert(name.clone());
                    self.expression(value);
                }
                Step::Repeat { body, .. } => self.steps(body),
                Step::While { condition, body } => {
                    self.expression(condition);
                    self.steps(body);
                }
                Step::If {
                    condition,
                    then,
                    r#else,
                } => {
                    self.expression(condition);
                    self.steps(then);
                    self.steps(r#else);
                }
                _ => {}
            }
        }
    }
}
struct C {
    source: String,
    next: usize,
}
impl C {
    fn frames(&mut self, frames: &[[u8; 16]], ms: Option<u32>) {
        let id = self.next;
        self.next += 1;
        let wait = ms.map_or(String::new(), |ms| format!("kb_hw_delay({ms});"));
        let bytes = frames
            .iter()
            .map(|frame| {
                format!(
                    "{{{}}}",
                    frame
                        .iter()
                        .map(u8::to_string)
                        .collect::<Vec<_>>()
                        .join(",")
                )
            })
            .collect::<Vec<_>>()
            .join(",");
        self.source+=&format!("{{static const uint8_t __kb_frames{id}[][16]={{{bytes}}};\nfor(size_t __kb_i{id}=0;__kb_i{id}<{};__kb_i{id}++){{kb_hw_matrix(__kb_frames{id}[__kb_i{id}]);if(!kb_ok)goto __kb_done;{wait}}}}}\n",frames.len());
    }
    fn steps(&mut self, steps: &[Step]) {
        for step in steps {
            self.source += "if(!kb_ok)goto __kb_done;\n";
            match step {
                Step::Tone { hz, ms } => {
                    self.source += &format!(
                        "kb_tone({});if(!kb_ok)goto __kb_done;kb_hw_delay({ms});kb_off();\n",
                        expr(hz)
                    )
                }
                Step::ToneStart { hz } => self.source += &format!("kb_tone({});\n", expr(hz)),
                Step::Note { note, duration } => {
                    self.source += &format!(
                        "kb_tone(kb_num({}));if(!kb_ok)goto __kb_done;kb_hw_delay({});kb_off();\n",
                        NOTES[*note as usize], DURATIONS[*duration as usize]
                    )
                }
                Step::NoteStart { note } => {
                    self.source += &format!("kb_tone(kb_num({}));\n", NOTES[*note as usize])
                }
                Step::Rest { duration } => {
                    self.source +=
                        &format!("kb_hw_delay({});kb_off();\n", DURATIONS[*duration as usize])
                }
                Step::Volume { value } => {
                    self.source += &format!("kb_set_volume({});\n", expr(value))
                }
                Step::BuzzerOff => self.source += "kb_off();\n",
                Step::Clear => self.frames(&[[0; 16]], None),
                Step::MatrixWait => self.source += "kb_hw_delay(0);\n",
                Step::Delay { ms } => self.source += &format!("kb_hw_delay({ms});\n"),
                Step::Bitmap { columns } => {
                    let mut bytes = [0; 16];
                    bytes.copy_from_slice(columns);
                    self.frames(&[bytes], None);
                }
                Step::Number { value } => self.source += &format!("kb_number({});\n", expr(value)),
                Step::Char { value } => self.source += &format!("kb_char({});\n", expr(value)),
                Step::Text { text, ms } => self.frames(
                    &text
                        .bytes()
                        .map(crate::simulation_compiler::glyph)
                        .collect::<Vec<_>>(),
                    Some(*ms),
                ),
                Step::Scroll { text, ms } => {
                    let mut bytes = vec![0; 16];
                    for code in text.bytes() {
                        bytes.extend_from_slice(
                            &crate::simulation_compiler::FONT[usize::from(code - 32)],
                        );
                    }
                    bytes.extend_from_slice(&[0; 16]);
                    let frames = bytes
                        .windows(16)
                        .map(|window| window.try_into().unwrap())
                        .collect::<Vec<[u8; 16]>>();
                    self.frames(&frames, Some(*ms));
                }
                Step::Output { port, value } => {
                    self.source += &format!(
                        "kb_output({}, {});\n",
                        ["out1", "out2", "usb"]
                            .iter()
                            .position(|p| *p == port)
                            .unwrap(),
                        expr(value)
                    )
                }
                Step::Set { name, value } => {
                    self.source += &format!("__kb_v_{name}={};\n", expr(value))
                }
                Step::Repeat { count, body } => {
                    let id = self.next;
                    self.next += 1;
                    self.source+=&count.map_or_else(||"while(kb_ok){\n".into(),|n|format!("for(uint32_t __kb_i{id}=0;__kb_i{id}<{n} && kb_ok;__kb_i{id}++){{\n"));
                    self.steps(body);
                    self.source += "kb_hw_delay(0);\n}\n";
                }
                Step::While { condition, body } => {
                    self.source += &format!("while(kb_ok && kb_truth({})){{\n", expr(condition));
                    self.steps(body);
                    self.source += "kb_hw_delay(0);\n}\n";
                }
                Step::If {
                    condition,
                    then,
                    r#else,
                } => {
                    self.source += &format!("if(kb_truth({})){{\n", expr(condition));
                    self.steps(then);
                    self.source += "}else{\n";
                    self.steps(r#else);
                    self.source += "}\n";
                }
                Step::Break => self.source += "break;\n",
            }
            self.source += "if(!kb_ok)goto __kb_done;\n";
        }
    }
}
const NOTES: [u32; 37] = [
    261, 277, 293, 311, 329, 349, 369, 391, 415, 440, 466, 493, 523, 554, 587, 622, 659, 698, 740,
    784, 831, 880, 932, 988, 1046, 1109, 1175, 1244, 1318, 1396, 1480, 1568, 1661, 1760, 1865,
    1976, 2093,
];
const DURATIONS: [u32; 5] = [2000, 1000, 500, 250, 125];
pub fn compile(program_json: &str, hardware: Hardware) -> Result<Bundle, String> {
    // Validate the SAME typed schema, operators, bounds and unsupported list first.
    let source = crate::simulation_compiler::compile(program_json)?;
    if !["classic_reference","kidbright32_v15_analog_bypass"].contains(&hardware.profile.as_str()) {
        return Err("Paired firmware requires classic_reference or explicitly confirmed kidbright32_v15_analog_bypass ESP32 wiring; preserve custom board drivers and use generate_simulation for other profiles".into());
    }
    if ![0x49, 0x4d].contains(&hardware.lm73_address)
        || hardware.light_bright > 4095
        || hardware.light_dark > 4095
        || hardware.light_bright == hardware.light_dark
    {
        return Err("Invalid LM73 address/light calibration: use reference address 0x49 or 0x4D and distinct ADC endpoints 0..4095".into());
    }
    let program: Program = serde_json::from_str(program_json).map_err(|e| e.to_string())?;
    let mut resources = Resources::default();
    resources.steps(&program.steps);
    let has_analog=(1..=4).any(|n|resources.features.contains(&format!("analog{n}")));
    if has_analog && hardware.profile!="kidbright32_v15_analog_bypass" {
        return Err("Analog firmware requires explicitly confirmed kidbright32_v15_analog_bypass input wiring. V1.3/unknown buffered IN connectors cannot be assumed to read ADC. Confirm board assembly first; the simulator can read analog1..4 without changing existing firmware.".into());
    }
    for n in 1..=4 {
        if resources.features.contains(&format!("analog{n}")) && resources.features.contains(&format!("input{n}")) {
            return Err(format!("IN{n} cannot be independent Analog and Digital inputs on the same physical connector. Use different channels; firmware is preserved."));
        }
    }
    let description = Description {
        version: 1,
        hardware: hardware.clone(),
        program: serde_json::from_str(program_json).map_err(|e| e.to_string())?,
    };
    let mut firmware=format!("{PREFIX}{}\n// Generated C and JS share the checked device behavior. Physical board still needs validation.\n#include <stdint.h>\n#include <stdbool.h>\n#include <stdio.h>\n#include <string.h>\n#include <math.h>\nstatic bool kb_ok=true;\n",serde_json::to_string(&description).map_err(|e|e.to_string())?);
    for (name, feature) in [
        ("MATRIX", "matrix"),
        ("TEMPERATURE", "temperature"),
        ("LIGHT", "light"),
        ("BUZZER", "buzzer"),
        ("S1", "s1"),
        ("S2", "s2"),
        ("INPUT1", "input1"),
        ("INPUT2", "input2"),
        ("INPUT3", "input3"),
        ("INPUT4", "input4"),
    ] {
        firmware += &format!(
            "#define KB_{name} {}\n",
            u8::from(resources.features.contains(feature))
        );
    }
    for (name, feature) in [("OUT1", "out1"), ("OUT2", "out2"), ("USB", "usb")] {
        firmware += &format!(
            "#define KB_{name} {}\n",
            u8::from(resources.outputs.contains(feature))
        );
    }
    firmware += &format!(
        "#define KB_LM73_ADDRESS {}\n#define KB_LIGHT_BRIGHT {}\n#define KB_LIGHT_DARK {}\n",
        hardware.lm73_address, hardware.light_bright, hardware.light_dark
    );
    if has_analog {
        for n in 1..=4 {firmware+=&format!("#define KB_ANALOG{n} {}\n",u8::from(resources.features.contains(&format!("analog{n}"))));}
    }
    firmware += "#ifdef KB_HOST_TEST\n#include \"kb_host_test.h\"\n#else\n";
    firmware += &if has_analog { include_str!("device_program_analog_idf.c") } else { include_str!("device_program_idf.c") }.replace("\r\n", "\n");
    firmware += "\n#endif\n";
    firmware += "static const uint8_t kb_font[95][6]={";
    firmware += &crate::simulation_compiler::FONT
        .iter()
        .map(|bytes| {
            format!(
                "{{{}}}",
                bytes
                    .iter()
                    .map(u8::to_string)
                    .collect::<Vec<_>>()
                    .join(",")
            )
        })
        .collect::<Vec<_>>()
        .join(",");
    firmware += "};\n";
    firmware += &include_str!("device_program_core.c").replace("\r\n", "\n");
    if has_analog {
        firmware += "\nstatic inline kb_value kb_analog(unsigned id){double raw=kb_hw_read(id);if(!isfinite(raw)||raw<0||raw>4095){kb_ok=false;kb_hw_error(\"Invalid raw analog input (0..4095)\");return kb_num(NAN);}return kb_num(trunc(raw));}\n";
    }
    firmware += "\nvoid app_main(void){\nkb_hw_init();if(!kb_ok)goto __kb_done;\n";
    for name in &resources.vars {
        firmware += &format!("kb_value __kb_v_{name}=kb_num(0);(void)__kb_v_{name};\n");
    }
    let mut compiler = C {
        source: String::new(),
        next: 0,
    };
    compiler.steps(&program.steps);
    firmware += &compiler.source;
    firmware += "__kb_done:\nkb_hw_finish(!kb_ok);\n}\n";
    if firmware.len() > 240 * 1024 {
        return Err("Generated firmware exceeds 240 KiB; simplify the behavior".into());
    }
    Ok(Bundle {
        firmware,
        source,
        features: resources.features.into_iter().collect(),
        outputs: resources.outputs.into_iter().collect(),
    })
}
pub fn decode(source: &str) -> Result<Option<Bundle>, String> {
    let Some(header) = source
        .lines()
        .next()
        .and_then(|line| line.strip_prefix(PREFIX))
    else {
        return Ok(None);
    };
    let description: Description =
        serde_json::from_str(header).map_err(|e| format!("Invalid owned device program: {e}"))?;
    if description.version != 1 {
        return Err("Unsupported owned device program version".into());
    }
    compile(&description.program.to_string(), description.hardware).map(Some)
}
pub fn boilerplate(source: &str) -> bool {
    let normalized = source.split_whitespace().collect::<String>();
    source.trim().is_empty() || normalized == "#include<stdio.h>voidapp_main(void){printf(\"Hello\\n\");}"
}
pub fn can_replace(source: &str) -> Result<(), String> {
    if boilerplate(source) {
        return Ok(());
    }
    if let Some(bundle) = decode(source)? {
        if source.replace("\r\n", "\n") == bundle.firmware {
            return Ok(());
        }
    }
    Err("Existing custom/edited firmware must be preserved. Read it and use write_file + generate_simulation instead of replacing it with a paired template. For a new program, create a fresh project first.".into())
}
pub fn result(bundle: &Bundle) -> Value {
    json!({"features":bundle.features,"outputs":bundle.outputs,"validation":"shared_behavior","build_checked":false,"physical_board_checked":false,"profile":"classic_reference"})
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn validates_before_generation_and_does_not_replace_custom_code() {
        assert!(compile(
            r#"{"version":1,"steps":[{"op":"delay","ms":5}]}"#,
            Hardware::default()
        )
        .is_err());
        assert!(compile(
            r#"{"version":1,"steps":[{"op":"tone","hz":0,"ms":1}]}"#,
            Hardware::default()
        )
        .is_err());
        assert!(can_replace("void app_main(void){ /* important custom logic */ }").is_err());
        can_replace("#include <stdio.h>\nvoid app_main(void) { printf(\"Hello\\n\"); }\n").unwrap();
        let bundle=compile(r#"{"version":1,"steps":[{"op":"number","value":{"kind":"sensor","name":"temperature"}}]}"#,Hardware::default()).unwrap();
        assert_eq!(bundle.features, ["matrix", "temperature"]);
        can_replace(&bundle.firmware).unwrap();
        let described = describe(&bundle.firmware).unwrap();
        assert_eq!(
            described["program"],
            json!({"version":1,"steps":[{"op":"number","value":{"kind":"sensor","name":"temperature"}}]})
        );
        assert!(describe(&(bundle.firmware.clone() + "\n// manual edit\n")).is_err());
        assert!(can_replace(&(bundle.firmware + "\n// a user edit\n")).is_err());
    }
    #[test]
    fn preserves_strict_types_short_circuit_and_defined_shifts() {
        let p = r#"{"version":1,"steps":[{"op":"output","port":"out1","value":{"kind":"binary","operator":"===","left":true,"right":1}},{"op":"number","value":{"kind":"binary","operator":">>>","left":-1,"right":31}}]}"#;
        let bundle = compile(p, Hardware::default()).unwrap();
        assert!(bundle
            .firmware
            .contains("KB_BINARY(11, kb_bool(true), kb_num(1.0))"));
        assert!(bundle.firmware.contains("KB_BINARY(18,"));
        assert!(compile(
            p,
            Hardware {
                profile: "unknown_board".into(),
                ..Default::default()
            }
        )
        .is_err());
        assert!(compile(
            p,
            Hardware {
                light_dark: 0,
                ..Default::default()
            }
        )
        .is_err());
    }
}

#[cfg(test)]
mod coverage_tests {
    #[test]
    fn all_device_cases_use_the_same_compiler_and_cover_supported_resources() {
        let cases: serde_json::Value = serde_json::from_str(include_str!(
            "../../simulator-tests/device-program-cases.json"
        ))
        .unwrap();
        let mut covered = std::collections::BTreeSet::new();
        for case in cases.as_array().unwrap() {
            let hardware=case.get("hardware").map(|value|serde_json::from_value(value.clone()).unwrap()).unwrap_or_default();
            let bundle = super::compile(&case["program"].to_string(), hardware).unwrap();
            assert_eq!(
                super::decode(&bundle.firmware).unwrap().unwrap().source,
                bundle.source
            );
            covered.extend(bundle.features);
        }
        for feature in [
            "matrix",
            "temperature",
            "light",
            "s1",
            "s2",
            "input1",
            "input2",
            "input3",
            "input4",
            "out1",
            "out2",
            "usb",
            "buzzer",
            "volume",
        ] {
            assert!(covered.contains(feature), "Missing coverage for {feature}");
        }
        for unsupported in [
            "Wi-Fi",
            "Servo/PWM motor speed",
            "FreeRTOS parallel tasks",
            "analog IN1",
        ] {
            let json = serde_json::json!({"version":1,"steps":[{"op":"output","port":"usb","value":1}],"unsupported":[unsupported]});
            assert!(super::compile(&json.to_string(), Default::default()).is_err());
        }
    }
}

/// Read the editable behavior instead of forcing the model to repeat driver/font code.
pub fn describe(source: &str) -> Result<Value, String> {
    let header=source.lines().next().and_then(|line|line.strip_prefix(PREFIX)).ok_or("This is custom firmware: read_file and preserve it; there is no owned device program to replace")?;
    let description: Description =
        serde_json::from_str(header).map_err(|_| "Invalid owned device program")?;
    let bundle = decode(source)?.ok_or("No owned device program")?;
    if source.replace("\r\n", "\n") != bundle.firmware {
        return Err("Firmware was manually edited. Preserve those edits and inspect the full source before choosing the custom workflow; do not replace it from the old behavior description".into());
    }
    Ok(
        json!({"program":description.program,"hardware":description.hardware,"features":bundle.features,"build_checked":false}),
    )
}

#[cfg(test)]
mod analog_hardware_tests {
    use super::*;
    #[test]
    fn analog_needs_confirmed_wiring_and_conflicting_channels_are_rejected() {
        let p=r#"{"version":1,"steps":[{"op":"number","value":{"kind":"sensor","name":"analog1"}}]}"#;
        assert!(compile(p,Hardware::default()).err().unwrap().contains("confirmed"));
        let hardware=Hardware{profile:"kidbright32_v15_analog_bypass".into(),..Default::default()};
        let bundle=compile(p,hardware.clone()).unwrap();
        assert!(bundle.firmware.contains("ADC_CHANNEL_4"));assert!(bundle.source.contains("kb_specio.input1_read()"));
        assert!(bundle.features.contains(&"analog1".into()));
        assert!(decode(&bundle.firmware).unwrap().is_some());
        let conflict=r#"{"version":1,"steps":[{"op":"number","value":{"kind":"sensor","name":"analog1"}},{"op":"number","value":{"kind":"sensor","name":"input1"}}]}"#;
        assert!(compile(conflict,hardware).err().unwrap().contains("same physical connector"));
    }
}

#[cfg(test)]
mod legacy_template_tests {
    use sha2::{Digest,Sha256};
    #[test]
    fn existing_programs_keep_identical_firmware_and_companions_after_adding_analog() {
        let cases:serde_json::Value=serde_json::from_str(include_str!("../../simulator-tests/device-program-cases.json")).unwrap();
        let golden:serde_json::Value=serde_json::from_str(include_str!("../../simulator-tests/device-program-legacy-sha256.json")).unwrap();
        for expected in golden.as_array().unwrap(){
            let case=cases.as_array().unwrap().iter().find(|case|case["name"]==expected["name"]).unwrap();
            let bundle=super::compile(&case["program"].to_string(),Default::default()).unwrap();
            assert_eq!(format!("{:x}",Sha256::digest(bundle.firmware.as_bytes())),expected["firmware_sha256"].as_str().unwrap(),"{} firmware",case["name"]);
            assert_eq!(format!("{:x}",Sha256::digest(bundle.source.as_bytes())),expected["source_sha256"].as_str().unwrap(),"{} companion",case["name"]);
        }
    }
}
