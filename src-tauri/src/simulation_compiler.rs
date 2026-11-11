//! Device-behavior IR -> the reference interpreter's JavaScript.
//! This is not a C/C++ parser. Compilation is pure and cannot modify firmware.
use serde::Deserialize;
use std::collections::BTreeSet;
include!("simulation_font.rs");

pub const TOOL_DESCRIPTION: &str = "Create ONLY simulation/main.sim.js from checked device behavior, preserving firmware. Read simulator_compiler.md first. Pass program_json: JSON {version:1,steps:[...],unsupported:[]}. Supported op: tone_start{hz:expression 20..20000,nonblocking}, note_start{note:0..36,nonblocking}, rest{duration:0..4,hold then stop}, clear{}, matrix_wait{}, tone{hz:expression,ms:1..60000}, note{note:0..36,duration:0..4 at 120BPM}, volume{value:expression 0..100}, buzzer_off{}, delay{ms}, bitmap{columns:16 bytes}, number{value:expression}, char{value:ASCII code expression}, text{text,ms}, scroll{text,ms}, output{port:out1/out2/usb,value:expression}, set{name,value}, repeat{count:integer or null,body}, while{condition,body}, if{condition,then,else}, break. Expressions: numeric/bool literals, {kind:var,name}, {kind:sensor,name:temperature/light/s1/s2/input1..4 (digital)/analog1..4 (raw 0..4095)/out1/out2/usb/volume/matrix_idle}, {kind:binary,operator,left,right}, {kind:unary,operator,value}. Do not omit unsupported firmware behavior; list it in unsupported to report an error instead of claiming coverage. Firmware must be written before generating a companion.";


// Shared by both provider declarations and both generation tools. Keep the
// serialized-string contract explicit; provider schemas cannot inspect its JSON.
pub const PROGRAM_JSON_DESCRIPTION: &str = r#"A STRING containing the complete JSON object (not JS or C): {"version":1,"steps":[...],"unsupported":[]}. EVERY statement object, including every element inside body/then/else, must contain "op". Sensor/variable/arithmetic expressions use "kind", never "op". Do not put a bare expression into a steps array; wrap it in e.g. {"op":"number","value":expression} or {"op":"set","name":"raw","value":expression}. Supported op: tone, tone_start, note, note_start, rest, volume, buzzer_off, clear, matrix_wait, delay, bitmap, number, char, text, scroll, output, set, repeat, while, if, break. Read simulator_compiler.md for all fields and limits. On a validation error correct the indicated JSON location and retry this tool with the FULL program; do not claim success or silently drop behavior. Example: {"version":1,"steps":[{"op":"repeat","body":[{"op":"number","value":{"kind":"sensor","name":"temperature"}},{"op":"delay","ms":250}]}],"unsupported":[]}"#;

// Diagnose a missing statement discriminator without guessing the intended op.
// The typed parser below remains the authority for all other fields and limits.
fn missing_operation_path(steps: &serde_json::Value, path: &str, depth: usize) -> Option<String> {
    if depth > 12 { return None; }
    for (index, step) in steps.as_array()?.iter().enumerate() {
        let location = format!("{path}[{index}]");
        if step.is_object() && step.get("op").is_none() { return Some(location); }
        for field in ["body", "then", "else"] {
            if let Some(children) = step.get(field) {
                if let Some(found) = missing_operation_path(children, &format!("{location}.{field}"), depth + 1) { return Some(found); }
            }
        }
    }
    None
}

fn program_json_error(input: &str, error: serde_json::Error) -> String {
    let location = serde_json::from_str::<serde_json::Value>(input).ok()
        .and_then(|value| missing_operation_path(&value["steps"], "$.steps", 0));
    let hint = location.map(|path| format!(" At {path}: every statement needs an explicit \"op\"; expressions use \"kind\" inside a statement's value/condition. Do not infer or omit the requested behavior.")).unwrap_or_default();
    format!("Invalid simulation program JSON: {error}.{hint} Read simulator_compiler.md, correct program_json and retry the same tool with the complete program.")
}

