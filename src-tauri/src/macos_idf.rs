//! Native macOS ESP-IDF installation. No Windows ZIPs or relocated virtualenvs.
//! Also used by the macOS firmware smoke test, independently of Tauri/UI.
use std::collections::BTreeMap;
use std::io::{BufRead, BufReader};
use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};
use std::sync::atomic::{AtomicBool, Ordering};
use std::time::{Duration, Instant};

pub const VERSION: &str = "v5.5.4";
const REPOSITORY: &str = "https://github.com/espressif/esp-idf.git";
const OWNER: &str = "vibekidbright-native-macos-v1";
pub type Environment = BTreeMap<String, String>;

#[derive(Clone, Debug)]
pub struct Sdk { pub idf: PathBuf, pub tools: PathBuf }
#[derive(Clone, Debug)]
pub struct Ready { pub python: PathBuf, pub env: Environment }

pub fn host_arch(arch: &str) -> Result<&'static str, String> {
    match arch { "aarch64" | "arm64" => Ok("arm64"), "x86_64" => Ok("x86_64"),
        _ => Err(format!("Unsupported macOS architecture: {arch}")) }
}
pub fn default_root(home: &Path) -> PathBuf { home.join(".vibekidbright").join("toolchain") }
pub fn paths(root: &Path, arch: &str) -> Result<Sdk, String> {
    let base = root.join("macos").join(host_arch(arch)?);
    Ok(Sdk { idf: base.join("frameworks").join(format!("esp-idf-{VERSION}")), tools: base.join("tools") })
}
pub fn check_path(path: &Path, label: &str) -> Result<(), String> {
    if !path.is_absolute() || path.components().any(|part| matches!(part, std::path::Component::ParentDir)) {
        return Err(format!("{label} must be an absolute path without ..: {}", path.display()));
    }
    if path.to_string_lossy().chars().any(char::is_whitespace) {
        return Err(format!("{label} contains spaces: {}. ESP-IDF requires SDK/project paths without spaces. Choose a location such as ~/esp or ~/.vibekidbright/toolchain.", path.display()));
    }
    Ok(())
}

fn system_path() -> String {
    let mut dirs = vec![PathBuf::from("/opt/homebrew/bin"), PathBuf::from("/usr/local/bin"), PathBuf::from("/usr/bin"), PathBuf::from("/bin"), PathBuf::from("/usr/sbin"), PathBuf::from("/sbin")];
    if let Some(old) = std::env::var_os("PATH") { dirs.extend(std::env::split_paths(&old)); }
    std::env::join_paths(dirs).unwrap_or_default().to_string_lossy().into_owned()
}
fn base_env(sdk: &Sdk) -> Environment {
    BTreeMap::from([
        ("IDF_PATH".into(), sdk.idf.to_string_lossy().into_owned()),
        ("IDF_TOOLS_PATH".into(), sdk.tools.to_string_lossy().into_owned()),
        ("PATH".into(), system_path()), ("PYTHONUTF8".into(), "1".into()),
        ("PYTHONIOENCODING".into(), "utf-8".into()), ("IDF_TARGET".into(), "esp32".into()),
    ])
}
fn command(program: &Path, args: &[&str], env: &Environment) -> Command {
    let mut cmd = Command::new(program);
    cmd.args(args).envs(env).env_remove("PYTHONHOME").env_remove("PYTHONPATH")
        .env_remove("VIRTUAL_ENV").env_remove("IDF_PYTHON_ENV_PATH");
    // The selected venv is restored after inherited values have been cleared.
    if let Some(venv) = env.get("IDF_PYTHON_ENV_PATH") { cmd.env("IDF_PYTHON_ENV_PATH", venv).env("VIRTUAL_ENV", venv); }
    cmd
}
fn terminate_tree(child: &mut std::process::Child) {
    #[cfg(unix)]
    unsafe { libc::kill(-(child.id() as i32), libc::SIGKILL); }
    let _ = child.kill();
    let _ = child.wait();
}

