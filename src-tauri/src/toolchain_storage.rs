//! SDK storage planning. Never downloads, moves an existing SDK, or scans user files.
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::path::{Component, Path, PathBuf};

pub const GIB: u64 = 1024 * 1024 * 1024;
// Conservative initial budget for ZIPs + extracted SDK + working room.
// The exact uncompressed ZIP sizes are checked again before extraction.
pub const REQUIRED_BYTES: u64 = 12 * GIB;
pub const RESERVE_BYTES: u64 = GIB / 2;
const LOCATION_FILE: &str = "toolchain-location.json";

#[derive(Clone, Debug, Serialize)]
pub struct StorageOption {
    pub install_dir: PathBuf,
    pub available_bytes: Option<u64>,
    pub sufficient: bool,
    pub error: Option<String>,
}
#[derive(Debug, Serialize)]
pub struct StoragePlan {
    pub required_bytes: u64,
    pub options: Vec<StorageOption>,
    pub recommended_dir: Option<PathBuf>,
    pub warning: Option<String>,
}
#[derive(Serialize, Deserialize)]
struct Location {
    toolchain_dir: PathBuf,
}

fn account_id(app_data: &Path) -> String {
    let digest = Sha256::digest(app_data.as_os_str().to_string_lossy().as_bytes());
    digest[..8]
        .iter()
        .map(|byte| format!("{byte:02x}"))
        .collect()
}

fn default_dir(app_data: &Path) -> PathBuf {
    #[cfg(target_os = "macos")]
    if let Some(base) = directories::BaseDirs::new() {
        return crate::macos_idf::default_root(base.home_dir());
    }
    app_data.join("toolchain")
}

pub fn alternative_dir(app_data: &Path, base: &Path) -> PathBuf {
    base.join(".vibekidbright-toolchain")
        .join(account_id(app_data))
        .join("toolchain")
}

// Only app-owned directories may be cleaned/replaced by the installer.
pub fn validate_location(app_data: &Path, dir: &Path) -> Result<(), String> {
    if !dir.is_absolute() || dir.components().any(|c| matches!(c, Component::ParentDir)) {
        return Err("ตำแหน่ง SDK ต้องเป็น absolute path และไม่มี ..".into());
    }
    let external = dir.file_name().is_some_and(|name| name == "toolchain")
        && dir
            .parent()
            .and_then(Path::file_name)
            .is_some_and(|name| name == account_id(app_data).as_str())
        && dir
            .parent()
            .and_then(Path::parent)
            .and_then(Path::file_name)
            .is_some_and(|name| name == ".vibekidbright-toolchain");
    if dir != app_data.join("toolchain") && dir != default_dir(app_data) && !external {
        return Err("เลือกได้เฉพาะโฟลเดอร์ SDK ที่โปรแกรมจัดการ กรุณาเลือกตำแหน่งใหม่".into());
    }
    // Reject redirected managed directories, including junctions, before cleanup.
    for managed in [
        dir.to_path_buf(),
        dir.with_extension("installing"),
        dir.with_extension("backup"),
    ] {
        for ancestor in managed.ancestors().take(if external { 3 } else { 1 }) {
            match std::fs::symlink_metadata(ancestor) {
                Ok(meta) => {
                    #[cfg(windows)]
                    let redirected = {
                        use std::os::windows::fs::MetadataExt;
                        meta.file_attributes() & 0x400 != 0
                    };
                    #[cfg(not(windows))]
                    let redirected = meta.file_type().is_symlink();
                    if redirected {
                        return Err(format!(
                            "โฟลเดอร์ SDK เป็น symlink/junction: {}",
                            ancestor.display()
                        ));
                    }
                }
                Err(error) if error.kind() == std::io::ErrorKind::NotFound => {}
                Err(error) => return Err(format!("ตรวจตำแหน่ง SDK ไม่สำเร็จ: {error}")),
            }
        }
    }
    Ok(())
}

pub fn resolve_location(app_data: &Path) -> Result<PathBuf, String> {
    let file = app_data.join(LOCATION_FILE);
    let data = match std::fs::read(file) {
        Ok(data) => data,
        Err(error) if error.kind() == std::io::ErrorKind::NotFound => {
            return Ok(default_dir(app_data))
        }
        Err(error) => return Err(format!("อ่านตำแหน่ง SDK ที่บันทึกไว้ไม่ได้: {error}")),
    };
    let location: Location = serde_json::from_slice(&data)
        .map_err(|error| format!("ไฟล์ตำแหน่ง SDK ไม่ถูกต้อง ({LOCATION_FILE}): {error}"))?;
    // Old Mac installers selected Library/Application Support, which ESP-IDF
    // does not support as a build/SDK path. Preserve it and install separately.
    #[cfg(target_os = "macos")]
    if location.toolchain_dir == app_data.join("toolchain") && location.toolchain_dir.to_string_lossy().chars().any(char::is_whitespace) {
        return Ok(default_dir(app_data));
    }
    validate_location(app_data, &location.toolchain_dir)?;
    Ok(location.toolchain_dir)
}

