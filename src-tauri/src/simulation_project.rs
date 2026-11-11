//! Version-paired simulation companion. Hashes identify file revisions, not semantic equivalence.
use crate::simulation_paths::{safe_traversal_entry, PathScope};
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::{
    collections::BTreeMap,
    io::Read,
    path::{Path, PathBuf},
};

pub(crate) const SOURCE_PATH: &str = "simulation/main.sim.js";
pub(crate) fn is_companion_path(relative: &str) -> bool {
    crate::simulation_paths::validate_relative_path(relative).is_ok_and(|path| {
        let normalized: PathBuf = path
            .components()
            .filter(|part| *part != std::path::Component::CurDir)
            .collect();
        normalized == Path::new(SOURCE_PATH)
    })
}
const PREFIX: &str = "// @kidbright-simulation ";
const RUNTIME: &str = "kidbright-local-v1";
const MAX_FILE: u64 = 256 * 1024;

pub(crate) const AI_INSTRUCTIONS: &str = r#"
### LOCAL SIMULATOR COMPANION (AUTOMATIC FOR SUPPORTED CODE GENERATION)
- When creating/changing firmware behavior using LED matrix, temperature, light, S1/S2, buzzer or OUT/USB, ALSO use generate_simulation to create/update simulation/main.sim.js AFTER all firmware writes. Do this in the same task, without waiting for the user to request simulation separately. Read simulator_programs.md with read_knowledge_file first.
- This is a separate ES5 simulation program, NOT a C/C++ translator and NOT proof the firmware works. Firmware stays pure ESP-IDF C/C++; simulation/main.sim.js is the explicit JavaScript exception. Preserve existing unrelated behavior in BOTH files.
- For a fresh supported KidBright32/ESP32 program, prefer generate_device_program with one COMPLETE behavior description so both firmware and simulation share the same logic. Read simulator_paired_programs.md. It preserves custom/edited firmware by refusing replacement, and existing files still use Keep/Undo. Never remove unrelated behavior to fit a template.
- For custom firmware or simulator-only repair, prefer generate_simulation AFTER reading the actual firmware. Read simulator_compiler.md. Supply complete device-behavior JSON; the IDE compiles it to the original bitmap/port/sensor JavaScript APIs. It ONLY writes simulation/main.sim.js through the normal Diff workflow. Keep firmware intact when repairing a companion.
- Cover all requested device behavior: outputs, glyph bitmaps, timing, loop order, input conditions and arithmetic. For A-Z, text/char operations are available, or bitmap operations using the actual firmware font to match the physical display. Never replace unsupported firmware with a delay-only loop. If the request needs a known unsupported capability, do not call a generator that cannot cover it; finish real firmware and call report_simulation_unsupported instead. Never omit the behavior to obtain a successful generator result.
- Buzzer: sound.note(0..36), sound.rest(0..4 at 120 BPM), sound.off(), sound.set_volume(0..100), sound.get_volume(); sound.tone(20..20000 Hz) is OUR local PWM-frequency extension, not an original Unity API. Use tone/note/volume/buzzer_off operations; firmware must retain its real board-specific LEDC/GPIO driver. Audio is rendered locally by the IDE Web Audio renderer, not by executing C/C++ or sending a cloud API request.
- Supported calls: lm73_1.get(), ldr.get(), button12.is_sw1_pressed(), button12.is_sw2_pressed(), button12.is_sw1_released(), button12.is_sw2_released(), button12.sw1_get(), button12.sw2_get(), ports.output1_write/read(), ports.output2_write/read(), ports.usbsw_write/read(), ports.input1_read()..input4_read() for Digital, kb_specio.input1_read()..input4_read() for raw Analog (IR sensor analog1..4, 0..4095), ht16k33.show(16-byte-string), ht16k33.idle(), ht16k33.wait_idle(), vTaskDelay(ms), matrix.show_number(value).
- External sensors: use the actual connected IN channel and its Analog/Digital mode. Digital sensor input1..4 is 0/1; Analog sensor analog1..4 preserves raw readings, not calibrated moisture percent/lux/gas ppm. Do not alias an external light sensor to the on-board ldr.get(). For paired analog firmware require hardware_json profile kidbright32_v15_analog_bypass only after the user confirms bypass/unbuffered input wiring; V1.3 and unknown revisions remain digital-only. If revision is unknown, ask before generating analog board drivers; local draft examples still work. Do not independently use Analog and Digital on the same physical connector.
- matrix.show_number is OUR local helper (not an original Unity API). It rounds a finite numeric value to an integer and displays -999..9999 on 16x8 LEDs. Example temperature display:
  while (1) { matrix.show_number(lm73_1.get()); vTaskDelay(250); }
- Only ES5 variables, arithmetic, arrays, if/while/for and the listed device calls. No user functions, dynamic calls, Math/String/browser/Node APIs, imports, eval, network, Wi-Fi or FreeRTOS tasks. Scrolling is supported through the generator scroll operation, lowered to bitmap frames; do not call ht16k33.scroll directly. Delay arguments are milliseconds 0..60000; keep loops responsive with vTaskDelay. A run has a 5-minute/1-million-step limit.
- generate_simulation uses the internal write_file pipeline to stamp simulation/main.sim.js; raw AI write_file calls for that path are rejected. It stamps with hashes of the current/proposed main/ and components/ sources and root CMake/config files. Do NOT invent or edit that metadata. Write firmware first, companion last. Existing file changes still require Keep/Undo; tell the user to accept BOTH before Run Sim. If hardware sources are edited later, regenerate the companion, do not relabel stale code as current.
- For unsupported behavior (servo/PWM, Wi-Fi, Bluetooth, RTC, accelerometer, parallel tasks or networking), finish the real board firmware first, then call report_simulation_unsupported with the missing capabilities. This records a FIRMWARE-ONLY, NOT simulation-ready outcome. Explain the limitation and board-revision/pin uncertainty; do NOT produce a dummy companion, replace servo angle control with OUT switching, or claim Build/Flash was tested. Do not auto-build/flash for a simulation-only request or call an external Simulator API. The user starts local Run Simulator; the IDE requests Unity Play after loading.
"#;

