//! Explicit device wiring contract; checks ports, not arbitrary C semantics.
use crate::fan_recipe::FanPort;
use crate::simulation_paths::PathScope;
use serde::{Deserialize, Serialize};
use std::{
    collections::BTreeMap,
    io::Write,
    path::{Path, PathBuf},
};
const FILE: &str = ".vibekidbright/hardware.json";
#[derive(Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
struct Plan {
    version: u8,
    fan: FanPort,
}
#[derive(Clone, Copy, Debug, Default)]
pub(crate) struct Requirements {
    pub fan: Option<FanPort>,
    pub simulation: bool,
    pub classic_board: bool,
}

pub(crate) fn load(project: &Path) -> Result<Option<FanPort>, String> {
    let scope = PathScope::new(project)?;
    let path = scope.resolve(FILE, true)?;
    match std::fs::read_to_string(path) {
        Ok(text) => {
            let plan: Plan =
                serde_json::from_str(&text).map_err(|e| format!("Invalid hardware plan: {e}"))?;
            if plan.version != 1 {
                return Err("Unsupported hardware plan version".into());
            }
            Ok(Some(plan.fan))
        }
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => Ok(None),
        Err(e) => Err(format!("Cannot read hardware plan: {e}")),
    }
}
pub(crate) fn save(project: &Path, fan: FanPort) -> Result<(), String> {
    let scope = PathScope::new(project)?;
    let path = scope.resolve(FILE, true)?;
    if path != scope.root.join(FILE) {
        return Err("Hardware plan cannot be redirected by a link".into());
    }
    let parent = path.parent().ok_or("Invalid hardware plan path")?;
    std::fs::create_dir_all(parent).map_err(|e| e.to_string())?;
    let mut temp = tempfile::NamedTempFile::new_in(parent).map_err(|e| e.to_string())?;
    temp.write_all(
        serde_json::to_string_pretty(&Plan { version: 1, fan })
            .map_err(|e| e.to_string())?
            .as_bytes(),
    )
    .map_err(|e| e.to_string())?;
    temp.as_file().sync_all().map_err(|e| e.to_string())?;
    if scope.resolve(FILE, true)? != path {
        return Err("Hardware plan destination changed".into());
    }
    temp.persist(&path)
        .map_err(|e| format!("Cannot save hardware plan: {}", e.error))?;
    Ok(())
}

pub(crate) fn choose(
    request: &str,
    selected: Option<FanPort>,
    stored: Option<FanPort>,
    fresh: bool,
) -> Result<Option<FanPort>, String> {
    let lower = request.to_lowercase();
    let fan = lower.contains("พัดลม") || regex::Regex::new(r"\bfan\b").unwrap().is_match(&lower);
    if !fan {
        return Ok(if fresh { None } else { stored });
    }
    let mentioned = [
        ("usb", FanPort::Usb),
        ("out1", FanPort::Out1),
        ("out2", FanPort::Out2),
    ]
    .into_iter()
    .filter(|(name, _)| lower.contains(name))
    .map(|(_, port)| port)
    .collect::<Vec<_>>();
    if mentioned.len() == 1 {
        if selected.is_some_and(|port| port != mentioned[0]) {
            return Err("ช่องพัดลมในข้อความไม่ตรงกับข้อมูลอุปกรณ์ที่ส่งมา กรุณาระบุช่องให้ตรงกัน".into());
        }
        return Ok(Some(mentioned[0]));
    }
    if mentioned.len() > 1 {
        return selected
            .map(Some)
            .ok_or_else(|| "คำสั่งกล่าวถึงหลายช่องพัดลม กรุณาระบุในข้อความว่าพัดลมใช้ USB, OUT1 หรือ OUT2".into());
    }
    selected.or(if fresh{None}else{stored}).map(Some).ok_or_else(||"ยังไม่ทราบว่าพัดลมต่อช่องใด กรุณาระบุในข้อความ เช่น พัดลม USB, พัดลม OUT1 หรือพัดลม OUT2 การลากอุปกรณ์ใน Unity ยังไม่ส่งข้อมูลนี้อัตโนมัติ".into())
}
pub(crate) fn instructions(requirements: Requirements) -> String {
    let mut text=String::from("\n[DEVICE WIRING]\nThe reference Unity fan accessory is a USB fan: ports.usbsw_write(), not OUT1. OUT1/OUT2/USB are distinct channels. Unity drag/drop inventory is NOT automatically visible to the model. Never guess a fan port from 'the image'; follow the explicit device plan or ask the user to state USB/OUT1/OUT2 in their message. For classic KidBright, USB_SW=GPIO25, OUT1=26, OUT2=27; read the revision-specific KB for polarity. Never remap OUT1 to USB in the runtime.\n");
    if let Some(port) = requirements.fan {
        text+=&format!("[SELECTED FAN] port={}, GPIO{}, simulator ports.{}(). Prefer generate_fan_control for SW1 toggle, debounce and Fan ON/OFF logs; it generates matching firmware and simulation from a checked recipe. Preserve unrelated requested behavior. Use this recipe only when it covers the user request. Code changes remain subject to Keep/Undo. Port checks do not prove arbitrary C behavior or physical board compatibility. Do not claim Build/Flash passed without an actual successful build result.\n",port.key(),port.gpio(),port.api());
    }
    text
}