pub fn save_location(app_data: &Path, dir: &Path) -> Result<(), String> {
    validate_location(app_data, dir)?;
    std::fs::create_dir_all(app_data)
        .map_err(|error| format!("สร้างโฟลเดอร์ตั้งค่า SDK ไม่ได้: {error}"))?;
    let data = serde_json::to_vec(&Location {
        toolchain_dir: dir.into(),
    })
    .map_err(|e| e.to_string())?;
    let mut temp = tempfile::NamedTempFile::new_in(app_data)
        .map_err(|error| format!("สร้างไฟล์ตำแหน่ง SDK ชั่วคราวไม่ได้: {error}"))?;
    use std::io::Write;
    temp.write_all(&data)
        .and_then(|_| temp.as_file().sync_all())
        .map_err(|error| format!("บันทึกตำแหน่ง SDK ไม่สำเร็จ: {error}"))?;
    temp.persist(app_data.join(LOCATION_FILE))
        .map_err(|error| {
            format!(
                "SDK อยู่ที่ {} แต่บันทึกตำแหน่งไม่สำเร็จ กรุณาลองติดตั้งตำแหน่งนี้อีกครั้ง: {error}",
                dir.display()
            )
        })?;
    Ok(())
}

fn existing_ancestor(path: &Path) -> Result<&Path, String> {
    path.ancestors()
        .find(|p| p.is_dir())
        .ok_or_else(|| format!("ไม่พบไดรฟ์หรือโฟลเดอร์ปลายทาง: {}", path.display()))
}

pub fn available_bytes(path: &Path) -> Result<u64, String> {
    let existing = existing_ancestor(path)?;
    platform::available_bytes(existing).map_err(|error| {
        format!(
            "ตรวจพื้นที่ว่างของ {} ไม่สำเร็จ: {error} กรุณาตรวจว่าไดรฟ์พร้อมใช้งาน",
            existing.display()
        )
    })
}

pub fn ensure_space(path: &Path, required: u64) -> Result<(), String> {
    let available = available_bytes(path)?;
    check_space(path, available, required)
}

fn check_space(path: &Path, available: u64, required: u64) -> Result<(), String> {
    if available < required {
        return Err(format!(
            "พื้นที่ไม่พอที่ {}: ว่าง {:.1} GiB ต้องมีอย่างน้อย {:.1} GiB กรุณาเลือกไดรฟ์อื่นหรือคืนพื้นที่แล้วลองใหม่",
            path.display(),
            available as f64 / GIB as f64,
            required as f64 / GIB as f64
        ));
    }
    Ok(())
}

pub fn make_plan(preferred: &Path, mut options: Vec<StorageOption>) -> StoragePlan {
    options.sort_by_key(|option| {
        (
            option.install_dir != preferred,
            std::cmp::Reverse(option.available_bytes.unwrap_or(0)),
        )
    });
    options.dedup_by(|a, b| a.install_dir == b.install_dir);
    let recommended_dir = options
        .iter()
        .find(|option| option.sufficient)
        .map(|option| option.install_dir.clone());
    let warning = match &recommended_dir {
        None => Some(
            "พื้นที่ไม่พอในทุกไดรฟ์ที่ตรวจพบ หรือไม่สามารถเขียนได้ กรุณาคืนพื้นที่หรือเลือกโฟลเดอร์ในไดรฟ์อื่นก่อนติดตั้ง"
                .into(),
        ),
        Some(dir) if dir != preferred => Some(format!(
            "ตำแหน่งเดิมมีพื้นที่ไม่พอหรือใช้งานไม่ได้ เปลี่ยนตำแหน่งแนะนำเป็น {}",
            dir.display()
        )),
        _ => None,
    };
    StoragePlan {
        required_bytes: REQUIRED_BYTES,
        options,
        recommended_dir,
        warning,
    }
}