#[derive(Clone, Debug, Serialize, Deserialize, PartialEq, Eq)]
struct Manifest {
    version: u32,
    runtime: String,
    source_sha256: String,
    firmware: BTreeMap<String, String>,
}
#[derive(Debug, Serialize)]
pub struct ProjectSimulation {
    pub path: String,
    pub source: String,
    pub revision: String,
    pub firmware_files: Vec<String>,
}
fn hash(bytes: &[u8]) -> String {
    format!("{:x}", Sha256::digest(bytes))
}

pub(crate) fn is_firmware_input(relative: &str) -> bool {
    let relative = relative.replace('\\', "/");
    if matches!(
        relative.as_str(),
        "CMakeLists.txt"
            | "sdkconfig.defaults"
            | "sdkconfig.defaults.esp32"
            | "sdkconfig.defaults.esp32s3"
    ) {
        return true;
    }
    if !(relative.starts_with("main/") || relative.starts_with("components/")) {
        return false;
    }
    let file = Path::new(&relative);
    matches!(
        file.extension().and_then(|s| s.to_str()),
        Some("c" | "cpp" | "cc" | "h" | "hpp" | "s" | "S" | "cmake")
    ) || file.file_name().is_some_and(|s| {
        s == "CMakeLists.txt" || s == "idf_component.yml" || s == "Kconfig.projbuild"
    })
}
fn read_limited(path: &Path, limit: u64) -> Result<Vec<u8>, String> {
    let mut file =
        std::fs::File::open(path).map_err(|e| format!("Cannot read simulation input: {e}"))?;
    let mut bytes = Vec::new();
    file.by_ref()
        .take(limit + 1)
        .read_to_end(&mut bytes)
        .map_err(|e| format!("Cannot read simulation input: {e}"))?;
    if bytes.len() as u64 > limit {
        return Err("Simulation input is too large".into());
    }
    Ok(bytes)
}
fn firmware_snapshot(
    scope: &PathScope,
    proposed: &BTreeMap<PathBuf, String>,
) -> Result<BTreeMap<String, String>, String> {
    fn walk(
        scope: &PathScope,
        dir: &Path,
        depth: usize,
        files: &mut BTreeMap<String, Vec<u8>>,
    ) -> Result<(), String> {
        if depth > 12 {
            return Err("Firmware input directory is too deep".into());
        }
        for entry in
            std::fs::read_dir(dir).map_err(|e| format!("Cannot inspect firmware inputs: {e}"))?
        {
            let entry = entry.map_err(|e| e.to_string())?;
            let path = entry.path();
            if !safe_traversal_entry(&scope.root, &path) {
                return Err("Firmware inputs contain a link or an inaccessible path; use regular files inside the workspace".into());
            }
            if path.is_dir() {
                walk(scope, &path, depth + 1, files)?;
            } else {
                let rel = path
                    .strip_prefix(&scope.root)
                    .map_err(|_| "Input escaped workspace")?
                    .to_string_lossy()
                    .replace('\\', "/");
                if is_firmware_input(&rel) {
                    if files.len() >= 128 {
                        return Err(
                            "Too many firmware inputs for this simulation companion (maximum 128)"
                                .into(),
                        );
                    }
                    files.insert(
                        rel,
                        read_limited(
                            &scope.resolve(
                                &path.strip_prefix(&scope.root).unwrap().to_string_lossy(),
                                false,
                            )?,
                            MAX_FILE,
                        )?,
                    );
                }
            }
        }
        Ok(())
    }
    let mut files = BTreeMap::new();
    for root in ["main", "components"] {
        let path = scope.resolve(root, true)?;
        if path.exists() {
            walk(scope, &path, 0, &mut files)?;
        }
    }
    for name in [
        "CMakeLists.txt",
        "sdkconfig.defaults",
        "sdkconfig.defaults.esp32",
        "sdkconfig.defaults.esp32s3",
    ] {
        let path = scope.resolve(name, true)?;
        if path.exists() {
            files.insert(name.into(), read_limited(&path, MAX_FILE)?);
        }
    }
    for (path, source) in proposed {
        let relative = path
            .strip_prefix(&scope.root)
            .map_err(|_| "Proposed input escaped workspace")?
            .to_string_lossy()
            .replace('\\', "/");
        if is_firmware_input(&relative) {
            scope.resolve(&relative, true)?;
            if source.len() as u64 > MAX_FILE {
                return Err("Proposed firmware input is too large".into());
            }
            files.insert(relative, source.as_bytes().to_vec());
        }
    }
    if !files.keys().any(|p| {
        p.starts_with("main/")
            && matches!(
                Path::new(p).extension().and_then(|s| s.to_str()),
                Some("c" | "cpp" | "cc")
            )
    }) {
        return Err(
            "Write the project firmware under main/ before creating simulation/main.sim.js".into(),
        );
    }
    if files.len() > 128 || files.values().map(Vec::len).sum::<usize>() > 4 * 1024 * 1024 {
        return Err("Firmware inputs exceed the simulation snapshot limit".into());
    }
    Ok(files
        .into_iter()
        .map(|(path, bytes)| (path, hash(&bytes)))
        .collect())
}

pub(crate) fn stamp_source(
    scope: &PathScope,
    source: &str,
    proposed: &BTreeMap<PathBuf, String>,
) -> Result<String, String> {
    let body = if source.starts_with(PREFIX) {
        source
            .split_once('\n')
            .ok_or("Simulation metadata has no program")?
            .1
    } else {
        source
    };
    if body.trim().is_empty() || body.len() > 128 * 1024 {
        return Err("Simulation source is empty or too large".into());
    }
    crate::simulation_validation::require_device_output(body)?;
    let manifest = Manifest {
        version: 1,
        runtime: RUNTIME.into(),
        source_sha256: hash(body.as_bytes()),
        firmware: firmware_snapshot(scope, proposed)?,
    };
    Ok(format!(
        "{PREFIX}{}\n{body}",
        serde_json::to_string(&manifest).map_err(|e| e.to_string())?
    ))
}

