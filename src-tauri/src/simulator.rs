//! Read-only loopback host and IDE-only native export dialog.
//! The simulator iframe never receives Tauri capabilities or filesystem access.
use serde::Serialize;
use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use std::sync::{
    atomic::{AtomicBool, Ordering},
    Arc, Mutex,
};
use std::thread::JoinHandle;

#[derive(Clone, Serialize)]
pub struct HostInfo {
    url: String,
    origin: String,
    session_id: String,
}

struct Host {
    info: HostInfo,
    stop: Arc<AtomicBool>,
    thread: Option<JoinHandle<()>>,
}
impl Drop for Host {
    fn drop(&mut self) {
        self.stop.store(true, Ordering::Release);
        if let Some(thread) = self.thread.take() {
            let _ = thread.join();
        }
    }
}
#[derive(Default)]
pub struct SimulatorState(Mutex<Option<Host>>);

fn validate_export_source(source: &str) -> Result<(), String> {
    if source.trim().is_empty() {
        return Err("ไม่มีโค้ดจำลองให้ส่งออก กรุณาเลือกตัวอย่างหรือใส่โค้ดก่อน".into());
    }
    Ok(())
}

async fn save_export(
    destination: Option<std::path::PathBuf>,
    source: &str,
) -> Result<Option<String>, String> {
    let Some(path) = destination else {
        return Ok(None);
    };
    validate_export_source(source)?;
    tokio::fs::write(&path, source.as_bytes())
        .await
        .map_err(|err| format!("ส่งออกไฟล์ไม่สำเร็จ ({}): {err}", path.display()))?;
    Ok(Some(path.to_string_lossy().into_owned()))
}

#[tauri::command]
pub async fn export_simulator_source(
    window: tauri::Window,
    source: String,
) -> Result<Option<String>, String> {
    if window.label() != "main" {
        return Err("Simulator export is restricted to the IDE window".into());
    }
    validate_export_source(&source)?;
    // The user chooses the destination in a native, parented dialog. No path
    // supplied by the iframe or HTTP client is accepted by this command.
    let file = rfd::AsyncFileDialog::new()
        .set_parent(&window)
        .set_title("ส่งออกโปรแกรมจำลอง .sim.js")
        .add_filter("KidBright Simulation (*.sim.js)", &["sim.js"])
        .set_file_name("kidbright.sim.js")
        .save_file()
        .await;
    save_export(file.map(|file| file.path().to_path_buf()), &source).await
}

fn allowed_parent(origin: &str) -> bool {
    if matches!(
        origin,
        "tauri://localhost" | "http://tauri.localhost" | "https://tauri.localhost"
    ) {
        return true;
    }
    let Ok(url) = tauri::Url::parse(origin) else {
        return false;
    };
    cfg!(debug_assertions)
        && url.scheme() == "http"
        && matches!(url.host_str(), Some("localhost" | "127.0.0.1"))
        && url.port().is_some()
        && url.origin().ascii_serialization() == origin
}
fn valid_session(session: &str) -> bool {
    session.len() == 36
        && session.bytes().enumerate().all(|(i, b)| {
            if [8, 13, 18, 23].contains(&i) {
                b == b'-'
            } else {
                b.is_ascii_hexdigit()
            }
        })
}