fn stripped(text: &str) -> String {
    let re =
        regex::Regex::new(r#"(?s)/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'"#).unwrap();
    re.replace_all(text, " ").into_owned()
}
pub(crate) fn validate_firmware(source: &str, port: FanPort) -> Result<(), String> {
    let code = stripped(source);
    let mut aliases = BTreeMap::new();
    let defines =
        regex::Regex::new(r"(?m)^\s*#\s*define\s+([A-Za-z_]\w*)\s+([A-Za-z_]\w*|\d+)").unwrap();
    for captures in defines.captures_iter(&code) {
        aliases.insert(captures[1].to_string(), captures[2].to_string());
    }
    let constants = regex::Regex::new(
        r"\b(?:gpio_num_t|int|uint32_t)\s+(?:const\s+)?([A-Za-z_]\w*)\s*=\s*([A-Za-z_]\w*|\d+)\s*;",
    )
    .unwrap();
    for captures in constants.captures_iter(&code) {
        aliases.insert(captures[1].to_string(), captures[2].to_string());
    }
    let calls = regex::Regex::new(r"\bgpio_set_level\s*\(\s*([A-Za-z_]\w*|\d+)\s*,").unwrap();
    let mut pins = Vec::new();
    for capture in calls.captures_iter(&code) {
        let mut value = capture[1].to_string();
        for _ in 0..16 {
            if let Some(next) = aliases.get(&value) {
                value = next.clone();
            } else {
                break;
            }
        }
        if let Ok(pin) = value
            .strip_prefix("GPIO_NUM_")
            .unwrap_or(&value)
            .parse::<u8>()
        {
            pins.push(pin);
        }
    }
    if pins.contains(&port.gpio()) {
        Ok(())
    } else {
        Err(format!("ตรวจช่องพัดลมไม่ผ่าน: เลือก {} ต้องพบการเขียน GPIO{} แต่พบ {:?}. ใช้ generate_fan_control หรือแก้ driver ให้ตรง ห้ามอ้างว่าใช้งานได้จนตรวจผ่าน (ตัวตรวจนี้รองรับ direct gpio_set_level และ alias แบบง่าย ไม่ใช่ C/C++ compiler)",port.key(),port.gpio(),pins))
    }
}
pub(crate) fn validate_simulation(source: &str, port: FanPort) -> Result<(), String> {
    let code = stripped(source);
    let re = regex::Regex::new(&format!(r"\bports\s*\.\s*{}\s*\(", port.api())).unwrap();
    if re.is_match(&code) {
        Ok(())
    } else {
        Err(format!(
            "เลือกพัดลม {} แต่ไฟล์ซิมไม่ได้ใช้ ports.{}() ต้องสร้างซิมให้ตรงกับช่องที่เลือก",
            port.key(),
            port.api()
        ))
    }
}
fn effective(
    project: &Path,
    relative: &str,
    proposed: &BTreeMap<PathBuf, String>,
) -> Result<String, String> {
    let scope = PathScope::new(project)?;
    let path = scope.resolve(relative, true)?;
    if let Some(text) = proposed.get(&path) {
        return Ok(text.clone());
    }
    std::fs::read_to_string(path).map_err(|e| format!("Cannot verify {relative}: {e}"))
}
pub(crate) fn check(project: &Path, requirements: Requirements) -> Result<(), String> {
    let Some(port) = requirements.fan else {
        return Ok(());
    };
    let scope = PathScope::new(project)?;
    let proposed = crate::simulation_project::proposed_inputs(&scope)?;
    validate_firmware(&effective(project, "main/main.c", &proposed)?, port)?;
    if requirements.simulation {
        validate_simulation(
            &effective(project, crate::simulation_project::SOURCE_PATH, &proposed)?,
            port,
        )?;
    }
    Ok(())
}

#[tauri::command]
pub fn get_hardware_target(
    window: tauri::WebviewWindow,
    project_dir: String,
) -> Result<Option<FanPort>, String> {
    if window.label() != "main" {
        return Err("Main window required".into());
    }
    if project_dir == "." || project_dir.is_empty() {
        return Ok(None);
    }
    load(Path::new(&project_dir))
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn fan_target_is_explicit_and_does_not_guess_from_an_image() {
        assert_eq!(choose("พัดลม USB",None,Some(FanPort::Out1),false).unwrap(),Some(FanPort::Usb));
        assert!(choose("พัดลม",None,Some(FanPort::Out1),true).is_err());
        assert!(choose("สร้างพัดลมในภาพ", None, None, true).is_err());
        assert_eq!(
            choose("SW1 toggle USB fan", None, None, true).unwrap(),
            Some(FanPort::Usb)
        );
        assert!(choose("USB fan", Some(FanPort::Out1), None, false).is_err());
        assert_eq!(
            choose("แก้พัดลม", None, Some(FanPort::Usb), false).unwrap(),
            Some(FanPort::Usb)
        );
        assert_eq!(
            choose("สร้าง LED", None, Some(FanPort::Usb), true).unwrap(),
            None
        );
    }
    #[test]
    fn wrong_pin_comments_and_placeholder_calls_do_not_pass() {
        let bad = crate::fan_recipe::firmware(FanPort::Out1);
        assert!(validate_firmware(&bad, FanPort::Usb).is_err());
        assert!(validate_firmware(
            "// gpio_set_level(GPIO_NUM_25,0);\nvoid app_main(void){}",
            FanPort::Usb
        )
        .is_err());
        assert!(validate_simulation(
            "// ports.usbsw_write(1);\nports.output1_write(1);",
            FanPort::Usb
        )
        .is_err());
        for port in [FanPort::Usb, FanPort::Out1, FanPort::Out2] {
            validate_firmware(&crate::fan_recipe::firmware(port), port).unwrap();
        }
    }
    #[test]
    fn plan_roundtrips_and_rejects_invalid_data_without_touching_firmware() {
        let root = tempfile::tempdir().unwrap();
        std::fs::create_dir(root.path().join("main")).unwrap();
        std::fs::write(root.path().join("main/main.c"), "unchanged").unwrap();
        assert_eq!(load(root.path()).unwrap(), None);
        save(root.path(), FanPort::Usb).unwrap();
        assert_eq!(load(root.path()).unwrap(), Some(FanPort::Usb));
        std::fs::write(root.path().join(FILE), "{broken").unwrap();
        assert!(load(root.path()).is_err());
        assert_eq!(
            std::fs::read_to_string(root.path().join("main/main.c")).unwrap(),
            "unchanged"
        );
    }
}

#[cfg(test)]
mod run_guard_tests {
    use super::*;
    #[test]
    fn paired_hashes_cannot_hide_wrong_hardware_channels() {
        let root = tempfile::tempdir().unwrap();
        std::fs::create_dir(root.path().join("main")).unwrap();
        std::fs::create_dir(root.path().join("simulation")).unwrap();
        let scope = PathScope::new(root.path()).unwrap();
        save(root.path(), FanPort::Usb).unwrap();
        for (firmware_port, sim_port, valid) in [
            (FanPort::Out1, FanPort::Out1, false),
            (FanPort::Usb, FanPort::Out1, false),
            (FanPort::Usb, FanPort::Usb, true),
        ] {
            let c = crate::fan_recipe::firmware(firmware_port);
            std::fs::write(root.path().join("main/main.c"), &c).unwrap();
            let js = crate::simulation_compiler::compile(
                &crate::fan_recipe::program(sim_port).to_string(),
            )
            .unwrap();
            let stamped =
                crate::simulation_project::stamp_source(&scope, &js, &BTreeMap::new()).unwrap();
            std::fs::write(
                root.path().join(crate::simulation_project::SOURCE_PATH),
                stamped,
            )
            .unwrap();
            assert_eq!(crate::simulation_project::load(&scope, &[]).is_ok(), valid);
            assert_eq!(
                std::fs::read_to_string(root.path().join("main/main.c")).unwrap(),
                c
            );
        }
    }
}

// Pure explanations must not rewrite device metadata or claim code validation.
pub(crate) fn generation_tool(name: &str, input: &serde_json::Value) -> bool {
    if matches!(
        name,
        "generate_fan_control" | "generate_simulation" | "generate_device_program" | "create_project_workspace"
    ) {
        return true;
    }
    let path = input["path"]
        .as_str()
        .unwrap_or("")
        .replace('\\', "/")
        .to_lowercase();
    name == "write_file"
        && (crate::simulation_project::is_firmware_input(path.trim_start_matches("./"))
            || crate::simulation_project::is_companion_path(&path))
}