pub(crate) fn load(scope: &PathScope, pending: &[String]) -> Result<ProjectSimulation, String> {
    if pending
        .iter()
        .any(|path| path == SOURCE_PATH || is_firmware_input(path))
    {
        return Err(
            format!("มี Diff รอตรวจรับ: {} — กด Keep หรือ Undo สำหรับไฟล์เหล่านี้ก่อน Run Sim",pending.iter().filter(|path|path.as_str()==SOURCE_PATH||is_firmware_input(path)).cloned().collect::<Vec<_>>().join(", ")),
        );
    }
    let path = scope.resolve(SOURCE_PATH, false).map_err(|_| "ยังไม่มี simulation/main.sim.js: ให้ AI สร้างคู่ไฟล์สำหรับงานที่ซิมรองรับก่อน หรือเลือกตัวอย่างทดลองแยก หากใช้ความสามารถที่ซิมยังไม่มี เช่น เซอร์โว/PWM จะยังรันซิมงานนั้นไม่ได้ ให้ตรวจเฟิร์มแวร์และรุ่นบอร์ดสำหรับ Build/Flash แยก ห้ามสร้างไฟล์จำลองเปล่า".to_string())?;
    if path != scope.root.join(SOURCE_PATH) {
        return Err("Simulation companion must not be redirected by a link".into());
    }
    let text = String::from_utf8(read_limited(&path, MAX_FILE)?)
        .map_err(|_| "Simulation source must be UTF-8")?;
    let (header, source) = text
        .split_once('\n')
        .ok_or("ไฟล์จำลองไม่มีข้อมูลรุ่น ให้ AI สร้างไฟล์ simulation/main.sim.js ใหม่")?;
    let manifest: Manifest = serde_json::from_str(
        header
            .strip_prefix(PREFIX)
            .ok_or("ไฟล์จำลองไม่มีข้อมูลรุ่น ให้ AI บันทึก simulation/main.sim.js ผ่าน write_file ใหม่")?,
    )
    .map_err(|_| "Invalid simulation metadata")?;
    if manifest.version != 1 || manifest.runtime != RUNTIME {
        return Err("Unsupported simulation runtime/version".into());
    }
    if manifest.source_sha256 != hash(source.as_bytes()) {
        return Err(
            "ไฟล์จำลองถูกแก้หลังจับคู่กับ firmware ให้ AI ปรับและบันทึกไฟล์จำลองใหม่ หรือเลือกทดลองโค้ดร่างแยก"
                .into(),
        );
    }
    if manifest.firmware != firmware_snapshot(scope, &BTreeMap::new())? {
        return Err("firmware เปลี่ยนหลังสร้างไฟล์จำลอง ให้ AI อัปเดต simulation/main.sim.js ให้ตรงกับโค้ดล่าสุดก่อน Run Sim".into());
    }
    if let Some(port)=crate::hardware_contract::load(&scope.root)? {
        crate::hardware_contract::check(&scope.root,crate::hardware_contract::Requirements{fan:Some(port),simulation:true,..Default::default()})?;
    }
    verify_owned_program(&scope, &BTreeMap::new(),source)?;
    crate::simulation_validation::require_device_output(source)?;
    Ok(ProjectSimulation {
        path: SOURCE_PATH.into(),
        source: source.into(),
        revision: hash(header.as_bytes()),
        firmware_files: manifest.firmware.into_keys().collect(),
    })
}

// Pending diff keys use editor paths; canonicalize the existing ancestor so
// Windows C:\ paths and canonical \?\ paths refer to the same revision.
fn canonical_candidate(path: &Path) -> Option<PathBuf> {
    crate::pending_diff::canonical_candidate(path)
}

pub(crate) fn proposed_inputs(scope: &PathScope) -> Result<BTreeMap<PathBuf, String>, String> {
    let diffs = crate::ai_chat::get_pending_diffs()
        .lock()
        .map_err(|_| "Pending diff state unavailable")?;
    Ok(diffs
        .iter()
        .filter_map(|(path, content)| {
            let resolved = canonical_candidate(path)?;
            resolved
                .starts_with(&scope.root)
                .then(|| (resolved, content.clone()))
        })
        .collect())
}
fn companion_guards() -> &'static std::sync::Mutex<BTreeMap<PathBuf, PathScope>> {
    static GUARDS: std::sync::OnceLock<std::sync::Mutex<BTreeMap<PathBuf, PathScope>>> =
        std::sync::OnceLock::new();
    GUARDS.get_or_init(Default::default)
}
pub(crate) fn recheck_companion(path: &Path) -> Result<(), String> {
    if path.file_name().is_none_or(|name| name != "main.sim.js")
        || path
            .parent()
            .and_then(Path::file_name)
            .is_none_or(|name| name != "simulation")
    {
        return Ok(());
    }
    let guards = companion_guards()
        .lock()
        .map_err(|_| "Companion guards unavailable")?;
    let resolved = canonical_candidate(path).ok_or("Cannot resolve companion destination")?;
    let scope = guards
        .get(path)
        .or_else(|| {
            guards
                .values()
                .find(|scope| scope.root.join(SOURCE_PATH) == resolved)
        })
        .ok_or(
            "Companion destination is not the reviewed project path; ask AI to propose it again",
        )?;
    let expected = scope.resolve(SOURCE_PATH, true)?;
    if expected != scope.root.join(SOURCE_PATH) || resolved != expected {
        return Err("Companion destination changed after review; refusing to write outside the original project".into());
    }
    Ok(())
}
pub(crate) fn register_companion_review(project:&Path,relative:&str)->Result<(),String> {
    let scope=PathScope::new(project)?;
    if scope.resolve(relative,true)?!=scope.root.join(SOURCE_PATH) {return Err("Companion path escaped the project".into());}
    companion_guards().lock().map_err(|_|"Companion guards unavailable")?.insert(project.join(relative),scope);
    Ok(())
}
pub(crate) fn prepare_companion(
    project: &Path,
    relative: &str,
    content: &str,
) -> Result<String, String> {
    let scope = PathScope::new(project)?;
    let target = scope.resolve(relative, true)?;
    if target != scope.root.join(SOURCE_PATH) {
        return Err("Companion path must remain inside simulation/main.sim.js".into());
    }
    if let Some(port)=crate::hardware_contract::load(project)? {
        crate::hardware_contract::check(project,crate::hardware_contract::Requirements{fan:Some(port),simulation:false,..Default::default()})?;
        crate::hardware_contract::validate_simulation(content,port)?;
    }
    let proposed=proposed_inputs(&scope)?;
    reject_boilerplate_firmware(&scope,&proposed)?;
    verify_owned_program(&scope,&proposed,content)?;
    let stamped = stamp_source(&scope, content, &proposed)?;
    companion_guards()
        .lock()
        .map_err(|_| "Companion guards unavailable")?
        .insert(project.join(relative), scope);
    Ok(stamped)
}