/// Captured output is bounded. Readers drain both pipes while cancellation kills
/// the entire process group (git/pip/compiler children included).
pub fn run(mut cmd: Command, label: &str, cancel: &AtomicBool, timeout: Duration,
           progress: &(dyn Fn(&str) + Sync)) -> Result<String, String> {
    if cancel.load(Ordering::SeqCst) { return Err("Cancelled".into()); }
    #[cfg(unix)]
    { use std::os::unix::process::CommandExt; cmd.process_group(0); }
    cmd.stdout(Stdio::piped()).stderr(Stdio::piped());
    let mut child = cmd.spawn().map_err(|error| format!("{label}: {error}"))?;
    let stdout = child.stdout.take().ok_or("Cannot capture installer stdout")?;
    let stderr = child.stderr.take().ok_or("Cannot capture installer stderr")?;
    std::thread::scope(|scope| {
        fn drain<R: std::io::Read>(pipe: R, progress: &(dyn Fn(&str) + Sync)) -> String {
            let mut lines = std::collections::VecDeque::new();
            for line in BufReader::new(pipe).lines().map_while(Result::ok) {
                let bounded: String = line.chars().take(32768).collect();
                progress(&bounded.chars().take(350).collect::<String>());
                if lines.len() == 120 { lines.pop_front(); }
                lines.push_back(bounded);
            }
            lines.into_iter().collect::<Vec<_>>().join("\n")
        }
        let out = scope.spawn(|| drain(stdout, progress));
        let err = scope.spawn(|| drain(stderr, progress));
        let started = Instant::now();
        let status = loop {
            if cancel.load(Ordering::SeqCst) { terminate_tree(&mut child); break Err("Cancelled".to_string()); }
            if started.elapsed() > timeout { terminate_tree(&mut child); break Err(format!("{label} timed out; check network/prerequisites and retry.")); }
            match child.try_wait() {
                Ok(Some(status)) => break Ok(status),
                Ok(None) => std::thread::sleep(Duration::from_millis(40)),
                Err(error) => { terminate_tree(&mut child); break Err(format!("{label}: {error}")); }
            }
        };
        let output = out.join().map_err(|_| "Installer stdout reader failed")?;
        let errors = err.join().map_err(|_| "Installer stderr reader failed")?;
        let status = status?;
        if !status.success() {
            let tail: String = format!("{errors}\n{output}").chars().rev().take(5000).collect::<String>().chars().rev().collect();
            return Err(format!("{label} failed (exit {:?}): {tail}", status.code()));
        }
        Ok(output)
    })
}

pub fn find_python(tools: &Path) -> Result<PathBuf, String> {
    let root = tools.join("python_env");
    let mut envs: Vec<_> = std::fs::read_dir(&root).map_err(|_| format!("Native macOS ESP-IDF Python environment is missing at {}. Use Setup / Repair ESP-IDF to install the Mac tools; a Windows Scripts/python.exe environment cannot run on macOS.",root.display()))?
        .flatten().map(|entry| entry.path()).filter(|path| path.file_name().is_some_and(|name| {
            let name = name.to_string_lossy(); name.starts_with("idf") && name.contains("_py") && name.ends_with("_env")
        })).collect();
    envs.sort(); envs.reverse();
    for env in envs { for name in ["python", "python3"] { let python=env.join("bin").join(name); if python.is_file() {return Ok(python);} } }
    Err(format!("No runnable macOS Python environment in {} (expected idf*_py*_env/bin/python). Reinstall with Setup / Repair ESP-IDF; do not copy a Windows virtualenv.",root.display()))
}