macro_rules! asset {
    ($name:literal) => {
        include_bytes!(concat!(
            env!("CARGO_MANIFEST_DIR"),
            "/../simulator-assets/",
            $name
        )) as &'static [u8]
    };
}
fn asset(path: &str) -> Option<(&'static str, &'static [u8])> {
    Some(match path {
        "UnityBuild/index.html" => ("text/html; charset=utf-8", asset!("UnityBuild/index.html")),
        "runtime.js" => ("text/javascript; charset=utf-8", asset!("runtime.js")),
        "runtime-core.js" => ("text/javascript; charset=utf-8", asset!("runtime-core.js")),
        "JS-Interpreter/acorn.js" => ("text/javascript", asset!("JS-Interpreter/acorn.js")),
        "JS-Interpreter/interpreter.js" => {
            ("text/javascript", asset!("JS-Interpreter/interpreter.js"))
        }
        "UnityBuild/Build/KV.json" => ("application/json", asset!("UnityBuild/Build/KV.json")),
        "UnityBuild/Build/UnityLoader.js" => {
            ("text/javascript", asset!("UnityBuild/Build/UnityLoader.js"))
        }
        "UnityBuild/Build/KV.data.unityweb" => (
            "application/octet-stream",
            asset!("UnityBuild/Build/KV.data.unityweb"),
        ),
        "UnityBuild/Build/KV.wasm.code.unityweb" => (
            "application/wasm",
            asset!("UnityBuild/Build/KV.wasm.code.unityweb"),
        ),
        "UnityBuild/Build/KV.wasm.framework.unityweb" => (
            "text/javascript",
            asset!("UnityBuild/Build/KV.wasm.framework.unityweb"),
        ),
        "UnityBuild/src/FileSaver.js" => ("text/javascript", asset!("UnityBuild/src/FileSaver.js")),
        "UnityBuild/TemplateData/UnityProgress.js" => (
            "text/javascript",
            asset!("UnityBuild/TemplateData/UnityProgress.js"),
        ),
        "UnityBuild/TemplateData/responsive.js" => (
            "text/javascript",
            asset!("UnityBuild/TemplateData/responsive.js"),
        ),
        "UnityBuild/TemplateData/style.css" => {
            ("text/css", asset!("UnityBuild/TemplateData/style.css"))
        }
        "UnityBuild/TemplateData/favicon.ico" => (
            "image/x-icon",
            asset!("UnityBuild/TemplateData/favicon.ico"),
        ),
        "UnityBuild/TemplateData/progressLogoKV.Dark.png" => (
            "image/png",
            asset!("UnityBuild/TemplateData/progressLogoKV.Dark.png"),
        ),
        "UnityBuild/TemplateData/progressEmpty.Dark.png" => (
            "image/png",
            asset!("UnityBuild/TemplateData/progressEmpty.Dark.png"),
        ),
        "UnityBuild/TemplateData/progressFull.Dark.png" => (
            "image/png",
            asset!("UnityBuild/TemplateData/progressFull.Dark.png"),
        ),
        _ => return None,
    })
}

fn respond(stream: &mut TcpStream, session: &str, parent: &str, host: &str) -> std::io::Result<()> {
    // Windows accepts sockets with the listener's nonblocking mode. Browsers
    // preconnect before sending headers, so WouldBlock must not close the socket.
    stream.set_nonblocking(false)?;
    stream.set_read_timeout(Some(std::time::Duration::from_secs(2)))?;
    stream.set_write_timeout(Some(std::time::Duration::from_secs(5)))?;
    let mut buffer = Vec::new();
    let mut chunk = [0; 2048];
    while !buffer.windows(4).any(|part| part == b"\r\n\r\n") && buffer.len() < 8192 {
        let n = stream.read(&mut chunk)?;
        if n == 0 {
            return Ok(());
        }
        buffer.extend_from_slice(&chunk[..n]);
    }
    let request = String::from_utf8_lossy(&buffer);
    let mut lines = request.lines();
    let fields: Vec<_> = lines.next().unwrap_or("").split_whitespace().collect();
    let correct_host = lines.any(|line| {
        line.split_once(':')
            .is_some_and(|(name, value)| name.eq_ignore_ascii_case("host") && value.trim() == host)
    });
    let mut status = "404 Not Found";
    let mut content_type = "text/plain";
    let mut body: &[u8] = b"Not found";
    if fields.len() == 3 && correct_host && fields[0] == "GET" {
        let prefix = format!("/{session}/");
        let uri = fields[1].split('?').next().unwrap_or("");
        if let Some(relative) = uri.strip_prefix(&prefix) {
            if let Some((kind, data)) = asset(relative) {
                status = "200 OK";
                content_type = kind;
                body = data;
            }
        }
    } else {
        status = "400 Bad Request";
        body = b"Bad request";
    }
    let policy = format!("default-src 'self'; script-src 'self' 'unsafe-inline' 'unsafe-eval' blob:; style-src 'self' 'unsafe-inline'; img-src 'self' data:; connect-src 'self'; worker-src 'self' blob:; object-src 'none'; frame-ancestors {parent}; base-uri 'none'; form-action 'none'");
    write!(stream, "HTTP/1.1 {status}\r\nContent-Type: {content_type}\r\nContent-Length: {}\r\nConnection: close\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nContent-Security-Policy: {policy}\r\nCross-Origin-Resource-Policy: same-origin\r\n\r\n", body.len())?;
    stream.write_all(body)
}