#[tauri::command]
pub fn load_project_simulation(
    window: tauri::WebviewWindow,
    project_dir: String,
) -> Result<ProjectSimulation, String> {
    if window.label() != "main" || project_dir == "." {
        return Err("เลือกโปรเจกต์ก่อนโหลดไฟล์จำลอง".into());
    }
    let scope = PathScope::new(Path::new(&project_dir))?;
    let proposed = proposed_inputs(&scope)?;
    let pending: Vec<String> = proposed
        .keys()
        .map(|path| {
            path.strip_prefix(&scope.root)
                .unwrap()
                .to_string_lossy()
                .replace('\\', "/")
        })
        .collect();
    load(&scope, &pending)
}

#[cfg(test)]
mod tests {
    use super::*;
    struct Fixture {
        root: PathBuf,
        scope: PathScope,
    }
    impl Fixture {
        fn new() -> Self {
            let root = std::env::temp_dir().join(format!(
                "simulation-pair-{}-{}",
                std::process::id(),
                std::time::SystemTime::now()
                    .duration_since(std::time::UNIX_EPOCH)
                    .unwrap()
                    .as_nanos()
            ));
            std::fs::create_dir_all(root.join("main")).unwrap();
            std::fs::create_dir_all(root.join("simulation")).unwrap();
            std::fs::write(
                root.join("main/main.c"),
                "void app_main(void) {} // วัดอุณหภูมิ",
            )
            .unwrap();
            let scope = PathScope::new(&root).unwrap();
            Self { root, scope }
        }
        fn stamp(&self, proposed: &BTreeMap<PathBuf, String>) {
            let content = stamp_source(
                &self.scope,
                "while(1){matrix.show_number(lm73_1.get());vTaskDelay(250);}",
                proposed,
            )
            .unwrap();
            std::fs::write(self.root.join(SOURCE_PATH), content).unwrap();
        }
    }
    impl Drop for Fixture {
        fn drop(&mut self) {
            let _ = std::fs::remove_dir_all(&self.root);
        }
    }
    #[test]
    fn pairs_unicode_firmware_and_rejects_content_changes_added_headers_and_source_tampering() {
        let f = Fixture::new();
        f.stamp(&BTreeMap::new());
        let program = load(&f.scope, &[]).unwrap();
        assert!(program.source.contains("matrix.show_number"));
        assert_eq!(program.firmware_files, ["main/main.c"]);
        std::fs::write(f.root.join("main/เซนเซอร์.h"), "#define SENSOR 1").unwrap();
        assert!(load(&f.scope, &[]).unwrap_err().contains("firmware"));
        f.stamp(&BTreeMap::new());
        assert_ne!(load(&f.scope, &[]).unwrap().revision, program.revision);
        std::fs::write(
            f.root.join("main/main.c"),
            "void app_main(void) { /* changed */ }",
        )
        .unwrap();
        assert!(load(&f.scope, &[]).is_err());
        f.stamp(&BTreeMap::new());
        let text = std::fs::read_to_string(f.root.join(SOURCE_PATH)).unwrap();
        std::fs::write(
            f.root.join(SOURCE_PATH),
            format!("{text}\nports.output2_write(1);"),
        )
        .unwrap();
        assert!(load(&f.scope, &[]).unwrap_err().contains("ไฟล์จำลองถูกแก้"));
    }
    #[test]
    fn pending_firmware_is_paired_but_cannot_run_until_reviews_match_disk() {
        let f = Fixture::new();
        let replacement = "void app_main(void) { /* ใหม่ */ }".to_string();
        let proposed = BTreeMap::from([(f.scope.root.join("main/main.c"), replacement.clone())]);
        f.stamp(&proposed);
        assert!(load(&f.scope, &["main/main.c".into()])
            .unwrap_err()
            .contains("Diff"));
        assert!(load(&f.scope, &[SOURCE_PATH.into()]).is_err());
        assert!(load(&f.scope, &[]).is_err());
        std::fs::write(f.root.join("main/main.c"), replacement).unwrap();
        assert!(load(&f.scope, &[]).is_ok());
        assert!(load(&f.scope, &["README.md".into()]).is_ok());
    }
    #[test]
    fn rejects_missing_malformed_unsupported_and_oversized_programs_without_reading_metadata_paths()
    {
        let f = Fixture::new();
        assert!(load(&f.scope, &[]).is_err());
        std::fs::write(f.root.join(SOURCE_PATH), "ports.output2_write(1);").unwrap();
        assert!(load(&f.scope, &[]).is_err());
        f.stamp(&BTreeMap::new());
        let text = std::fs::read_to_string(f.root.join(SOURCE_PATH)).unwrap();
        std::fs::write(
            f.root.join(SOURCE_PATH),
            text.replace("kidbright-local-v1", "unknown-v2"),
        )
        .unwrap();
        assert!(load(&f.scope, &[]).unwrap_err().contains("runtime"));
        std::fs::write(f.root.join(SOURCE_PATH), vec![b'a'; MAX_FILE as usize + 1]).unwrap();
        assert!(load(&f.scope, &[]).unwrap_err().contains("large"));
        assert!(is_companion_path("./simulation/main.sim.js"));
        assert!(is_companion_path("simulation\\main.sim.js"));
        assert!(!is_companion_path("../simulation/main.sim.js"));
        assert!(!is_companion_path("/simulation/main.sim.js"));
    }
    #[test]
    fn rejects_escaping_firmware_links() {
        let f = Fixture::new();
        let outside = Fixture::new();
        #[cfg(windows)]
        {
            assert!(std::process::Command::new("cmd")
                .args(["/C", "mklink", "/J"])
                .arg(f.root.join("components"))
                .arg(&outside.root)
                .output()
                .unwrap()
                .status
                .success());
        }
        #[cfg(unix)]
        std::os::unix::fs::symlink(&outside.root, f.root.join("components")).unwrap();
        assert!(stamp_source(&f.scope, "ports.output2_write(1);", &BTreeMap::new()).is_err());
    }
    #[test]
    fn prompt_requires_real_companion_and_refuses_unsupported_claims() {
        assert!(AI_INSTRUCTIONS.contains("simulation/main.sim.js"));
        assert!(AI_INSTRUCTIONS.contains("matrix.show_number(lm73_1.get())"));
        assert!(AI_INSTRUCTIONS.contains("NOT proof"));
        assert!(AI_INSTRUCTIONS.contains("Do NOT invent"));
        assert!(AI_INSTRUCTIONS.contains("report_simulation_unsupported"));
        assert!(AI_INSTRUCTIONS.contains("do not call a generator that cannot cover it"));
    }
}