pub fn parse_export(text: &str, inherited_path: &str) -> Result<Environment, String> {
    let mut env = Environment::new();
    for line in text.lines() {
        let Some((name, value)) = line.split_once('=') else { continue; };
        if ["PATH","IDF_PYTHON_ENV_PATH","OPENOCD_SCRIPTS","ESP_ROM_ELF_DIR","ESP_IDF_VERSION"].contains(&name) {
            if value.contains('\0') { return Err("Invalid NUL in ESP-IDF environment".into()); }
            env.insert(name.into(), if name=="PATH" {value.replace("${PATH}",inherited_path).replace("$PATH",inherited_path)} else {value.into()});
        }
    }
    if !env.get("PATH").is_some_and(|path| !path.is_empty()) { return Err("ESP-IDF export did not provide PATH; repair the native tools.".into()); }
    Ok(env)
}
pub fn environment(sdk: &Sdk, cancel: &AtomicBool) -> Result<Ready, String> {
    check_path(&sdk.idf,"ESP-IDF path")?; check_path(&sdk.tools,"ESP-IDF tools path")?;
    let python=find_python(&sdk.tools)?;
    let mut env=base_env(sdk);
    let venv=python.parent().and_then(Path::parent).ok_or("Invalid Python venv path")?;
    env.insert("IDF_PYTHON_ENV_PATH".into(),venv.to_string_lossy().into_owned());
    let tools_py=sdk.idf.join("tools/idf_tools.py");
    let args=[tools_py.to_str().ok_or("Invalid tools path")?,"export","--format","key-value"];
    let output=run(command(&python,&args,&env),"Export ESP-IDF environment",cancel,Duration::from_secs(30),&|_| {})?;
    env.extend(parse_export(&output,&env["PATH"])?);
    // Never accept an exporter pointing at another machine's Python environment.
    env.insert("IDF_PYTHON_ENV_PATH".into(),venv.to_string_lossy().into_owned());
    Ok(Ready {python,env})
}
pub fn validate(sdk: &Sdk, cancel: &AtomicBool) -> Result<Ready, String> {
    let ready=environment(sdk,cancel)?;
    let tools_py=sdk.idf.join("tools/idf_tools.py");
    let args=[tools_py.to_str().ok_or("Invalid tools path")?,"check-python-dependencies"];
    run(command(&ready.python,&args,&ready.env),"Check ESP-IDF Python packages",cancel,Duration::from_secs(40),&|_| {})?;
    let idf_py=sdk.idf.join("tools/idf.py");
    run(command(&ready.python,&[idf_py.to_str().ok_or("Invalid IDF path")?,"--version"],&ready.env),"Check idf.py",cancel,Duration::from_secs(30),&|_| {})?;
    for name in ["cmake","ninja","xtensa-esp32-elf-gcc"] {
        run(command(Path::new(name),&["--version"],&ready.env),name,cancel,Duration::from_secs(20),&|_| {})?;
    }
    Ok(ready)
}
pub fn build_command(sdk: &Sdk, ready: &Ready, project: &Path, args: &[String]) -> Result<Command,String> {
    check_path(project,"Project path")?;
    let mut cmd=command(&ready.python,&[],&ready.env);
    cmd.arg(sdk.idf.join("tools/idf.py")).args(args).current_dir(project);
    Ok(cmd)
}

fn host_python(sdk: &Sdk, cancel: &AtomicBool) -> Result<PathBuf,String> {
    // Prefer a supported, explicitly installed Python over a newer Homebrew default.

    let mut candidates=Vec::new();
    if let Some(selected)=std::env::var_os("VIBEKIDBRIGHT_BOOTSTRAP_PYTHON") {candidates.push(PathBuf::from(selected));}
    candidates.extend([
        "/opt/homebrew/opt/python@3.12/bin/python3.12", "/usr/local/opt/python@3.12/bin/python3.12",
        "/Library/Frameworks/Python.framework/Versions/3.12/bin/python3",
        "/opt/homebrew/bin/python3", "/usr/local/bin/python3", "/usr/bin/python3", "python3"
    ].into_iter().map(PathBuf::from));
    if let Ok(versions)=std::fs::read_dir("/Library/Frameworks/Python.framework/Versions") {
        for version in versions.flatten() {candidates.push(version.path().join("bin/python3"));}
    }
    let env=base_env(sdk);
    let expected=host_arch(std::env::consts::ARCH)?;
    for candidate in candidates {
        if cancel.load(Ordering::SeqCst) { return Err("Cancelled".into()); }
        let args=["-c","import sys,platform,json;print(json.dumps({'version':list(sys.version_info[:2]),'arch':platform.machine(),'exe':sys.executable}))"];
        if let Ok(text)=run(command(&candidate,&args,&env),"Check host Python",cancel,Duration::from_secs(10),&|_| {}) {
            if let Ok(info)=serde_json::from_str::<serde_json::Value>(text.trim()) {
                let valid=info["version"][0].as_u64()==Some(3) && info["version"][1].as_u64().is_some_and(|minor|minor>=10);
                if valid && info["arch"].as_str().and_then(|arch|host_arch(arch).ok())==Some(expected) {
                    if let Some(exe)=info["exe"].as_str() {return Ok(PathBuf::from(exe));}
                }
            }
        }
    }
    Err(format!("macOS requires Python 3.10 or newer for {expected}. Install a matching Python 3 from python.org or Homebrew, then click Retry Installation. Python from Windows or a different architecture cannot be used."))
}