pub fn inspect(app_data: &Path, directory: Option<&Path>) -> Result<StoragePlan, String> {
    let current = resolve_location(app_data)?;
    let preferred = directory
        .map(|base| alternative_dir(app_data, base))
        .unwrap_or_else(|| current.clone());
    let mut candidates = vec![preferred.clone(), current, default_dir(app_data)];
    for root in platform::roots()? {
        // AppData/current/custom directories already represent their drives.
        // Avoid a second C: entry that would require writing to the system drive root.
        if !candidates
            .iter()
            .any(|dir| dunce::simplified(dir).starts_with(&root))
        {
            candidates.push(alternative_dir(app_data, &root));
        }
    }
    if let Some(base) = directory {
        if !base.is_absolute() || !base.is_dir() {
            return Err("กรุณาเลือกโฟลเดอร์ที่มีอยู่จริงในไดรฟ์ปลายทาง".into());
        }
        candidates.insert(0, alternative_dir(app_data, base));
    }
    candidates.sort();
    candidates.dedup();
    let options = candidates
        .into_iter()
        .map(|dir| {
            let result = validate_location(app_data, &dir).and_then(|_| {
                #[cfg(target_os = "macos")]
                crate::macos_idf::check_path(&dir, "SDK install path")?;
                let free = available_bytes(&dir)?;
                // Probe permissions only when there is enough room. Remove the temporary probe on drop.
                if free >= REQUIRED_BYTES {
                    let parent = dir.parent().ok_or("ไม่มีโฟลเดอร์แม่สำหรับ SDK")?;
                    // Do not create SDK directories merely to inspect a drive.
                    tempfile::NamedTempFile::new_in(existing_ancestor(parent)?)
                        .map_err(|e| format!("ไม่มีสิทธิ์เขียนใน {}: {e}", parent.display()))?;
                }
                Ok(free)
            });
            match result {
                Ok(free) => StorageOption {
                    install_dir: dir,
                    available_bytes: Some(free),
                    sufficient: free >= REQUIRED_BYTES,
                    error: None,
                },
                Err(error) => StorageOption {
                    install_dir: dir,
                    available_bytes: None,
                    sufficient: false,
                    error: Some(error),
                },
            }
        })
        .collect();
    Ok(make_plan(&preferred, options))
}

pub fn select_install_dir(app_data: &Path, requested: Option<&Path>) -> Result<PathBuf, String> {
    let dir = match requested {
        Some(dir) => {
            validate_location(app_data, dir)?;
            dir.into()
        }
        None => {
            let plan = inspect(app_data, None)?;
            plan.recommended_dir
                .ok_or_else(|| plan.warning.unwrap_or_else(|| "ไม่มีไดรฟ์ที่ติดตั้ง SDK ได้".into()))?
        }
    };
    // Recheck at the time of download; the report displayed by UI can become stale.
    ensure_space(&dir, REQUIRED_BYTES)?;
    // Even on another drive, a tiny locator must remain writable in AppData.
    ensure_space(app_data, 1024 * 1024).map_err(|error| {
        format!("ต้องมีพื้นที่ว่างอย่างน้อย 1 MiB ในไดรฟ์ของ AppData เพื่อบันทึกตำแหน่ง SDK: {error}")
    })?;
    std::fs::create_dir_all(app_data).map_err(|e| format!("สร้างโฟลเดอร์ตั้งค่า SDK ไม่ได้: {e}"))?;
    {
        use std::io::Write;
        let mut probe = tempfile::NamedTempFile::new_in(app_data)
            .map_err(|e| format!("ไม่สามารถบันทึกตำแหน่ง SDK ใน AppData: {e}"))?;
        probe
            .write_all(b"SDK location preflight")
            .and_then(|_| probe.as_file().sync_all())
            .map_err(|e| format!("ไม่มีพื้นที่หรือสิทธิ์บันทึกตำแหน่ง SDK ใน AppData: {e}"))?;
    }
    Ok(dir)
}

pub fn required_extraction_bytes(zip_paths: &[PathBuf]) -> Result<u64, String> {
    let mut required = RESERVE_BYTES;
    for path in zip_paths {
        let file = std::fs::File::open(path).map_err(|e| format!("เปิด ZIP เพื่อตรวจพื้นที่ไม่ได้: {e}"))?;
        let mut archive =
            zip::ZipArchive::new(file).map_err(|e| format!("ตรวจขนาด ZIP ไม่ได้: {e}"))?;
        for i in 0..archive.len() {
            let entry = archive.by_index(i).map_err(|e| e.to_string())?;
            required = required
                .checked_add(entry.size())
                .ok_or("ขนาดไฟล์ SDK ใหญ่เกินขอบเขตที่ตรวจได้")?;
        }
    }
    Ok(required)
}