#[cfg(test)]
mod integration_tests {
    use super::*;
    #[test]
    fn proposed_editor_paths_are_paired_and_late_redirect_is_rejected() {
        let directory = tempfile::tempdir().unwrap();
        let root = directory.path();
        std::fs::create_dir(root.join("main")).unwrap();
        std::fs::write(root.join("main/main.c"), "void app_main(void) {} // old").unwrap();
        let pending = root.join("main/main.c");
        crate::ai_chat::get_pending_diffs()
            .lock()
            .unwrap()
            .insert(pending.clone(), "void app_main(void) {} // revised".into());
        let stamped = prepare_companion(root, SOURCE_PATH, "ports.output2_write(1);").unwrap();
        crate::ai_chat::get_pending_diffs()
            .lock()
            .unwrap()
            .remove(&pending);
        std::fs::create_dir(root.join("simulation")).unwrap();
        let destination = root.join(SOURCE_PATH);
        std::fs::write(&destination, &stamped).unwrap();
        let scope = PathScope::new(root).unwrap();
        assert!(load(&scope, &[]).unwrap_err().contains("firmware"));
        std::fs::write(&pending, "void app_main(void) {} // revised").unwrap();
        assert!(load(&scope, &[]).is_ok());
        recheck_companion(&destination).unwrap();
        std::fs::rename(root.join("simulation"), root.join("saved-simulation")).unwrap();
        let outside = tempfile::tempdir().unwrap();
        #[cfg(windows)]
        assert!(std::process::Command::new("cmd.exe")
            .args(["/C", "mklink", "/J"])
            .arg(root.join("simulation"))
            .arg(outside.path())
            .output()
            .unwrap()
            .status
            .success());
        #[cfg(unix)]
        std::os::unix::fs::symlink(outside.path(), root.join("simulation")).unwrap();
        assert!(recheck_companion(&destination).is_err());
        assert!(!outside.path().join("main.sim.js").exists());
    }
}