#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
pub(crate) struct Program {
    pub(crate) version: u32,
    pub(crate) steps: Vec<Step>,
    #[serde(default)]
    pub(crate) unsupported: Vec<String>,
}
#[derive(Deserialize)]
#[serde(tag = "op", rename_all = "snake_case", deny_unknown_fields)]
pub(crate) enum Step {
    Tone { hz: Expr, ms: u32 },
    ToneStart { hz: Expr },
    NoteStart { note: u8 },
    Rest { duration: u8 },
    Clear,
    MatrixWait,
    Note { note: u8, duration: u8 },
    Volume { value: Expr },
    BuzzerOff,
    Delay {
        ms: u32,
    },
    Bitmap {
        columns: Vec<u8>,
    },
    Number {
        value: Expr,
    },
    Char {
        value: Expr,
    },
    Text {
        text: String,
        ms: u32,
    },
    Scroll {
        text: String,
        ms: u32,
    },
    Output {
        port: String,
        value: Expr,
    },
    Set {
        name: String,
        value: Expr,
    },
    Repeat {
        count: Option<u32>,
        body: Vec<Step>,
    },
    While {
        condition: Expr,
        body: Vec<Step>,
    },
    If {
        condition: Expr,
        then: Vec<Step>,
        #[serde(default)]
        r#else: Vec<Step>,
    },
    Break,
}
#[derive(Deserialize)]
#[serde(untagged)]
pub(crate) enum Expr {
    Number(f64),
    Bool(bool),
    Node(Box<Expression>),
}
#[derive(Deserialize)]
#[serde(tag = "kind", rename_all = "snake_case", deny_unknown_fields)]
pub(crate) enum Expression {
    Var {
        name: String,
    },
    Sensor {
        name: String,
    },
    Binary {
        operator: String,
        left: Expr,
        right: Expr,
    },
    Unary {
        operator: String,
        value: Expr,
    },
}
fn identifier(name: &str) -> bool {
    !name.is_empty()
        && name.len() <= 48
        && !name.starts_with("__kb_")
        && name
            .bytes()
            .enumerate()
            .all(|(i, b)| b.is_ascii_alphabetic() || b == b'_' || (i > 0 && b.is_ascii_digit()))
        && ![
            "var",
            "let",
            "const",
            "true",
            "false",
            "null",
            "undefined",
            "if",
            "else",
            "while",
            "for",
            "return",
            "break",
            "continue",
            "function",
            "new",
            "this",
            "delete",
            "void",
            "typeof",
            "in",
            "instanceof",
            "switch",
            "case",
            "default",
            "do",
            "try",
            "catch",
            "finally",
            "throw",
            "with",
            "debugger",
            "class",
            "import",
            "export",
            "yield",
            "super",
            "enum",
            "implements",
            "interface",
            "package",
            "private",
            "protected",
            "public",
            "static",
            "await",
            "ports",
            "kb_specio",
            "sound",
            "matrix",
            "ht16k33",
            "ldr",
            "lm73_1",
            "button12",
            "vTaskDelay",
            "portTICK_RATE_MS",
            "window",
            "document",
            "eval",
            "Function",
            "Math",
            "String",
            "Number",
            "Object",
            "Array",
            "Infinity",
            "NaN",
        ]
        .contains(&name)
}
fn gather(
    steps: &[Step],
    vars: &mut BTreeSet<String>,
    depth: usize,
    count: &mut usize,
) -> Result<(), String> {
    if depth > 12 {
        return Err("Simulation nesting exceeds 12 levels".into());
    }
    for step in steps {
        *count += 1;
        if *count > 1024 {
            return Err("Simulation exceeds 1024 operations".into());
        }
        match step {
            Step::Set { name, .. } => {
                if !identifier(name) {
                    return Err(format!("Invalid simulation variable: {name}"));
                }
                vars.insert(name.clone());
            }
            Step::Repeat { body, .. } | Step::While { body, .. } => {
                gather(body, vars, depth + 1, count)?
            }
            Step::If { then, r#else, .. } => {
                gather(then, vars, depth + 1, count)?;
                gather(r#else, vars, depth + 1, count)?;
            }
            _ => {}
        }
    }
    Ok(())
}
fn expression(expr: &Expr, vars: &BTreeSet<String>, depth: usize) -> Result<String, String> {
    if depth > 16 {
        return Err("Simulation expression exceeds 16 levels".into());
    }
    Ok(match expr {
        Expr::Number(n) if n.is_finite() => serde_json::to_string(n).map_err(|e| e.to_string())?,
        Expr::Number(_) => return Err("Simulation numbers must be finite".into()),
        Expr::Bool(b) => b.to_string(),
        Expr::Node(node) => match node.as_ref() {
            Expression::Var { name } => {
                if !vars.contains(name) {
                    return Err(format!("Undefined simulation variable: {name}"));
                }
                name.clone()
            }
            Expression::Sensor { name } => match name.as_str() {
                "volume" => "sound.get_volume()",
                "matrix_idle" => "ht16k33.idle()",
                "temperature" => "lm73_1.get()",
                "light" => "ldr.get()",
                "s1" => "button12.sw1_get()",
                "s2" => "button12.sw2_get()",
                "input1" => "ports.input1_read()",
                "input2" => "ports.input2_read()",
                "input3" => "ports.input3_read()",
                "input4" => "ports.input4_read()",
                "analog1" => "kb_specio.input1_read()",
                "analog2" => "kb_specio.input2_read()",
                "analog3" => "kb_specio.input3_read()",
                "analog4" => "kb_specio.input4_read()",
                "out1" => "ports.output1_read()",
                "out2" => "ports.output2_read()",
                "usb" => "ports.usbsw_read()",
                _ => return Err(format!("Unsupported simulator input: {name}")),
            }
            .into(),
            Expression::Binary {
                operator,
                left,
                right,
            } => {
                if ![
                    "+", "-", "*", "/", "%", "<", "<=", ">", ">=", "==", "!=", "===", "!==", "&&",
                    "||", "&", "|", "^", "<<", ">>", ">>>",
                ]
                .contains(&operator.as_str())
                {
                    return Err(format!("Unsupported binary operator: {operator}"));
                }
                format!(
                    "({} {operator} {})",
                    expression(left, vars, depth + 1)?,
                    expression(right, vars, depth + 1)?
                )
            }
            Expression::Unary { operator, value } => {
                if !["!", "~", "+", "-"].contains(&operator.as_str()) {
                    return Err("Unsupported unary operator".into());
                }
                let operand=expression(value, vars, depth + 1)?;
                // -(-1) must not become the invalid ES5 token --1 (prefix decrement).
                let separator=if ["+","-"].contains(&operator.as_str()) && operand.starts_with(operator.as_str()) {" "} else {""};
                format!("({operator}{separator}{operand})")
            }
        },
    })
}
fn delay(ms: u32) -> Result<(), String> {
    if ms > 60000 {
        Err("Simulation delay must be 0..60000 ms".into())
    } else {
        Ok(())
    }
}
fn bytes_literal(columns: &[u8]) -> String {
    format!(
        "\"{}\"",
        columns
            .iter()
            .map(|b| format!("\\x{b:02x}"))
            .collect::<String>()
    )
}
pub(crate) fn glyph(code: u8) -> [u8; 16] {
    let mut buffer = [0; 16];
    // The original font uses bit zero at the visual bottom, as does ht16k33.show.
    buffer[5..11].copy_from_slice(&FONT[usize::from(code - 32)]);
    buffer
}
fn ascii(text: &str) -> Result<&[u8], String> {
    if text.is_empty() || text.len() > 64 || !text.bytes().all(|b| (32..=126).contains(&b)) {
        return Err("Reference LED font supports 1..64 printable ASCII characters (A-Z, a-z, digits and punctuation); use bitmap for other glyphs".into());
    }
    Ok(text.as_bytes())
}
struct Compiler<'a> {
    vars: &'a BTreeSet<String>,
    source: String,
    actions: usize,
    next: usize,
    needs_font: bool,
}
impl Compiler<'_> {
    fn steps(&mut self, steps: &[Step], loops: usize) -> Result<(), String> {
        for step in steps {
            let expr = |e: &Expr| expression(e, self.vars, 0);
            match step {
                Step::ToneStart {hz} => {
                    if matches!(hz,Expr::Bool(_)) || matches!(hz,Expr::Number(n) if !(20.0..=20000.0).contains(n)){return Err("tone_start requires 20..20000 Hz".into());}
                    self.source+=&format!("sound.tone({});\n",expr(hz)?);self.actions+=1;
                }
                Step::NoteStart {note} => {
                    if *note>36{return Err("note_start requires 0..36".into());}
                    self.source+=&format!("sound.note({note});\n");self.actions+=1;
                }
                Step::Rest {duration} => {
                    if *duration>4{return Err("rest requires duration 0..4".into());}
                    self.source+=&format!("sound.rest({duration});\n");
                }
                Step::Clear => {self.source+=&format!("ht16k33.show({});\n",bytes_literal(&[0;16]));self.actions+=1;}
                Step::MatrixWait => {self.source+="ht16k33.wait_idle();\n";}
                Step::Tone { hz, ms } => {
                    delay(*ms)?;
                    if *ms == 0 || matches!(hz, Expr::Bool(_)) || matches!(hz, Expr::Number(value) if !(20.0..=20000.0).contains(value)) {
                        return Err("tone requires 20..20000 Hz and a duration of 1..60000 ms".into());
                    }
                    self.source += &format!("sound.tone({});\nvTaskDelay({ms});\nsound.off();\n", expr(hz)?);
                    self.actions += 1;
                }
                Step::Note { note, duration } => {
                    if *note > 36 || *duration > 4 { return Err("note requires note 0..36 and duration 0..4 at 120 BPM".into()); }
                    self.source += &format!("sound.note({note});\nsound.rest({duration});\n");
                    self.actions += 1;
                }
                Step::Volume { value } => {
                    if matches!(value, Expr::Bool(_)) || matches!(value, Expr::Number(number) if !(0.0..=100.0).contains(number)) {
                        return Err("volume requires 0..100".into());
                    }
                    self.source += &format!("sound.set_volume({});\n", expr(value)?);
                }
                Step::BuzzerOff => {
                    self.source += "sound.off();\n";
                    self.actions += 1;
                }
                Step::Delay { ms } => {
                    delay(*ms)?;
                    self.source += &format!("vTaskDelay({ms});\n");
                }
                Step::Bitmap { columns } => {
                    if columns.len() != 16 {
                        return Err("bitmap requires exactly 16 column bytes".into());
                    }
                    self.source += &format!("ht16k33.show({});\n", bytes_literal(columns));
                    self.actions += 1;
                }
                Step::Number { value } => {
                    if matches!(value,Expr::Bool(_)) || matches!(value,Expr::Number(n) if (n+0.5).floor() < -999.0 || (n+0.5).floor() > 9999.0) {
                        return Err("number requires a numeric value that rounds to -999..9999".into());
                    }
                    self.source += &format!("matrix.show_number({});\n", expr(value)?);
                    self.actions += 1;
                }
                Step::Char { value } => {
                    if matches!(value,Expr::Bool(_)) || matches!(value,Expr::Number(n) if n.fract()!=0.0 || !(32.0..=126.0).contains(n)) {
                        return Err("char requires a printable ASCII integer 32..126".into());
                    }
                    self.source +=
                        &format!("ht16k33.show(__kb_glyphs[({}) - 32]);\n", expr(value)?);
                    self.needs_font = true;
                    self.actions += 1;
                }
                Step::Text { text, ms } => {
                    delay(*ms)?;
                    for code in ascii(text)? {
                        self.source += &format!(
                            "ht16k33.show({});\nvTaskDelay({ms});\n",
                            bytes_literal(&glyph(*code))
                        );
                    }
                    self.actions += 1;
                }
                Step::Scroll { text, ms } => {
                    delay(*ms)?;
                    let mut columns = vec![0; 16];
                    for code in ascii(text)? {
                        columns.extend_from_slice(&FONT[usize::from(code - 32)]);
                    }
                    columns.extend_from_slice(&[0; 16]);
                    let id = self.next;
                    self.next += 1;
                    self.source += &format!(
                        "var __kb_frames{id} = [{}];\n",
                        columns
                            .windows(16)
                            .map(bytes_literal)
                            .collect::<Vec<_>>()
                            .join(",")
                    );
                    self.source+=&format!("for(var __kb_i{id}=0; __kb_i{id}<__kb_frames{id}.length; __kb_i{id}++) {{ ht16k33.show(__kb_frames{id}[__kb_i{id}]); vTaskDelay({ms}); }}\n");
                    self.actions += 1;
                }
                Step::Output { port, value } => {
                    let api = match port.as_str() {
                        "out1" => "output1",
                        "out2" => "output2",
                        "usb" => "usbsw",
                        _ => return Err(format!("Unsupported simulator output: {port}")),
                    };
                    self.source += &format!("ports.{api}_write({});\n", expr(value)?);
                    self.actions += 1;
                }
                Step::Set { name, value } => {
                    self.source += &format!("{name} = {};\n", expr(value)?)
                }
                Step::Repeat { count, body } => {
                    if count.is_some_and(|n| n == 0 || n > 1_000_000) {
                        return Err("repeat count must be 1..1000000 or null (forever)".into());
                    }
                    let id = self.next;
                    self.next += 1;
                    self.source += &count.map_or_else(
                        || "while(1) {\n".into(),
                        |n| format!("for(var __kb_i{id}=0;__kb_i{id}<{n};__kb_i{id}++) {{\n"),
                    );
                    self.steps(body, loops + 1)?;
                    self.source += "vTaskDelay(0);\n}\n";
                }
                Step::While { condition, body } => {
                    self.source += &format!("while({}) {{\n", expr(condition)?);
                    self.steps(body, loops + 1)?;
                    self.source += "vTaskDelay(0);\n}\n";
                }
                Step::If {
                    condition,
                    then,
                    r#else,
                } => {
                    self.source += &format!("if({}) {{\n", expr(condition)?);
                    self.steps(then, loops)?;
                    self.source += "} else {\n";
                    self.steps(r#else, loops)?;
                    self.source += "}\n";
                }
                Step::Break => {
                    if loops == 0 {
                        return Err("break requires a surrounding repeat/while".into());
                    }
                    self.source += "break;\n";
                }
            }
            if self.source.len() > 54000 {
                return Err("Generated simulation is too large; simplify the program".into());
            }
        }
        Ok(())
    }
}
pub fn compile(json: &str) -> Result<String, String> {
    if json.len() > 128 * 1024 {
        return Err("Simulation description exceeds 128 KiB".into());
    }
    let program: Program = serde_json::from_str(json).map_err(|error| program_json_error(json, error))?;
    if program.version != 1 {
        return Err("Unsupported simulation description version".into());
    }
    if !program.unsupported.is_empty() {
        return Err(format!("Cannot simulate complete firmware behavior: {}. Keep firmware intact and explain these unsupported capabilities.",program.unsupported.join(", ")));
    }
    let mut vars = BTreeSet::new();
    gather(&program.steps, &mut vars, 0, &mut 0)?;
    let mut compiler = Compiler {
        vars: &vars,
        source: String::new(),
        actions: 0,
        next: 0,
        needs_font: false,
    };
    compiler.steps(&program.steps, 0)?;
    if compiler.actions == 0 {
        return Err("Simulation has no device output; a delay-only placeholder cannot represent firmware behavior".into());
    }
    let mut header=String::from("// Generated device behavior for the reference JS-Interpreter -> LED/ports -> Unity bridge.\n");
    for name in vars.iter() {
        header += &format!("var {name} = 0;\n");
    }
    if compiler.needs_font {
        header += &format!(
            "var __kb_glyphs = [{}];\n",
            (32..=126)
                .map(|code| bytes_literal(&glyph(code)))
                .collect::<Vec<_>>()
                .join(",")
        );
    }
    header += &compiler.source;
    Ok(header)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn compiles_letters_and_reference_bitmaps_without_host_apis() {
        let source=compile(r#"{"version":1,"steps":[{"op":"repeat","body":[{"op":"text","text":"ABCDEFGHIJKLMNOPQRSTUVWXYZ","ms":500}]}]}"#).unwrap();
        assert_eq!(source.matches("ht16k33.show(").count(), 26);
        assert_eq!(source.matches("vTaskDelay(500)").count(), 26);
        assert!(source.contains("\\x00\\x7e\\x88\\x88\\x88\\x7e"));
        assert!(!source.contains("String.") && !source.contains("function"));
    }
    #[test]
    fn checks_controls_variables_sensors_and_break() {
        let source=compile(r#"{"version":1,"steps":[{"op":"set","name":"n","value":0},{"op":"while","condition":{"kind":"binary","operator":"<","left":{"kind":"var","name":"n"},"right":26},"body":[{"op":"char","value":{"kind":"binary","operator":"+","left":65,"right":{"kind":"var","name":"n"}}},{"op":"if","condition":{"kind":"sensor","name":"s1"},"then":[{"op":"break"}],"else":[{"op":"output","port":"out2","value":{"kind":"unary","operator":"!","value":{"kind":"sensor","name":"input1"}}}]},{"op":"set","name":"n","value":{"kind":"binary","operator":"+","left":{"kind":"var","name":"n"},"right":1}},{"op":"delay","ms":500}]}]}"#).unwrap();
        assert!(source.contains("button12.sw1_get()") && source.contains("ports.input1_read()"));
        assert!(source.contains("vTaskDelay(0)") && source.contains("__kb_glyphs["));
    }
    #[test]
    fn rejects_placeholders_unknown_features_and_invalid_operations() {
        for steps in [
            r#"[{"op":"delay","ms":500}]"#,
            r#"[{"op":"bitmap","columns":[1,2]}]"#,
            r#"[{"op":"output","port":"gpio99","value":1}]"#,
            r#"[{"op":"break"}]"#,
            r#"[{"op":"text","text":"ภาษาไทย","ms":500}]"#,
            r#"[{"op":"set","name":"ports","value":1}]"#,
            r#"[{"op":"number","value":{"kind":"var","name":"unknown"}}]"#,
            r#"[{"op":"delay","ms":60001}]"#,
            r#"[{"op":"run_command","command":"erase-flash"}]"#,
        ] {
            assert!(
                compile(&format!(r#"{{"version":1,"steps":{steps}}}"#)).is_err(),
                "{steps}"
            );
        }
        assert!(
            compile(r#"{"version":1,"steps":[],"unsupported":["Wi-Fi"]}"#)
                .unwrap_err()
                .contains("Wi-Fi")
        );
    }
    #[test]
    fn validates_all_sensor_output_mappings_and_scroll_frames() {
        for name in [
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
        ] {
            for port in ["out1", "out2", "usb"] {
                compile(&format!(r#"{{"version":1,"steps":[{{"op":"output","port":"{port}","value":{{"kind":"sensor","name":"{name}"}}}}]}}"#)).unwrap();
            }
        }
        let source =
            compile(r#"{"version":1,"steps":[{"op":"scroll","text":"HELLO","ms":100}]}"#).unwrap();
        assert!(source.contains("__kb_frames0") && source.contains("vTaskDelay(100)"));
    }
    #[test]
    fn generated_fixtures_match_the_actual_compiler() {
        let cases: serde_json::Value =
            serde_json::from_str(include_str!("../../simulator-tests/compiler-cases.json"))
                .unwrap();
        for case in cases.as_array().unwrap() {
            assert_eq!(
                compile(&case["program"].to_string()).unwrap(),
                case["source"].as_str().unwrap(),
                "{}",
                case["name"]
            );
        }
    }
    #[test]
    fn rejects_nesting_and_injected_operators() {
        assert!(compile(r#"{"version":1,"steps":[{"op":"number","value":{"kind":"binary","operator":";fetch(","left":1,"right":2}}]}"#).is_err());
        let mut step = serde_json::json!({"op":"number","value":1});
        for _ in 0..14 {
            step = serde_json::json!({"op":"repeat","body":[step]});
        }
        assert!(compile(&serde_json::json!({"version":1,"steps":[step]}).to_string()).is_err());
    }
}


#[cfg(test)]
mod buzzer_tests {
    use super::*;
    #[test]
    fn generates_audio_only_programs_and_reference_note_timing() {
        let source=compile(r#"{"version":1,"steps":[{"op":"volume","value":30},{"op":"tone","hz":1000,"ms":200},{"op":"note","note":9,"duration":2},{"op":"buzzer_off"}]}"#).unwrap();
        assert!(source.contains("sound.set_volume(30.0);"));
        assert!(source.contains("sound.tone(1000.0);\nvTaskDelay(200);\nsound.off();"));
        assert!(source.contains("sound.note(9);\nsound.rest(2);"));
    }
    #[test]
    fn rejects_invalid_audio_ranges_placeholders_and_device_name_shadowing() {
        for steps in [
            r#"[{"op":"tone","hz":19,"ms":200}]"#,r#"[{"op":"tone","hz":20001,"ms":200}]"#,
            r#"[{"op":"tone","hz":true,"ms":200}]"#,r#"[{"op":"tone","hz":1000,"ms":0}]"#,
            r#"[{"op":"tone","hz":1000,"ms":60001}]"#,r#"[{"op":"note","note":37,"duration":2}]"#,
            r#"[{"op":"note","note":0,"duration":5}]"#,r#"[{"op":"volume","value":101}]"#,
            r#"[{"op":"volume","value":50}]"#,r#"[{"op":"set","name":"sound","value":1},{"op":"number","value":1}]"#,
        ] { assert!(compile(&format!(r#"{{"version":1,"steps":{steps}}}"#)).is_err(),"{steps}"); }
        for hz in [20,20000] { compile(&format!(r#"{{"version":1,"steps":[{{"op":"tone","hz":{hz},"ms":60000}}]}}"#)).unwrap(); }
    }
    #[test]
    fn supports_button_conditions_and_runtime_checked_frequency_expressions() {
        let source=compile(r#"{"version":1,"steps":[{"op":"repeat","body":[{"op":"if","condition":{"kind":"sensor","name":"s1"},"then":[{"op":"tone","hz":{"kind":"binary","operator":"+","left":500,"right":{"kind":"sensor","name":"temperature"}},"ms":100}]},{"op":"delay","ms":20}]}]}"#).unwrap();
        assert!(source.contains("button12.sw1_get()")&&source.contains("sound.tone((500.0 + lm73_1.get()))"));
    }
}

#[cfg(test)]
mod unary_token_regression {
    #[test]fn negating_negative_literals_does_not_emit_prefix_decrement(){
        let source=super::compile(r#"{"version":1,"steps":[{"op":"number","value":{"kind":"unary","operator":"-","value":-1}}]}"#).unwrap();
        assert!(source.contains("(- -1.0)"));assert!(!source.contains("(--1"));
        for step in [r#"{"op":"char","value":127}"#,r#"{"op":"char","value":65.5}"#,r#"{"op":"number","value":10000}"#,r#"{"op":"number","value":true}"#]{
            assert!(super::compile(&format!("{{\"version\":1,\"steps\":[{step}]}}")).is_err());
        }
    }
}

#[cfg(test)]
mod analog_input_tests {
    use super::*;
    #[test]
    fn all_analog_inputs_use_original_api_without_aliasing_digital_or_light() {
        for n in 1..=4 {
            let program=format!(r#"{{"version":1,"steps":[{{"op":"number","value":{{"kind":"sensor","name":"analog{n}"}}}}]}}"#);
            let source=compile(&program).unwrap();
            assert!(source.contains(&format!("kb_specio.input{n}_read()")));
            assert!(!source.contains("ports.input"));assert!(!source.contains("ldr.get"));
        }
        assert!(compile(r#"{"version":1,"steps":[{"op":"number","value":{"kind":"sensor","name":"analog5"}}]}"#).is_err());
        assert!(compile(r#"{"version":1,"steps":[{"op":"set","name":"kb_specio","value":0},{"op":"number","value":{"kind":"sensor","name":"analog1"}}]}"#).is_err());
    }
}

#[cfg(test)]
mod argument_diagnostic_tests {
    use super::*;
    use serde_json::json;

    #[test]
    fn locates_missing_op_in_every_statement_container_without_guessing_it() {
        for (steps, location) in [
            (json!([{"ms":100}]), "$.steps[0]"),
            (json!([{"op":"repeat","body":[{"op":"number","value":0},{"ms":100}]}]), "$.steps[0].body[1]"),
            (json!([{"op":"while","condition":true,"body":[{"kind":"sensor","name":"analog1"}]}]), "$.steps[0].body[0]"),
            (json!([{"op":"if","condition":true,"then":[{"value":20}]}]), "$.steps[0].then[0]"),
            (json!([{"op":"if","condition":true,"then":[],"else":[{"op":"repeat","body":[{"ms":1}]}]}]), "$.steps[0].else[0].body[0]"),
        ] {
            let input = json!({"version":1,"steps":steps}).to_string();
            let error = compile(&input).err().unwrap();
            assert!(error.contains("missing field `op`") && error.contains(location), "{error}");
            assert!(error.contains("complete program"));
        }
    }

    #[test]
    fn malformed_json_and_excessive_inputs_remain_errors() {
        let error = compile("{broken").unwrap_err();
        assert!(error.contains("Invalid simulation program JSON") && !error.contains("At $.steps"));
        assert!(compile(&" ".repeat(128 * 1024 + 1)).unwrap_err().contains("exceeds 128 KiB"));
        let mut step = json!({"value":1});
        for _ in 0..20 { step = json!({"op":"repeat","body":[step]}); }
        assert!(compile(&json!({"version":1,"steps":[step]}).to_string()).is_err());
    }
}