#[cfg(windows)]
mod platform {
    use super::*;
    use std::os::windows::ffi::OsStrExt;
    use windows_sys::Win32::Storage::FileSystem::{
        GetDiskFreeSpaceExW, GetDriveTypeW, GetLogicalDrives,
    };
    fn wide(path: &Path) -> Vec<u16> {
        path.as_os_str().encode_wide().chain(Some(0)).collect()
    }
    pub fn available_bytes(path: &Path) -> std::io::Result<u64> {
        let path = wide(path);
        let mut free = 0;
        // SAFETY: null-terminated UTF-16 and valid output pointer; optional outputs are null.
        if unsafe {
            GetDiskFreeSpaceExW(
                path.as_ptr(),
                &mut free,
                std::ptr::null_mut(),
                std::ptr::null_mut(),
            )
        } == 0
        {
            return Err(std::io::Error::last_os_error());
        }
        Ok(free)
    }
    pub fn roots() -> Result<Vec<PathBuf>, String> {
        // SAFETY: no pointers are passed.
        let mask = unsafe { GetLogicalDrives() };
        if mask == 0 {
            return Err(format!(
                "อ่านรายชื่อไดรฟ์ไม่ได้: {}",
                std::io::Error::last_os_error()
            ));
        }
        Ok((0..26)
            .filter(|i| mask & (1 << i) != 0)
            .filter_map(|i| {
                let root = PathBuf::from(format!("{}:\\", char::from(b'A' + i)));
                // SAFETY: valid null-terminated root path. 3 = DRIVE_FIXED, excludes network/CD/USB.
                (unsafe { GetDriveTypeW(wide(&root).as_ptr()) } == 3).then_some(root)
            })
            .collect())
    }
}