#[cfg(test)]
mod generated_companion_tests {
    use super::*;
    #[test]
    fn compiler_and_pairing_preserve_all_firmware_bytes_and_reject_old_delay_only_files() {
        let temp=tempfile::tempdir().unwrap();
        std::fs::create_dir(temp.path().join("main")).unwrap();
        let firmware=b"void app_main(void) { /* board code stays intact */ }";
        std::fs::write(temp.path().join("main/main.c"),firmware).unwrap();
        std::fs::create_dir(temp.path().join("simulation")).unwrap();
        let scope=PathScope::new(temp.path()).unwrap();
        assert!(stamp_source(&scope,"while(1){vTaskDelay(500);}",&BTreeMap::new()).is_err());
        let source=crate::simulation_compiler::compile(r#"{"version":1,"steps":[{"op":"text","text":"AZ","ms":500}]}"#).unwrap();
        let stamped=stamp_source(&scope,&source,&BTreeMap::new()).unwrap();
        std::fs::write(temp.path().join(SOURCE_PATH),stamped).unwrap();
        assert!(load(&scope,&[]).unwrap().source.contains("ht16k33.show"));
        assert_eq!(std::fs::read(temp.path().join("main/main.c")).unwrap(),firmware);
        let placeholder="while(1){vTaskDelay(500);}";
        let legacy=Manifest{version:1,runtime:RUNTIME.into(),source_sha256:hash(placeholder.as_bytes()),firmware:firmware_snapshot(&scope,&BTreeMap::new()).unwrap()};
        std::fs::write(temp.path().join(SOURCE_PATH),format!("{PREFIX}{}\n{placeholder}",serde_json::to_string(&legacy).unwrap())).unwrap();
        assert!(load(&scope,&[]).unwrap_err().contains("ยังไม่มีคำสั่ง"));
        assert_eq!(std::fs::read(temp.path().join("main/main.c")).unwrap(),firmware);
    }
}

#[cfg(test)]
mod buzzer_pairing_tests {
    use super::*;
    #[test]
    fn audio_only_companion_pairs_without_modifying_firmware() {
        let project=tempfile::tempdir().unwrap();
        std::fs::create_dir(project.path().join("main")).unwrap();
        let source="void app_main(void) { /* real firmware stays intact */ }";
        std::fs::write(project.path().join("main/main.c"),source).unwrap();
        let js=crate::simulation_compiler::compile(r#"{"version":1,"steps":[{"op":"tone","hz":1000,"ms":200}]}"#).unwrap();
        let paired=prepare_companion(project.path(),SOURCE_PATH,&js).unwrap();
        std::fs::create_dir(project.path().join("simulation")).unwrap();
        std::fs::write(project.path().join(SOURCE_PATH),paired).unwrap();
        assert!(load(&PathScope::new(project.path()).unwrap(),&[]).is_ok());
        assert_eq!(std::fs::read_to_string(project.path().join("main/main.c")).unwrap(),source);
    }
}

// Generation checks inspect effective pending content. They never Keep, Undo or write firmware.
fn effective_source(scope:&PathScope,proposed:&BTreeMap<PathBuf,String>,relative:&str)->Result<String,String>{
    let path=scope.resolve(relative,true)?;
    if let Some(source)=proposed.get(&path){return Ok(source.clone());}
    String::from_utf8(read_limited(&path,MAX_FILE)?).map_err(|_|"Firmware/simulation must be UTF-8".into())
}
fn main_source(scope:&PathScope,proposed:&BTreeMap<PathBuf,String>)->Result<String,String>{
    for name in ["main/main.c","main/main.cpp"]{
        let path=scope.resolve(name,true)?;
        if proposed.contains_key(&path)||path.is_file(){return effective_source(scope,proposed,name);}
    }
    // Custom component layouts remain custom: inspect actual main/ sources without creating a replacement.
    let snapshot=firmware_snapshot(scope,proposed)?;
    for name in snapshot.keys().filter(|name|name.starts_with("main/")&&(name.ends_with(".c")||name.ends_with(".cpp")||name.ends_with(".cc"))){
        let source=effective_source(scope,proposed,name)?;
        if source.contains("app_main(")||source.contains("app_main ("){return Ok(source);}
    }
    Err("No firmware entry source was found; generate real ESP-IDF firmware before the companion".into())
}
fn reject_boilerplate_firmware(scope:&PathScope,proposed:&BTreeMap<PathBuf,String>)->Result<(),String>{
    if crate::device_program::boilerplate(&main_source(scope,proposed)?){return Err("Firmware is still the Hello boilerplate. Write the requested real board program first, or use generate_device_program for a supported fresh project; a simulator file alone is not complete firmware.".into());}
    Ok(())
}
fn source_body(source:&str)->&str{if source.starts_with(PREFIX){source.split_once('\n').map_or(source,|(_,body)|body)}else{source}}
fn verify_owned_program(scope:&PathScope,proposed:&BTreeMap<PathBuf,String>,simulation:&str)->Result<Option<crate::device_program::Bundle>,String>{
    let firmware=main_source(scope,proposed)?;
    let Some(bundle)=crate::device_program::decode(&firmware)? else{return Ok(None);};
    check_shared_sources(scope,proposed)?;
    if firmware.replace("\r\n","\n")!=bundle.firmware{return Err("Owned generated firmware was edited outside its behavior program. Preserve the edits; update the complete shared program or use the custom firmware + companion workflow. Do not relabel it as a verified pair.".into());}
    if source_body(simulation).replace("\r\n","\n")!=bundle.source{return Err("Simulation differs from the shared behavior that generated this firmware. Regenerate the complete device program; do not change firmware just to hide the mismatch.".into());}
    Ok(Some(bundle))
}
pub(crate) fn can_generate_device_program(project:&Path)->Result<(),String>{
    let scope=PathScope::new(project)?;let proposed=proposed_inputs(&scope)?;
    // Do not install classic ESP32 firmware into a project configured for another target.
    for name in ["sdkconfig.defaults","sdkconfig"] {
        let path=scope.resolve(name,true)?;
        if path.exists()||proposed.contains_key(&path){
            let text=effective_source(&scope,&proposed,name)?;
            for line in text.lines().filter(|line|line.starts_with("CONFIG_IDF_TARGET=")){
                if line.trim()!="CONFIG_IDF_TARGET=\"esp32\""{return Err("Paired classic-reference firmware requires CONFIG_IDF_TARGET=esp32; existing target configuration is preserved".into());}
            }
        }
    }
    check_shared_sources(&scope,&proposed)?;
    crate::device_program::can_replace(&effective_source(&scope,&proposed,"main/main.c")?)
}
pub(crate) fn verify_generated_program(project:&Path)->Result<serde_json::Value,String>{
    let scope=PathScope::new(project)?;let proposed=proposed_inputs(&scope)?;
    reject_boilerplate_firmware(&scope,&proposed)?;
    let path=scope.resolve(SOURCE_PATH,true)?;
    if !proposed.contains_key(&path) && !path.exists() {
        return Err("ยังไม่มีไฟล์ simulation/main.sim.js: สำหรับงานที่ซิมรองรับ ให้สร้างคู่ไฟล์ด้วย generate_device_program หรือ generate_simulation หลังเขียนเฟิร์มแวร์ หากซิมไม่รองรับ ให้รายงานด้วย report_simulation_unsupported ห้ามสร้างไฟล์จำลองเปล่าเพื่อข้ามการตรวจ".into());
    }
    let companion=effective_source(&scope,&proposed,SOURCE_PATH)?;
    let (header,body)=companion.split_once('\n').ok_or("Generate a revision-paired simulation companion first")?;
    let manifest:Manifest=serde_json::from_str(header.strip_prefix(PREFIX).ok_or("Missing companion revision metadata")?).map_err(|_|"Invalid companion revision metadata")?;
    if manifest.version!=1||manifest.runtime!=RUNTIME||manifest.source_sha256!=hash(body.as_bytes())||manifest.firmware!=firmware_snapshot(&scope,&proposed)?{
        return Err("Firmware/simulation revisions do not match the current/proposed code. Finish firmware changes, then regenerate the companion before reporting completion.".into());
    }
    crate::simulation_validation::require_device_output(body)?;
    match verify_owned_program(&scope,&proposed,&companion)? {
        Some(bundle)=>Ok(crate::device_program::result(&bundle)),
        None=>Ok(serde_json::json!({"validation":"revision_only","buildChecked":false,"physicalBoardChecked":false})),
    }
}


#[cfg(test)]
mod shared_program_review_tests {
    use super::*;
    fn fixture()->tempfile::TempDir{
        let parent=tempfile::tempdir().unwrap();
        crate::ai::workflow::initialize_project(parent.path(),"program").unwrap();parent
    }
    #[test]
    fn a_simulator_file_cannot_hide_hello_firmware_or_an_unaccepted_review(){
        let parent=fixture();let project=parent.path().join("program");let scope=PathScope::new(&project).unwrap();
        assert!(prepare_companion(&project,SOURCE_PATH,"ports.output1_write(1);").unwrap_err().contains("Hello"));
        let before=std::fs::read(project.join("main/main.c")).unwrap();
        let program=r#"{"version":1,"steps":[{"op":"number","value":{"kind":"sensor","name":"temperature"}},{"op":"tone","hz":1000,"ms":100}]}"#;
        let bundle=crate::device_program::compile(program,Default::default()).unwrap();
        let c_path=scope.resolve("main/main.c",false).unwrap();
        let sim_path=scope.resolve(SOURCE_PATH,true).unwrap();
        crate::ai_chat::get_pending_diffs().lock().unwrap().insert(c_path.clone(),bundle.firmware.clone());
        let stamped=prepare_companion(&project,SOURCE_PATH,&bundle.source).unwrap();
        crate::ai_chat::get_pending_diffs().lock().unwrap().insert(sim_path.clone(),stamped.clone());
        let checked=verify_generated_program(&project).unwrap();assert_eq!(checked["validation"],"shared_behavior");
        assert!(load(&scope,&["main/main.c".into(),SOURCE_PATH.into()]).is_err());
        assert_eq!(std::fs::read(project.join("main/main.c")).unwrap(),before);
        // Only the test now models an explicit Keep; production never auto-accepts.
        std::fs::write(&c_path,&bundle.firmware).unwrap();std::fs::create_dir_all(sim_path.parent().unwrap()).unwrap();std::fs::write(&sim_path,&stamped).unwrap();
        {let mut pending=crate::ai_chat::get_pending_diffs().lock().unwrap();pending.remove(&c_path);pending.remove(&sim_path);}
        assert!(load(&scope,&[]).is_ok());
        // Even freshly stamped hashes cannot bless JS from a different behavior.
        let wrong=stamp_source(&scope,"ports.output2_write(1);",&BTreeMap::new()).unwrap();std::fs::write(&sim_path,wrong).unwrap();
        assert!(verify_generated_program(&project).is_err());assert!(load(&scope,&[]).is_err());
        assert_eq!(std::fs::read_to_string(&c_path).unwrap(),bundle.firmware);
    }
    #[test]
    fn custom_driver_repairs_preserve_bytes_and_do_not_claim_semantic_verification(){
        let parent=fixture();let project=parent.path().join("program");let scope=PathScope::new(&project).unwrap();
        let custom="#include <stdio.h>\nvoid app_main(void){ puts(\"Custom driver and behavior\"); }\n";
        std::fs::write(project.join("main/main.c"),custom).unwrap();
        assert!(can_generate_device_program(&project).is_err());
        let source=prepare_companion(&project,SOURCE_PATH,"ports.output2_write(1);").unwrap();std::fs::create_dir(project.join("simulation")).unwrap();std::fs::write(project.join(SOURCE_PATH),source).unwrap();
        assert_eq!(verify_generated_program(&project).unwrap()["validation"],"revision_only");
        assert_eq!(std::fs::read_to_string(project.join("main/main.c")).unwrap(),custom);
        assert!(load(&scope,&[]).is_ok());
        std::fs::write(project.join("sdkconfig.defaults"),"CONFIG_IDF_TARGET=\"esp32s3\"\n").unwrap();
        assert!(can_generate_device_program(&project).unwrap_err().contains("esp32"));
        assert_eq!(std::fs::read_to_string(project.join("main/main.c")).unwrap(),custom);
    }
}

pub(crate) fn read_device_program(project:&Path)->Result<serde_json::Value,String>{
    let scope=PathScope::new(project)?;let proposed=proposed_inputs(&scope)?;
    crate::device_program::describe(&effective_source(&scope,&proposed,"main/main.c")?)
}

fn check_shared_sources(scope:&PathScope,proposed:&BTreeMap<PathBuf,String>)->Result<(),String>{
    let inputs=firmware_snapshot(scope,proposed)?;
    if inputs.keys().any(|name|name.starts_with("components/") || (name.starts_with("main/") && name!="main/main.c" && (name.ends_with(".c")||name.ends_with(".cpp")||name.ends_with(".cc")))) {
        return Err("Custom firmware sources/components are present. Preserve them and use the custom firmware + companion workflow; this is not a single-entry shared program.".into());
    }
    Ok(())
}

// These are explicit gaps in the local runtime/compiler, never supported operations.
pub(crate) const UNSUPPORTED_CAPABILITIES: &[&str] = &[
    "servo", "pwm", "wifi", "bluetooth", "rtc", "accelerometer",
    "parallel_tasks", "network",
];
pub(crate) const UNSUPPORTED_TOOL_DESCRIPTION: &str = "Report a firmware-only outcome after writing complete real firmware that needs a known missing local simulator capability. Call after all firmware writes instead of creating a dummy companion. Preserves files and Keep/Undo. This does NOT confirm firmware correctness, Build/Flash, physical pin wiring, or simulator readiness. Do not use for supported LED/buttons/sensors/OUT/USB/buzzer programs. capabilities must identify actual missing behavior requested by the user.";

pub(crate) fn report_simulation_unsupported(
    project: &Path,
    capabilities: &serde_json::Value,
) -> Result<serde_json::Value, String> {
    let capabilities: Vec<String> = serde_json::from_value(capabilities.clone())
        .map_err(|_| "capabilities must be an array of known unsupported simulator capabilities")?;
    if capabilities.is_empty() || capabilities.len() > UNSUPPORTED_CAPABILITIES.len()
        || capabilities.iter().any(|name| !UNSUPPORTED_CAPABILITIES.contains(&name.as_str()))
    {
        return Err("Only known missing simulator capabilities may be reported; supported programs still require a valid C/JS pair".into());
    }
    let scope = PathScope::new(project)?;
    let proposed = proposed_inputs(&scope)?;
    reject_boilerplate_firmware(&scope, &proposed)?;
    if crate::device_program::decode(&main_source(&scope, &proposed)?)?.is_some() {
        return Err("A shared device program uses supported operations; verify or regenerate its paired companion instead of declaring it unsupported".into());
    }
    let mut capabilities = capabilities;
    capabilities.sort();
    capabilities.dedup();
    Ok(serde_json::json!({
        "validation":"unsupported", "capabilities":capabilities,
        "firmware":firmware_snapshot(&scope, &proposed)?,
        "firmwareAvailable":true, "simulationReady":false,
        "buildChecked":false, "physicalBoardChecked":false,
        "result":"Firmware files are available for review only. The local simulator cannot cover the declared capabilities. Do not claim simulator/Build/Flash success. Keep/Undo still applies. Confirm board revision and pins before hardware testing."
    }))
}

pub(crate) fn generation_outcome(
    project: &Path,
    unsupported: Option<&serde_json::Value>,
) -> Result<serde_json::Value, String> {
    let Some(previous) = unsupported else { return verify_generated_program(project); };
    let report = report_simulation_unsupported(project, &previous["capabilities"])?;
    if previous["validation"] != "unsupported" || previous["firmware"] != report["firmware"] {
        return Err("Firmware changed after the unsupported-simulator report. Finish the firmware edits and report the actual missing capabilities again; do not claim simulation readiness.".into());
    }
    Ok(report)
}

#[cfg(test)]
mod unsupported_simulation_tests {
    use super::*;
    use serde_json::json;

