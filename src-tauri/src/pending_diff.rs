//! Shared identity and transactional review operations. Approval remains explicit.
use crate::simulation_paths::PathScope;
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::{
    collections::{BTreeMap, HashMap},
    path::{Path, PathBuf},
};
pub type Diffs = HashMap<PathBuf, String>;
pub fn canonical_candidate(path: &Path) -> Option<PathBuf> {
    let absolute = if path.is_absolute() {
        path.to_path_buf()
    } else {
        std::env::current_dir().ok()?.join(path)
    };
    let mut ancestor = absolute.as_path();
    let mut missing = Vec::new();
    while !ancestor.exists() {
        missing.push(ancestor.file_name()?.to_os_string());
        ancestor = ancestor.parent()?;
    }
    let mut result = ancestor.canonicalize().ok()?;
    for part in missing.into_iter().rev() {
        result.push(part);
    }
    Some(result)
}
fn keys(diffs: &Diffs, path: &Path) -> Vec<PathBuf> {
    let target = canonical_candidate(path);
    diffs
        .keys()
        .filter(|key| {
            **key == path
                || target
                    .as_ref()
                    .is_some_and(|target| canonical_candidate(key).as_ref() == Some(target))
        })
        .cloned()
        .collect()
}
pub fn get(diffs: &Diffs, path: &Path) -> Result<Option<String>, String> {
    let found = keys(diffs, path);
    let Some(first) = found.first().and_then(|key| diffs.get(key)) else {
        return Ok(None);
    };
    if found.iter().any(|key| diffs.get(key) != Some(first)) {
        return Err("มีข้อเสนอหลายรุ่นสำหรับไฟล์เดียวกัน กรุณาให้ AI เสนอไฟล์ล่าสุดใหม่ก่อนตรวจรับ".into());
    }
    Ok(Some(first.clone()))
}
pub fn propose(diffs: &mut Diffs, path: &Path, content: String) {
    for key in keys(diffs, path) {
        diffs.remove(&key);
    }
    diffs.insert(
        canonical_candidate(path).unwrap_or_else(|| path.to_path_buf()),
        content,
    );
}
pub fn accept_with<F>(
    diffs: &mut Diffs,
    path: &Path,
    expected: Option<&str>,
    write: F,
) -> Result<(), String>
where
    F: FnOnce(&Path, &str) -> Result<(), String>,
{
    let content = get(diffs, path)?.ok_or("No pending diff found for this file.")?;
    if expected.is_some_and(|expected| expected != content) {
        return Err("ข้อเสนอเปลี่ยนหลังเปิด Diff กรุณาตรวจรุ่นล่าสุดก่อนกด Keep".into());
    }
    let matched = keys(diffs, path);
    let target = canonical_candidate(path).ok_or("Cannot resolve the reviewed file path")?;
    write(&target, &content)?;
    // Never lose a proposal when a write fails.
    for key in matched {
        diffs.remove(&key);
    }
    Ok(())
}
pub fn reject(diffs: &mut Diffs, path: &Path, expected: Option<&str>) -> Result<(), String> {
    let content = get(diffs, path)?.ok_or("No pending diff found for this file.")?;
    if expected.is_some_and(|expected| expected != content) {
        return Err("ข้อเสนอเปลี่ยนหลังเปิด Diff กรุณาตรวจรุ่นล่าสุดก่อนกด Undo".into());
    }
    for key in keys(diffs, path) {
        diffs.remove(&key);
    }
    Ok(())
}
pub fn atomic_write(path: &Path, content: &str) -> Result<(), String> {
    use std::io::Write;
    let parent = path
        .parent()
        .ok_or("Reviewed file has no parent directory")?;
    let mut temp = tempfile::NamedTempFile::new_in(parent)
        .map_err(|e| format!("สร้างไฟล์ชั่วคราวเพื่อ Keep ไม่สำเร็จ: {e}"))?;
    temp.write_all(content.as_bytes())
        .and_then(|_| temp.as_file().sync_all())
        .map_err(|e| format!("บันทึก Keep ไม่สำเร็จ ข้อเสนอยังรอตรวจรับ: {e}"))?;
    let permissions = std::fs::metadata(path)
        .map_err(|e| format!("ไฟล์ที่ตรวจรับเปลี่ยนหรืออ่านสิทธิ์ไม่ได้: {e}"))?
        .permissions();
    temp.as_file()
        .set_permissions(permissions)
        .map_err(|e| format!("คงสิทธิ์ไฟล์เดิมไม่สำเร็จ: {e}"))?;
    temp.persist(path)
        .map_err(|e| format!("แทนที่ไฟล์หลัง Keep ไม่สำเร็จ ข้อเสนอยังรอตรวจรับ: {e}"))?;
    Ok(())
}
#[derive(Clone, Serialize, Debug)]
pub struct Review {
    pub path: String,
    pub relative_path: String,
}
pub fn list(scope: &PathScope, diffs: &Diffs) -> Vec<Review> {
    let mut result = BTreeMap::new();
    for path in diffs.keys() {
        let Some(absolute) = canonical_candidate(path) else {
            continue;
        };
        let Ok(relative) = absolute.strip_prefix(&scope.root) else {
            continue;
        };
        let relative = relative.to_string_lossy().replace('\\', "/");
        if relative != crate::simulation_project::SOURCE_PATH
            && !crate::simulation_project::is_firmware_input(&relative)
        {
            continue;
        }
        result.insert(
            relative.clone(),
            Review {
                path: dunce::simplified(&absolute).to_string_lossy().into(),
                relative_path: relative,
            },
        );
    }
    result.into_values().collect()
}
#[derive(Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct RecoveryEntry {
    pub path: String,
    pub content: String,
    pub base_sha256: String,
}
fn hash(bytes: &[u8]) -> String {
    format!("{:x}", Sha256::digest(bytes))
}
pub fn restore(
    scope: &PathScope,
    diffs: &mut Diffs,
    entries: &[RecoveryEntry],
) -> Result<Vec<Review>, String> {
    if entries.len() > 128
        || entries.iter().map(|e| e.content.len()).sum::<usize>() > 4 * 1024 * 1024
    {
        return Err("Recovery snapshot exceeds review limits".into());
    }
    let mut prepared = BTreeMap::new();
    for entry in entries {
        let absolute =
            canonical_candidate(Path::new(&entry.path)).ok_or("Cannot resolve recovery path")?;
        let relative = absolute
            .strip_prefix(&scope.root)
            .map_err(|_| "Recovery entry is outside this project")?
            .to_string_lossy()
            .replace('\\', "/");
        let destination = scope.resolve(&relative, false)?;
        let bytes = std::fs::read(&destination).map_err(|e| e.to_string())?;
        if hash(&bytes) == hash(entry.content.as_bytes()) {
            continue;
        } // Already kept.
        if hash(&bytes) != entry.base_sha256 {
            return Err(format!(
                "ไฟล์ {relative} เปลี่ยนหลังสำรอง Diff จึงไม่กู้คืนทับสถานะใหม่ ไฟล์สำรองยังอยู่"
            ));
        }
        if get(diffs, &destination)?.is_some_and(|current| current != entry.content) {
            return Err(format!("มี Diff ใหม่ของ {relative} อยู่แล้ว จึงไม่แทนที่ด้วยสำรอง"));
        }
        if let Some(previous) = prepared.insert(destination, entry.content.clone()) {
            if previous != entry.content {
                return Err("Recovery snapshot contains conflicting versions".into());
            }
        }
    }
    for path in prepared.keys() {
        let relative = path
            .strip_prefix(&scope.root)
            .unwrap()
            .to_string_lossy()
            .replace('\\', "/");
        if crate::simulation_project::is_companion_path(&relative) {
            crate::simulation_project::register_companion_review(&scope.root, &relative)?;
        }
    }
    for (path, content) in prepared {
        propose(diffs, &path, content);
    }
    Ok(list(scope, diffs))
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn windows_aliases_use_the_same_review_and_remove_all_matching_keys() {
        let temp = tempfile::tempdir().unwrap();
        let path = temp.path().join("ภาษาไทย.c");
        std::fs::write(&path, "old").unwrap();
        let canonical = path.canonicalize().unwrap();
        let mut diffs = Diffs::from([
            (canonical.clone(), "new".into()),
            (path.clone(), "new".into()),
        ]);
        #[cfg(windows)]
        assert_ne!(canonical, path);
        assert_eq!(get(&diffs, &path).unwrap().as_deref(), Some("new"));
        accept_with(&mut diffs, &path, Some("new"), atomic_write).unwrap();
        assert!(diffs.is_empty());
        assert_eq!(std::fs::read_to_string(path).unwrap(), "new");
    }
    #[test]
    fn failed_keep_retains_the_proposal_and_original_file() {
        let temp = tempfile::tempdir().unwrap();
        let path = temp.path().join("main.c");
        std::fs::write(&path, "old").unwrap();
        let mut diffs = Diffs::new();
        propose(&mut diffs, &path, "new".into());
        assert!(accept_with(&mut diffs, &path, Some("new"), |_, _| Err(
            "disk full".into()
        ))
        .is_err());
        assert_eq!(get(&diffs, &path).unwrap().as_deref(), Some("new"));
        assert_eq!(std::fs::read_to_string(&path).unwrap(), "old");
        assert!(accept_with(&mut diffs, &path, Some("stale"), atomic_write).is_err());
        assert!(reject(&mut diffs, &path, Some("stale")).is_err());
        reject(&mut diffs, &path, Some("new")).unwrap();
        assert!(diffs.is_empty());
    }
    #[test]
    fn failed_atomic_replace_does_not_discard_review() {
        let temp = tempfile::tempdir().unwrap();
        let path = temp.path().join("occupied");
        std::fs::create_dir(&path).unwrap();
        let mut diffs = Diffs::new();
        propose(&mut diffs, &path, "new".into());
        assert!(accept_with(&mut diffs, &path, Some("new"), atomic_write).is_err());
        assert!(path.is_dir());
        assert!(get(&diffs, &path).unwrap().is_some());
    }
    #[test]
    fn recovery_only_restores_pending_state_never_accepts_and_checks_base_hash() {
        let temp = tempfile::tempdir().unwrap();
        std::fs::create_dir(temp.path().join("main")).unwrap();
        let path = temp.path().join("main/main.c");
        std::fs::write(&path, "old").unwrap();
        let scope = PathScope::new(temp.path()).unwrap();
        let mut diffs = Diffs::new();
        let entries = vec![RecoveryEntry {
            path: path.to_string_lossy().into(),
            content: "new".into(),
            base_sha256: hash(b"old"),
        }];
        let reviews = restore(&scope, &mut diffs, &entries).unwrap();
        assert_eq!(reviews.len(), 1);
        assert_eq!(reviews[0].relative_path, "main/main.c");
        assert_eq!(std::fs::read_to_string(&path).unwrap(), "old");
        restore(&scope, &mut diffs, &entries).unwrap();
        assert_eq!(diffs.len(), 1);
        diffs.clear();
        std::fs::write(&path, "user edit").unwrap();
        assert!(restore(&scope, &mut diffs, &entries).is_err());
        assert!(diffs.is_empty());
    }
    #[test]
    fn identical_filenames_in_other_projects_are_not_merged_or_listed() {
        let one = tempfile::tempdir().unwrap();
        let two = tempfile::tempdir().unwrap();
        let a = one.path().join("CMakeLists.txt");
        let b = two.path().join("CMakeLists.txt");
        std::fs::write(&a, "a").unwrap();
        std::fs::write(&b, "b").unwrap();
        let mut diffs = Diffs::new();
        propose(&mut diffs, &a, "new a".into());
        propose(&mut diffs, &b, "new b".into());
        let reviews = list(&PathScope::new(one.path()).unwrap(), &diffs);
        assert_eq!(reviews.len(), 1);
        reject(&mut diffs, &a, None).unwrap();
        assert_eq!(get(&diffs, &b).unwrap().as_deref(), Some("new b"));
    }
}