fn start_host(session: String, parent: String) -> Result<Host, String> {
    if !valid_session(&session) || !allowed_parent(&parent) {
        return Err("Invalid simulator session or parent origin".into());
    }
    let listener = TcpListener::bind((std::net::Ipv4Addr::LOCALHOST, 0))
        .map_err(|e| format!("Cannot start local simulator: {e}"))?;
    let address = listener.local_addr().map_err(|e| e.to_string())?;
    listener.set_nonblocking(true).map_err(|e| e.to_string())?;
    let origin = format!("http://{address}");
    let info = HostInfo {
        url: format!("{origin}/{session}/UnityBuild/index.html"),
        origin,
        session_id: session.clone(),
    };
    let stop = Arc::new(AtomicBool::new(false));
    let stopping = stop.clone();
    let thread = std::thread::spawn(move || {
        while !stopping.load(Ordering::Acquire) {
            match listener.accept() {
                Ok((mut stream, _)) => {
                    let _ = respond(&mut stream, &session, &parent, &address.to_string());
                }
                Err(e) if e.kind() == std::io::ErrorKind::WouldBlock => {
                    std::thread::sleep(std::time::Duration::from_millis(5))
                }
                Err(_) => break,
            }
        }
    });
    Ok(Host {
        info,
        stop,
        thread: Some(thread),
    })
}

#[tauri::command]
pub async fn start_simulator_host(
    window: tauri::WebviewWindow,
    state: tauri::State<'_, SimulatorState>,
    session_id: String,
    parent_origin: String,
) -> Result<HostInfo, String> {
    if window.label() != "main" {
        return Err("Simulator host is restricted to the IDE window".into());
    }
    let mut current = state.0.lock().map_err(|_| "Simulator state unavailable")?;
    let host = start_host(session_id, parent_origin)?;
    let info = host.info.clone();
    *current = Some(host);
    Ok(info)
}

