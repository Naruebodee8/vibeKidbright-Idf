use std::path::{Path, PathBuf};

#[derive(Clone, Debug)]
pub(crate) struct PathScope {
    pub(crate) root: PathBuf,
}

impl PathScope {
    pub(crate) fn new(root: &Path) -> Result<Self, String> {
        let root = root
            .canonicalize()
            .map_err(|e| format!("Cannot resolve allowed root: {e}"))?;
        if !root.is_dir() {
            return Err("Allowed root is not a directory".into());
        }
        Ok(Self { root })
    }

    pub(crate) fn pinned(root: &Path) -> Result<Self, String> {
        let scope = Self::new(root)?;
        if scope.root != root {
            return Err("Workspace root changed; reopen the project".into());
        }
        Ok(scope)
    }

    pub(crate) fn resolve(&self, relative: &str, allow_missing: bool) -> Result<PathBuf, String> {
        Self::pinned(&self.root)?;
        let relative = validate_relative_path(relative)?;
        let target = self.root.join(relative);
        let mut ancestor = target.as_path();
        let mut missing = Vec::new();
        loop {
            match std::fs::symlink_metadata(ancestor) {
                Ok(_) => break,
                Err(e) if e.kind() == std::io::ErrorKind::NotFound && allow_missing => {
                    missing.push(
                        ancestor
                            .file_name()
                            .ok_or("Invalid file path")?
                            .to_os_string(),
                    );
                    ancestor = ancestor.parent().ok_or("Path has no allowed ancestor")?;
                }
                Err(e) => return Err(format!("Cannot inspect path: {e}")),
            }
        }
        let mut resolved = ancestor
            .canonicalize()
            .map_err(|e| format!("Cannot resolve path: {e}"))?;
        if !resolved.starts_with(&self.root) {
            return Err("Path escapes the allowed root".into());
        }
        for part in missing.into_iter().rev() {
            resolved.push(part);
        }
        if !resolved.starts_with(&self.root) {
            return Err("Path escapes the allowed root".into());
        }
        Ok(resolved)
    }
}

pub(crate) fn validate_relative_path(path: &str) -> Result<PathBuf, String> {
    // Interpret both separators on every platform; reject Windows prefixes and ADS too.
    let normalized = path.replace('\\', "/");
    if normalized.is_empty()
        || normalized.starts_with('/')
        || normalized.contains(':')
        || normalized.contains('\0')
        || normalized.split('/').any(|part| part == "..")
    {
        return Err(
            "Expected a relative path without '..', absolute paths or drive prefixes".into(),
        );
    }
    Ok(PathBuf::from(normalized))
}

pub(crate) fn safe_traversal_entry(root: &Path, path: &Path) -> bool {
    let Ok(metadata) = std::fs::symlink_metadata(path) else {
        return false;
    };
    if metadata.file_type().is_symlink() {
        return false;
    }
    #[cfg(windows)]
    {
        use std::os::windows::fs::MetadataExt;
        if metadata.file_attributes() & 0x400 != 0 {
            return false;
        }
    }
    let Ok(relative) = path.strip_prefix(root) else {
        return false;
    };
    PathScope::pinned(root)
        .and_then(|s| s.resolve(&relative.to_string_lossy(), false))
        .is_ok()
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn boundaries_accept_thai_and_reject_parent_absolute_drive_and_sibling_paths() {
        let root = tempfile::tempdir().unwrap();
        let outside = tempfile::tempdir().unwrap();
        let scope = PathScope::new(root.path()).unwrap();
        for path in [
            "../outside",
            "/absolute",
            "C:/outside",
            "\\\\host/share",
            "file:stream",
            "main/../../outside",
        ] {
            assert!(scope.resolve(path, true).is_err(), "accepted {path}");
        }
        let path = scope.resolve("simulation/ตัวอย่าง.sim.js", true).unwrap();
        assert!(path.starts_with(&scope.root));
        std::fs::write(root.path().join("ไทย.c"), "ไทย").unwrap();
        assert!(scope.resolve("ไทย.c", false).is_ok());
        #[cfg(windows)]
        assert!(std::process::Command::new("cmd.exe")
            .args(["/C", "mklink", "/J"])
            .arg(root.path().join("linked"))
            .arg(outside.path())
            .output()
            .unwrap()
            .status
            .success());
        #[cfg(unix)]
        std::os::unix::fs::symlink(outside.path(), root.path().join("linked")).unwrap();
        assert!(scope.resolve("linked/secret.c", true).is_err());
        assert!(!safe_traversal_entry(
            &scope.root,
            &root.path().join("linked")
        ));
    }
}
