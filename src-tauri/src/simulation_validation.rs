//! Basic companion completeness guard, not a proof of semantic equivalence.
//! Full ES5/API syntax validation still runs in the isolated JS runtime.
pub fn require_device_output(source: &str) -> Result<(), String> {
    let mut chars = source.chars().peekable();
    let mut tokens = Vec::<String>::new();
    while let Some(ch) = chars.next() {
        if ch == '/' && chars.peek() == Some(&'/') {
            chars.next();
            for c in chars.by_ref() {
                if c == '\n' {
                    break;
                }
            }
            continue;
        }
        if ch == '/' && chars.peek() == Some(&'*') {
            chars.next();
            let mut previous = ' ';
            for c in chars.by_ref() {
                if previous == '*' && c == '/' {
                    break;
                }
                previous = c;
            }
            continue;
        }
        if ['\'', '"', '\u{0060}'].contains(&ch) {
            let quote = ch;
            let mut escaped = false;
            for c in chars.by_ref() {
                if escaped {
                    escaped = false;
                } else if c == '\\' {
                    escaped = true;
                } else if c == quote {
                    break;
                }
            }
            tokens.push("#literal".into());
            continue;
        }
        if ch.is_ascii_alphabetic() || ch == '_' || ch == '$' {
            let mut name = String::from(ch);
            while chars
                .peek()
                .is_some_and(|c| c.is_ascii_alphanumeric() || *c == '_' || *c == '$')
            {
                name.push(chars.next().unwrap());
            }
            tokens.push(name);
        } else if !ch.is_whitespace() {
            tokens.push(ch.to_string());
        }
    }
    let outputs = [
        ("sound", "tone"),
        ("sound", "note"),
        ("sound", "off"),
        ("ht16k33", "show"),
        ("matrix", "show_number"),
        ("ports", "output1_write"),
        ("ports", "output2_write"),
        ("ports", "usbsw_write"),
    ];
    if tokens.windows(4).any(|w| {
        w[1] == "." && w[3] == "(" && outputs.iter().any(|(o, m)| w[0] == *o && w[2] == *m)
    }) {
        return Ok(());
    }
    Err("ไฟล์ซิมยังไม่มีคำสั่งแสดง LED, buzzer หรือควบคุม OUT/USB มีเพียงการรอเวลาหรืออ่านค่า จึงยังแทนพฤติกรรม firmware ไม่ได้ ให้ใช้ generate_simulation ตาม simulator_compiler.md โดยคงไฟล์ firmware เดิมไว้ และแจ้งกรณีที่ซิมไม่รองรับ".into())
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn rejects_the_reported_placeholder_and_calls_only_in_comments_or_strings() {
        for source in [
            "while(1){vTaskDelay(500);}",
            "// matrix.show_number(1);\nwhile(1){vTaskDelay(500);}",
            "/* ports.output1_write(1); */ var s='ht16k33.show(abc)';",
            "var s=\"ภาษาไทย 😀 matrix.show_number(1);\"; lm73_1.get();",
            "other.output2_write(1);",
        ] {
            assert!(require_device_output(source).is_err(), "{source}");
        }
    }
    #[test]
    fn accepts_outputs_with_spaces_comments_and_unicode_around_them() {
        for source in [
            "// ภาษาไทย\nmatrix.show_number(lm73_1.get());",
            "ports /* x */ . output2_write (1);",
            "ht16k33.show(\"bytes\");",
        ] {
            assert!(require_device_output(source).is_ok());
        }
    }
}

#[cfg(test)]
mod buzzer_output_tests {
    use super::*;
    #[test]
    fn accepts_audio_output_but_not_comment_string_or_volume_only_placeholders() {
        for source in ["sound.tone(1000);", "sound.note(9);sound.rest(2);", "sound.off();"] { assert!(require_device_output(source).is_ok()); }
        for source in ["// sound.note(9);\nvTaskDelay(100);", "var s='sound.tone(1000)';", "sound.set_volume(50);sound.get_volume();"] { assert!(require_device_output(source).is_err()); }
    }
}