    fn fixture() -> (tempfile::TempDir, PathBuf) {
        let temp = tempfile::tempdir().unwrap();
        let project = crate::ai::workflow::initialize_project(temp.path(), "servo").unwrap();
        (temp, project)
    }
    const SERVO: &str = "#include \"driver/ledc.h\"\nvoid app_main(void) { /* custom servo PWM driver, revision-specific wiring */ }\n";

    #[test]
    fn explicit_unsupported_report_preserves_firmware_without_inventing_a_companion() {
        let (_temp, project) = fixture();
        std::fs::write(project.join("main/main.c"), SERVO).unwrap();
        let before = std::fs::read(project.join("main/main.c")).unwrap();
        let report = report_simulation_unsupported(&project, &json!(["servo", "pwm", "servo"])).unwrap();
        let checked = generation_outcome(&project, Some(&report)).unwrap();
        assert_eq!(checked["validation"], "unsupported");
        assert_eq!(checked["capabilities"], json!(["pwm", "servo"]));
        for flag in ["simulationReady", "buildChecked", "physicalBoardChecked"] { assert_eq!(checked[flag], false); }
        assert_eq!(checked["firmwareAvailable"], true);
        assert!(!project.join("simulation").exists());
        assert_eq!(std::fs::read(project.join("main/main.c")).unwrap(), before);
        // Run stays unavailable; the outcome does not manufacture coverage.
        assert!(load(&PathScope::new(&project).unwrap(), &[]).unwrap_err().contains("เซอร์โว/PWM"));
    }