/// Installation stays at its FINAL absolute path. Only the source checkout is
/// staged; Python virtualenvs are created afterwards and never renamed/patched.
pub fn install(root: &Path, cancel: &AtomicBool, progress: &(dyn Fn(u8,&str)+Sync)) -> Result<Sdk,String> {
    if !cfg!(target_os="macos") {return Err("Native macOS installation must run on a Mac".into());}
    check_path(root,"SDK install path")?;
    let sdk=paths(root,std::env::consts::ARCH)?;
    // Never let a redirected managed subdirectory turn pip's --reinstall into
    // writes outside this app-owned native SDK.
    for path in [root.to_path_buf(),root.join("macos"),sdk.tools.parent().unwrap().to_path_buf(),
        sdk.tools.clone(),sdk.tools.join("python_env"),sdk.idf.parent().unwrap().to_path_buf(),sdk.idf.clone()] {
        if std::fs::symlink_metadata(&path).is_ok_and(|m|m.file_type().is_symlink()) {
            return Err(format!("Native SDK directory is a symlink: {}. Choose an app-owned non-redirected location.",path.display()));
        }
    }
    let env=base_env(&sdk);
    run(command(Path::new("/usr/bin/xcode-select"),&["-p"],&env),"Xcode Command Line Tools",cancel,Duration::from_secs(10),&|_| {})
        .map_err(|error|format!("{error}\nInstall Apple's Command Line Tools with xcode-select --install, finish that installation, then retry."))?;
    let python=host_python(&sdk,cancel)?;
    if validate(&sdk,cancel).is_ok() {progress(99,"Native macOS tools verified");return Ok(sdk);}
    if cancel.load(Ordering::SeqCst) { return Err("Cancelled".into()); }
    let owner=sdk.tools.parent().ok_or("Invalid native SDK root")?;
    std::fs::create_dir_all(owner).map_err(|e|e.to_string())?;
    let marker=owner.join(".vibekidbright-native-sdk");
    if !marker.exists() && std::fs::read_dir(owner).map_err(|e|e.to_string())?.next().is_some() {
        return Err(format!("Existing native SDK is not owned by this installer: {}. Choose a new SDK location; existing files are preserved.",owner.display()));
    }
    if let Ok(envs)=std::fs::read_dir(sdk.tools.join("python_env")) {
        for env in envs.flatten() {
            if env.file_type().map_err(|e|e.to_string())?.is_symlink() {
                return Err(format!("Native Python environment is a redirected directory: {}. Choose a new SDK location.",env.path().display()));
            }
        }
    }
    if marker.exists() {
        if std::fs::read_to_string(&marker).map_err(|e|e.to_string())?!=OWNER {return Err("Unrecognized native SDK ownership marker; choose a new SDK location.".into());}
    } else {std::fs::write(&marker,OWNER).map_err(|e|e.to_string())?;}
    let frameworks=sdk.idf.parent().ok_or("Invalid framework path")?;
    std::fs::create_dir_all(frameworks).map_err(|e|e.to_string())?;
    if !sdk.idf.exists() {
        let staging=frameworks.join(format!("esp-idf-{VERSION}.installing"));
        if std::fs::symlink_metadata(&staging).is_ok_and(|m|m.file_type().is_symlink()) {
            return Err("Interrupted SDK checkout is a symlink; choose a new SDK location.".into());
        }
        progress(5,"Downloading native ESP-IDF source and submodules");
        if !staging.exists() {
            let args=["clone","--recursive","--shallow-submodules","--depth","1","--branch",VERSION,REPOSITORY,staging.to_str().ok_or("Invalid checkout path")?];
            run(command(Path::new("/usr/bin/git"),&args,&env),"Download ESP-IDF",cancel,Duration::from_secs(2700),&|line|progress(10,line))?;
        } else {
            if !staging.join(".git").is_dir() {return Err(format!("Interrupted checkout at {} is incomplete. Choose a new SDK location to retry; existing files are preserved.",staging.display()));}
            for args in [vec!["fetch","--depth","1","origin",VERSION],vec!["checkout","--detach","FETCH_HEAD"]] {
                let mut resume=command(Path::new("/usr/bin/git"),&args,&env);resume.current_dir(&staging);
                run(resume,"Resume ESP-IDF checkout",cancel,Duration::from_secs(2700),&|line|progress(12,line))?;
            }
            let mut submodules=command(Path::new("/usr/bin/git"),&["submodule","update","--init","--recursive","--depth","1"],&env);
            submodules.current_dir(&staging);
            run(submodules,"Complete ESP-IDF submodules",cancel,Duration::from_secs(2700),&|line|progress(15,line))?;
        }
        if !staging.join("tools/idf_tools.py").is_file() || !staging.join("tools/idf.py").is_file() {return Err("ESP-IDF checkout is incomplete; retry installation.".into());}
        std::fs::rename(&staging,&sdk.idf).map_err(|e|format!("Cannot activate ESP-IDF checkout: {e}"))?;
    }
    std::fs::create_dir_all(&sdk.tools).map_err(|e|e.to_string())?;
    let tool_script=sdk.idf.join("tools/idf_tools.py");
    let script=tool_script.to_str().ok_or("Invalid tool script path")?;
    for (percent,label,args) in [
        (35,"Installing compiler tools for this Mac",vec![script,"install","--targets=esp32"]),
        (55,"Installing native CMake and Ninja",vec![script,"install","cmake","ninja"]),
        (75,"Creating Python environment on this Mac",vec![script,"install-python-env","--features=core","--reinstall"]),
    ] {
        progress(percent,label);
        run(command(&python,&args,&env),label,cancel,Duration::from_secs(2700),&|line|progress(percent,line))?;
    }
    progress(95,"Verifying Python, packages, CMake, Ninja and compiler");
    validate(&sdk,cancel)?;
    progress(99,"Native macOS ESP-IDF is ready");
    Ok(sdk)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test] fn mac_architectures_and_install_paths_are_separate() {
        let root=Path::new(if cfg!(windows){"C:/sdk"}else{"/sdk"});
        assert_ne!(paths(root,"aarch64").unwrap().tools,paths(root,"x86_64").unwrap().tools);
        assert_eq!(paths(root,"aarch64").unwrap().idf,paths(root,"arm64").unwrap().idf);
        assert!(paths(root,"i686").is_err());
        assert!(default_root(Path::new("/fixture/home")).to_string_lossy().contains(".vibekidbright"));
        assert!(check_path(Path::new(if cfg!(windows){"C:/fixture/Library/Application Support/sdk"}else{"/fixture/Library/Application Support/sdk"}),"SDK").unwrap_err().contains("spaces"));
    }
    #[test] fn windows_venv_and_old_ready_marker_do_not_make_mac_sdk_ready() {
        let tmp=tempfile::tempdir().unwrap();
        std::fs::write(tmp.path().join(".toolchain_ready"),"old").unwrap();
        let env=tmp.path().join("python_env/idf5.5_py3.11_env");std::fs::create_dir_all(env.join("Scripts")).unwrap();
        std::fs::write(env.join("Scripts/python.exe"),b"MZ").unwrap();
        assert!(find_python(tmp.path()).unwrap_err().contains("macOS"));
        std::fs::create_dir_all(env.join("bin")).unwrap();
        std::fs::write(env.join("bin/python3"),b"fixture").unwrap();
        assert_eq!(find_python(tmp.path()).unwrap(),env.join("bin/python3"));
        // A file with the expected name is insufficient: readiness must execute it.
        let sdk=Sdk{idf:tmp.path().join("idf"),tools:tmp.path().into()};
        assert!(validate(&sdk,&AtomicBool::new(false)).is_err());
    }
    #[test] fn parses_official_export_without_shell_execution() {
        let text="WARNING: fixture\nPATH=/sdk/cmake/CMake.app/Contents/bin:/sdk/ninja:$PATH\nESP_ROM_ELF_DIR=/sdk/rom\nAPI_KEY=must-not-be-imported\nIDF_PYTHON_ENV_PATH=/sdk/env";
        let env=parse_export(text,"/usr/bin:/bin").unwrap();
        assert_eq!(env["PATH"],"/sdk/cmake/CMake.app/Contents/bin:/sdk/ninja:/usr/bin:/bin");
        assert!(!env.contains_key("API_KEY"));
        assert!(parse_export("no environment","/usr/bin").is_err());
    }
    #[test] fn build_arguments_preserve_spaces_and_shell_metacharacters_as_arguments() {
        let sdk=Sdk{idf:"/sdk/idf".into(),tools:"/sdk/tools".into()};
        let ready=Ready{python:"/sdk/env/bin/python3".into(),env:BTreeMap::from([("PATH".into(),"/sdk/bin:/usr/bin".into())])};
        let project=if cfg!(windows){Path::new("C:/project")}else{Path::new("/project")};
        let args=vec!["build".into(),"-DNAME=value with spaces;$(touch bad)".into()];
        let cmd=build_command(&sdk,&ready,project,&args).unwrap();
        assert_eq!(cmd.get_program(),Path::new("/sdk/env/bin/python3"));
        assert_eq!(cmd.get_args().skip(1).collect::<Vec<_>>(),args.iter().map(std::ffi::OsStr::new).collect::<Vec<_>>());
    }
    #[cfg(unix)]
    #[test] fn stop_kills_the_installer_child_tree() {
        let dir=tempfile::tempdir().unwrap();let pid_file=dir.path().join("child.pid");
        let mut cmd=Command::new("/bin/sh");
        cmd.args(["-c","sleep 60 & echo $! > \"$1\"; wait","fixture"]).arg(&pid_file);
        let cancel=AtomicBool::new(false);
        std::thread::scope(|scope| {
            let result=scope.spawn(||run(cmd,"fixture",&cancel,Duration::from_secs(15),&|_|{}));
            let start=Instant::now();while !pid_file.exists() && start.elapsed()<Duration::from_secs(5) {std::thread::sleep(Duration::from_millis(20));}
            assert!(pid_file.exists());let pid: i32=std::fs::read_to_string(&pid_file).unwrap().trim().parse().unwrap();
            cancel.store(true,Ordering::SeqCst);assert_eq!(result.join().unwrap().unwrap_err(),"Cancelled");
            // A killed orphan may briefly be a zombie. It must never still be running.
            let start=Instant::now();loop {
                let status=Command::new("ps").args(["-o","stat=","-p",&pid.to_string()]).output().unwrap();
                let status=String::from_utf8_lossy(&status.stdout);
                if status.trim().is_empty() || status.trim().starts_with('Z') {break;}
                assert!(start.elapsed()<Duration::from_secs(5),"Installer grandchild remains running: {pid}");
                std::thread::sleep(Duration::from_millis(30));
            }
        });
    }
}