#[cfg(unix)]
mod platform {
    use super::*;
    use std::os::unix::ffi::OsStrExt;
    pub fn available_bytes(path: &Path) -> std::io::Result<u64> {
        let path = std::ffi::CString::new(path.as_os_str().as_bytes())?;
        let mut stat = std::mem::MaybeUninit::<libc::statvfs>::uninit();
        // SAFETY: C string and a correctly sized writable statvfs allocation.
        if unsafe { libc::statvfs(path.as_ptr(), stat.as_mut_ptr()) } != 0 {
            return Err(std::io::Error::last_os_error());
        }
        // SAFETY: statvfs succeeded and initialized the structure.
        let stat = unsafe { stat.assume_init() };
        (stat.f_bavail as u64)
            .checked_mul(stat.f_frsize as u64)
            .ok_or_else(|| std::io::Error::other("filesystem size overflow"))
    }
    pub fn roots() -> Result<Vec<PathBuf>, String> {
        // Other mounted filesystems can be selected with the folder picker.
        Ok(vec![])
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn option(dir: PathBuf, free: u64) -> StorageOption {
        StorageOption {
            install_dir: dir,
            available_bytes: Some(free),
            sufficient: free >= REQUIRED_BYTES,
            error: None,
        }
    }
    #[test]
    fn prefers_current_drive_and_uses_another_when_full() {
        let temp = tempfile::tempdir().unwrap();
        let c = temp.path().join("c");
        let d = temp.path().join("d");
        let current = make_plan(
            &c,
            vec![
                option(c.clone(), REQUIRED_BYTES),
                option(d.clone(), 100 * GIB),
            ],
        );
        assert_eq!(current.recommended_dir, Some(c.clone()));
        let fallback = make_plan(
            &c,
            vec![option(c.clone(), 2 * GIB), option(d.clone(), 100 * GIB)],
        );
        assert_eq!(fallback.recommended_dir, Some(d));
        assert!(fallback.warning.is_some());
        let full = make_plan(
            &c,
            vec![
                option(c.clone(), 2 * GIB),
                option(temp.path().join("d"), GIB),
            ],
        );
        assert!(full.recommended_dir.is_none());
        assert!(full.warning.unwrap().contains("ทุกไดรฟ์"));
    }
    #[test]
    fn boundary_and_unknown_space_never_bypass_preflight() {
        assert!(check_space(Path::new("SDK"), REQUIRED_BYTES - 1, REQUIRED_BYTES).is_err());
        assert!(check_space(Path::new("SDK"), REQUIRED_BYTES, REQUIRED_BYTES).is_ok());
        let plan = make_plan(
            Path::new("missing"),
            vec![StorageOption {
                install_dir: PathBuf::from("missing"),
                available_bytes: None,
                sufficient: false,
                error: Some("offline".into()),
            }],
        );
        assert!(plan.recommended_dir.is_none());
    }
    #[test]
    fn persists_thai_location_across_restarts_and_keeps_default_without_a_setting() {
        let temp = tempfile::tempdir().unwrap();
        let app_data = temp.path().join("ตั้งค่าแอป");
        assert_eq!(
            resolve_location(&app_data).unwrap(),
            default_dir(&app_data)
        );
        let selected = alternative_dir(&app_data, &temp.path().join("ไดรฟ์ สำรอง"));
        save_location(&app_data, &selected).unwrap();
        assert_eq!(resolve_location(&app_data).unwrap(), selected);
        // Atomic replacement of an existing setting must work on Windows too.
        save_location(&app_data, &app_data.join("toolchain")).unwrap();
        assert_eq!(
            resolve_location(&app_data).unwrap(),
            app_data.join("toolchain")
        );
        assert_eq!(std::fs::read_dir(app_data).unwrap().count(), 1);
    }
    #[test]
    fn rejects_arbitrary_or_parent_paths_and_corrupt_settings() {
        let temp = tempfile::tempdir().unwrap();
        let app_data = temp.path().join("app");
        assert!(validate_location(&app_data, temp.path()).is_err());
        assert!(validate_location(&app_data, Path::new("relative/toolchain")).is_err());
        assert!(validate_location(&app_data, &app_data.join("..").join("toolchain")).is_err());
        std::fs::create_dir_all(&app_data).unwrap();
        std::fs::write(app_data.join(LOCATION_FILE), b"{broken").unwrap();
        assert!(resolve_location(&app_data).is_err());
    }
    #[test]
    fn failing_to_validate_new_location_keeps_the_previous_setting() {
        let temp = tempfile::tempdir().unwrap();
        let data = temp.path().join("app");
        let selected = alternative_dir(&data, temp.path());
        save_location(&data, &selected).unwrap();
        assert!(save_location(&data, temp.path()).is_err());
        assert_eq!(resolve_location(&data).unwrap(), selected);
    }

    #[cfg(windows)]
    #[test]
    fn refuses_a_junction_in_the_managed_sdk_directory() {
        let temp = tempfile::tempdir().unwrap();
        let data = temp.path().join("app");
        let selected = alternative_dir(&data, temp.path());
        std::fs::create_dir_all(selected.parent().unwrap()).unwrap();
        let outside = temp.path().join("outside");
        std::fs::create_dir(&outside).unwrap();
        let output = std::process::Command::new("cmd")
            .args(["/C", "mklink", "/J"])
            .arg(&selected)
            .arg(&outside)
            .output()
            .unwrap();
        assert!(
            output.status.success(),
            "Cannot create test junction: {}",
            String::from_utf8_lossy(&output.stderr)
        );
        assert!(validate_location(&data, &selected)
            .unwrap_err()
            .contains("junction"));
        std::fs::remove_dir(&selected).unwrap();
        assert!(outside.is_dir());
    }

    #[test]
    fn real_storage_plan_reports_volumes_without_creating_an_sdk() {
        let temp = tempfile::tempdir().unwrap();
        let data = temp.path().join("app");
        let existed_default = default_dir(&data).exists();
        let plan = inspect(&data, None).unwrap();
        assert!(!plan.options.is_empty());
        for option in &plan.options {
            println!(
                "Storage: {} free={:?} sufficient={} error={:?}",
                option.install_dir.display(),
                option.available_bytes,
                option.sufficient,
                option.error
            );
            if option.install_dir == default_dir(&data) { assert_eq!(option.install_dir.exists(), existed_default); } else { assert!(!option.install_dir.exists()); }
        }
        assert!(!data.exists());
    }

    #[test]
    fn uses_exact_uncompressed_sizes_of_all_zips_with_reserve() {
        use std::io::Write;
        let temp = tempfile::tempdir().unwrap();
        let path = temp.path().join("sdk.zip");
        let mut zip = zip::ZipWriter::new(std::fs::File::create(&path).unwrap());
        zip.start_file("ภาษาไทย.txt", zip::write::SimpleFileOptions::default())
            .unwrap();
        zip.write_all(&[42; 2000]).unwrap();
        zip.finish().unwrap();
        assert_eq!(
            required_extraction_bytes(&[path.clone(), path]).unwrap(),
            RESERVE_BYTES + 4000
        );
    }
    #[test]
    fn reads_real_disk_space_without_changing_the_sdk_or_settings() {
        let temp = tempfile::tempdir().unwrap();
        let dir = temp.path().join("ยังไม่สร้าง").join("toolchain");
        assert!(available_bytes(&dir).unwrap() > 0);
        assert!(!dir.exists());
        #[cfg(windows)]
        assert!(!platform::roots().unwrap().is_empty());
    }
}