    #[test]
    fn missing_companion_stays_an_actionable_error_without_an_explicit_exception() {
        let (_temp, project) = fixture();
        std::fs::write(project.join("main/main.c"), SERVO).unwrap();
        let error = generation_outcome(&project, None).unwrap_err();
        assert!(error.contains(SOURCE_PATH));
        assert!(error.contains("generate_simulation"));
        assert!(!error.contains("os error"));
        let bundle = crate::device_program::compile(r#"{"version":1,"steps":[{"op":"number","value":42}]}"#, Default::default()).unwrap();
        std::fs::write(project.join("main/main.c"), &bundle.firmware).unwrap();
        assert!(generation_outcome(&project, None).is_err());
        assert!(report_simulation_unsupported(&project, &json!(["servo"])).unwrap_err().contains("shared device program"));
    }

    #[test]
    fn unsupported_declarations_cannot_bless_hello_supported_capabilities_or_unknown_names() {
        let (_temp, project) = fixture();
        assert!(report_simulation_unsupported(&project, &json!(["servo"])).unwrap_err().contains("Hello"));
        std::fs::write(project.join("main/main.c"), SERVO).unwrap();
        for capabilities in [json!([]), json!(["matrix"]), json!(["usb"]), json!(["buzzer"]), json!(["typo"]), json!("servo"), json!([1])] {
            assert!(report_simulation_unsupported(&project, &capabilities).is_err(), "{capabilities}");
        }
    }

    #[test]
    fn pending_firmware_is_checked_and_rechecked_without_auto_accepting_or_deleting_files() {
        let (_temp, project) = fixture();
        let scope = PathScope::new(&project).unwrap();
        let path = scope.resolve("main/main.c", false).unwrap();
        let before = std::fs::read(&path).unwrap();
        let old_sim = project.join(SOURCE_PATH);
        std::fs::create_dir(old_sim.parent().unwrap()).unwrap();
        std::fs::write(&old_sim, "old companion stays untouched").unwrap();
        crate::ai_chat::get_pending_diffs().lock().unwrap().insert(path.clone(), SERVO.into());
        let report = report_simulation_unsupported(&project, &json!(["servo"])).unwrap();
        assert!(generation_outcome(&project, Some(&report)).is_ok());
        assert_eq!(std::fs::read(&path).unwrap(), before);
        assert_eq!(crate::ai_chat::get_pending_diffs().lock().unwrap().get(&path).unwrap(), SERVO);
        crate::ai_chat::get_pending_diffs().lock().unwrap().insert(path.clone(), format!("{SERVO}// edited again\n"));
        assert!(generation_outcome(&project, Some(&report)).unwrap_err().contains("changed"));
        crate::ai_chat::get_pending_diffs().lock().unwrap().remove(&path);
        assert!(generation_outcome(&project, Some(&report)).is_err());
        assert_eq!(std::fs::read(&path).unwrap(), before);
        assert_eq!(std::fs::read_to_string(old_sim).unwrap(), "old companion stays untouched");
    }
}