#[tauri::command]
pub async fn stop_simulator_host(
    window: tauri::WebviewWindow,
    state: tauri::State<'_, SimulatorState>,
    session_id: String,
) -> Result<(), String> {
    if window.label() != "main" {
        return Err("Simulator host is restricted to the IDE window".into());
    }
    let mut current = state.0.lock().map_err(|_| "Simulator state unavailable")?;
    if current
        .as_ref()
        .is_some_and(|host| host.info.session_id == session_id)
    {
        *current = None;
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    #[tokio::test]
    async fn export_preserves_utf8_source_and_reports_write_failures_and_cancellation() {
        let directory = std::env::temp_dir().join(format!(
            "sim-export-{}-{}",
            std::process::id(),
            std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap()
                .as_nanos()
        ));
        std::fs::create_dir(&directory).unwrap();
        let path = directory.join("อุณหภูมิ.sim.js");
        let source = "// อุณหภูมิ 🌡️\r\nmatrix.show_number(25);\r\n";
        assert_eq!(
            save_export(Some(path.clone()), source).await.unwrap(),
            Some(path.to_string_lossy().into_owned())
        );
        assert_eq!(std::fs::read(&path).unwrap(), source.as_bytes());
        // Cancelling must not write anything, and a subsequent export must work.
        assert_eq!(save_export(None, "discarded").await.unwrap(), None);
        assert_eq!(std::fs::read(&path).unwrap(), source.as_bytes());
        save_export(Some(path.clone()), "matrix.show_number(42);\n")
            .await
            .unwrap();
        assert_eq!(
            std::fs::read_to_string(&path).unwrap(),
            "matrix.show_number(42);\n"
        );
        assert!(save_export(Some(path.clone()), "  \n").await.is_err());
        assert_eq!(
            std::fs::read_to_string(&path).unwrap(),
            "matrix.show_number(42);\n"
        );
        let error = save_export(Some(directory.clone()), source)
            .await
            .unwrap_err();
        assert!(error.contains("ส่งออกไฟล์ไม่สำเร็จ"));
        assert!(!error.contains(source));
        std::fs::remove_dir_all(directory).unwrap();
    }
    const SESSION: &str = "0b12a345-1234-1234-1234-123456789abc";
    fn get(host: &Host, path: &str) -> String {
        let address = host.info.origin.strip_prefix("http://").unwrap();
        let mut stream = TcpStream::connect(address).unwrap();
        write!(stream, "GET {path} HTTP/1.1\r\nHost: {address}\r\n\r\n").unwrap();
        let mut response = String::new();
        stream.read_to_string(&mut response).unwrap();
        response
    }
    #[test]
    fn serves_only_pinned_assets_and_never_workspace_or_traversal_paths() {
        let host = start_host(SESSION.into(), "http://localhost:1420".into()).unwrap();
        let ok = get(&host, &format!("/{SESSION}/runtime.js"));
        assert!(ok.starts_with("HTTP/1.1 200"));
        assert!(ok.contains("frame-ancestors http://localhost:1420"));
        for path in [
            "/runtime.js",
            "/wrong/runtime.js",
            "/../config.json",
            "/%2e%2e/config.json",
        ] {
            assert!(get(&host, path).starts_with("HTTP/1.1 404"));
        }
        assert!(get(&host, &format!("/{SESSION}/../../Cargo.toml")).starts_with("HTTP/1.1 404"));
        assert_eq!(
            asset("UnityBuild/Build/KV.wasm.code.unityweb").unwrap().0,
            "application/wasm"
        );
        let address = host.info.origin.trim_start_matches("http://").to_string();
        drop(host);
        assert!(TcpStream::connect(address).is_err());
    }
    #[test]
    fn accepts_a_browser_connection_before_its_request_headers_arrive() {
        let host = start_host(SESSION.into(), "http://localhost:1420".into()).unwrap();
        let address = host.info.origin.strip_prefix("http://").unwrap();
        let mut stream = TcpStream::connect(address).unwrap();
        stream
            .set_read_timeout(Some(std::time::Duration::from_secs(4)))
            .unwrap();
        // Browsers can preconnect before sending a request. An accepted Windows
        // socket must not be dropped merely because its first read would block.
        std::thread::sleep(std::time::Duration::from_millis(100));
        write!(stream, "GET /{SESSION}/UnityBuild/TemplateData/UnityProgress.js HTTP/1.1\r\nHost: {address}\r\n\r\n").unwrap();
        let mut response = String::new();
        stream.read_to_string(&mut response).unwrap();
        assert!(
            response.starts_with("HTTP/1.1 200"),
            "The preconnected browser must receive the script"
        );
        assert!(response.contains("function UnityProgress"));
    }
    #[test]
    fn rejects_remote_parent_origins_and_bad_sessions() {
        assert!(start_host(SESSION.into(), "https://example.invalid".into()).is_err());
        assert!(start_host("../secret".into(), "http://localhost:1420".into()).is_err());
        assert!(start_host("\r\nInjected: true".into(), "http://localhost:1420".into()).is_err());
    }
}
