//! Creation requests must select a fresh workspace before touching user files.
use crate::ai_chat::ChatMessage;
use std::path::{Path, PathBuf};

pub(crate) fn requests_new_project(messages: &[ChatMessage]) -> bool {
    messages.iter().rev().find(|m|m.role=="user")
        .and_then(|m|m.content.as_str()).is_some_and(requests_new_project_text)
}

pub(crate) fn requests_new_project_text(text:&str)->bool {
    let normalized=text.replace("\r\n","\n");
    let mut request=normalized.trim();
    // Compatibility with previous frontends that embedded app instructions.
    if request.starts_with("[CURRENT PROJECT STATE:") || request.starts_with("[CRITICAL SYSTEM ENFORCEMENT: NO WORKSPACE") {
        if let Some((_,body))=request.split_once("]\n\n") {
            request=body.split("\n\n[CRITICAL REMINDER:").next().unwrap_or(body);
        }
    }
    for fence in ["\u{0060}\u{0060}\u{0060}text\n","\u{0060}\u{0060}\u{0060}plaintext\n","~~~text\n","~~~plaintext\n"] {
        if let Some(body)=request.strip_prefix(fence) {
            if let Some(body)=body.trim_end().strip_suffix(&fence[..3]){request=body.trim();}
            break;
        }
    }
    let mut fenced=false;
    let mut prose=request.lines().filter_map(|line|{
        let line=line.trim();
        if line.starts_with("\u{0060}\u{0060}\u{0060}")||line.starts_with("~~~"){fenced=!fenced;return None;}
        (!fenced&&!line.starts_with('>')).then_some(line)
    }).collect::<Vec<_>>().join(" ").to_lowercase();
    let trimmed=prose.trim();
    if ["why ","explain ","how to ","show an example","ทำไม","อธิบายวิธี","อธิบายการ","ช่วยอธิบาย","ขอพรอม","ขอตัวอย่าง","ช่วยปรับพรอม"].iter().any(|p|trimmed.starts_with(p)){return false;}
    if ["ช่วยเช็ค","ช่วยตรวจสอบ","ตรวจสอบ"].iter().any(|p|trimmed.starts_with(p)) && (trimmed.contains("ทำไม")||trimmed.contains("why ")){return false;}
    for spelling in ["โปรเจ็คต์","โปรเจ็กต์","โปรเจคต์","โปรเจกต์","โปรเจ็ค","โปรเจก","โปรเจค"] {prose=prose.replace(spelling,"project");}
    if regex::Regex::new(r"(?:don't|do not|never)\s+(?:create|start|generate|make|build)(?:\s+[a-z-]+){0,4}\s+project\b|(?:ไม่ต้อง|ห้าม|อย่า(?:เพิ่ง|พึ่ง)?|ยังไม่(?:ต้อง)?|ไม่อยาก|ไม่ต้องการ)\s*(?:สร้าง|เริ่ม|ทำ)\s*project").unwrap().is_match(&prose){return false;}
    let thai=regex::Regex::new(r"(?:สร้าง|เริ่ม(?:ต้น)?|ทำ)\s*(?:(?:ตัว|อีกหนึ่ง|อีก|หนึ่ง)\s*)?project").unwrap();
    if thai.find_iter(&prose).any(|m|!prose[m.end()..].trim_start().starts_with("เดิม")){return true;}
    if regex::Regex::new(r"(?:ให้|ช่วย|ขอ|ต้องการ)\s*ร้าง\s*project\s*ใหม่").unwrap().is_match(&prose){return true;}
    let words:Vec<_>=prose.split(|c:char|!c.is_ascii_alphanumeric()&&c!='-').filter(|s|!s.is_empty()).collect();
    for (i,word) in words.iter().enumerate() {
        if !["create","start","generate","make","build"].contains(word){continue;}
        let mut new=false;
        for object in words.iter().skip(i+1).take(10) {
            if ["in","inside","within","for","to","of","this","current","existing"].contains(object){break;}
            if ["new","fresh","another"].contains(object){new=true;}
            if *object=="project"||*object=="projects" {if *word!="build"||new{return true;}break;}
        }
    }
    false
}

pub(crate) fn creation_completion_error(fresh:bool,no_workspace:bool)->Option<&'static str> {
    (fresh&&no_workspace).then_some("ยังไม่ได้สร้างโปรเจกต์ใหม่: ต้องเลือกโฟลเดอร์และสร้าง workspace ให้สำเร็จก่อน โปรเจกต์เดิมจะไม่ถูกใช้แทน กรุณาลองใหม่และยืนยันตำแหน่งโปรเจกต์")
}
pub(crate) fn project_file_path(project:&Path,relative:&str)->Result<PathBuf,String> {
    crate::simulation_paths::PathScope::new(project)?.resolve(relative,true)
        .map_err(|error|format!("Use a relative path inside the selected project workspace: {error}"))
}

pub(crate) fn workspace_tool_blocked(name: &str, no_workspace: bool) -> bool {
    no_workspace && matches!(name, "report_simulation_unsupported" | "write_file" | "generate_simulation" | "generate_fan_control" | "generate_device_program" | "run_command" | "read_file" | "read_device_program" | "list_files" | "get_file_tree" | "search_in_files" | "diff_file" | "install_idf_library")
}

pub(crate) fn initialize_project(base: &Path, name: &str) -> Result<PathBuf, String> {
    let reserved = ["CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"];
    if name.is_empty() || name.len() > 80 || !name.bytes().all(|c| c.is_ascii_alphanumeric() || c == b'_' || c == b'-') || reserved.contains(&name.to_ascii_uppercase().as_str()) {
        return Err("Choose a project name using ASCII letters, digits, underscores or hyphens.".into());
    }
    let path = base.join(name);
    std::fs::create_dir(&path).map_err(|error| {
        if error.kind()==std::io::ErrorKind::AlreadyExists {
            format!("มีไฟล์หรือโฟลเดอร์ชื่อ {name} อยู่แล้วที่ {} กรุณาเลือกชื่อโปรเจกต์ใหม่ หากต้องการทำต่อในโปรเจกต์เดิม ให้ยกเลิกคำสั่งสร้างใหม่แล้วสั่งให้ทำต่ออย่างชัดเจน ระบบจะไม่เขียนทับไฟล์เดิม",path.display())
        } else {
            format!("สร้างโฟลเดอร์โปรเจกต์ใหม่ไม่ได้ที่ {}: {error} กรุณาเลือกตำแหน่งที่เขียนไฟล์ได้",path.display())
        }
    })?;
    let result = (|| -> Result<(), std::io::Error> {
        std::fs::create_dir(path.join("main"))?;
        std::fs::write(path.join("CMakeLists.txt"), format!("cmake_minimum_required(VERSION 3.16)\ninclude($ENV{{IDF_PATH}}/tools/cmake/project.cmake)\nproject({name})\n"))?;
        std::fs::write(path.join("main/CMakeLists.txt"), "idf_component_register(SRCS \"main.c\" INCLUDE_DIRS \".\")\n")?;
        std::fs::write(path.join("main/main.c"), "#include <stdio.h>\nvoid app_main(void) { printf(\"Hello\\n\"); }\n")?;
        std::fs::write(path.join("sdkconfig.defaults"), "CONFIG_IDF_TARGET=\"esp32\"\nCONFIG_FREERTOS_HZ=1000\n")?;
        Ok(())
    })();
    result.map_err(|e| format!("Project initialization incomplete: {e}. The previous project was not changed."))?;
    Ok(path)
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::json;
    fn user(text: &str) -> ChatMessage { ChatMessage { role: "user".into(), content: json!(text) } }
    #[test]
    fn explicit_creation_is_fresh_even_with_an_open_workspace() {
        for text in ["Create a new project to blink the Wi-Fi LED", "ช่วยสร้างโปรเจคต์ใหม่ให้ไฟ Wi-Fi กับ IoT กระพริบ", "สร้างโปรเจกต์อ่านอุณหภูมิ"] {
            assert!(requests_new_project(&[user(text)]), "{text}");
            for tool in ["report_simulation_unsupported", "write_file", "generate_simulation", "generate_fan_control", "generate_device_program", "read_file", "run_command", "install_idf_library"] { assert!(workspace_tool_blocked(tool, true)); }
            assert!(!workspace_tool_blocked("create_project_workspace", true));
            assert!(!workspace_tool_blocked("knowledge_search", true));
        }
    }
    #[test]
    fn edits_negations_explanations_and_reference_text_do_not_create_projects() {
        for text in ["Change the blink delay in the existing project", "Build this project", "ไม่ต้องสร้างโปรเจคต์ใหม่ แก้ main.c", "Don't create a new project; edit main.c", "Explain how to create a new project", "> Create a new project\nแก้ main.c", "```c\n// create new project\n```\nFix this code"] {
            assert!(!requests_new_project(&[user(text)]), "{text}");
        }
        assert!(!requests_new_project(&[user("Create a new project"), user("Change GPIO2 to GPIO12")]));
    }
    #[test]
    fn duplicate_and_invalid_names_never_overwrite_a_project() {
        let temp = tempfile::tempdir().unwrap();
        let old = initialize_project(temp.path(), "old_project").unwrap();
        let original = std::fs::read(old.join("main/main.c")).unwrap();
        assert!(initialize_project(temp.path(), "old_project").is_err());
        for name in ["..", "../escape", "a/b", "C:\\escape", "NUL", "a.c", ""] { assert!(initialize_project(temp.path(), name).is_err()); }
        let new = initialize_project(temp.path(), "new_project").unwrap();
        assert!(new.join("sdkconfig.defaults").is_file());
        assert_eq!(std::fs::read(old.join("main/main.c")).unwrap(), original);
    }
}

#[cfg(test)]
mod creation_intent_regression_tests {
    use super::*;
    use serde_json::json;
    fn user(text:&str)->ChatMessage{ChatMessage{role:"user".into(),content:json!(text)}}
    #[test]
    fn app_context_does_not_override_the_actual_creation_request() {
        let wrapped="[CURRENT PROJECT STATE: workspace is ALREADY OPEN. Do NOT call create_project_workspace and do NOT create a new project UNLESS explicitly asked.]\n\nสร้างโปรเจกต์ใหม่ให้เลื่อน A-Z\n\n[CRITICAL REMINDER: use read_file or write_file immediately.]";
        assert!(requests_new_project(&[user(wrapped)]));
        assert!(requests_new_project_text("สร้างโปรเจกต์ใหม่ให้เลื่อน A-Z"));
        let greeting="[CRITICAL SYSTEM ENFORCEMENT: NO WORKSPACE IS CURRENTLY OPEN! create a project first.]\n\nสวัสดีครับ";
        assert!(!requests_new_project(&[user(greeting)]));
    }
    #[test]
    fn recognizes_creation_with_preservation_explanations_and_common_thai_spellings() {
        for text in ["สร้างโปรเจกต์ใหม่สำหรับ KidBright32 โดยไม่แก้โปรเจกต์เดิม",
            "ช่วยสร้างโปรเจ็คใหม่ แสดง A-Z และอธิบายโค้ด","ให้สร้างโปรเจคต์ใหม่แล้วทดสอบด้วย Simulator ก่อน",
            "ช่วยร้างโปรเจคใหม่ให้ไฟกระพริบ","~~~text\nสร้างโปรเจกต์ใหม่สำหรับ KidBright32\n~~~",
            "Create a new temperature display project without modifying the existing project","Build a new ESP-IDF project"]{
            assert!(requests_new_project_text(text),"{text}");
        }
        let tick="\u{0060}";
        assert!(requests_new_project_text(&format!("{tick}{tick}{tick}text\nสร้างโปรเจกต์ใหม่\n{tick}{tick}{tick}")));
    }
    #[test]
    fn edits_negations_references_and_latest_message_remain_distinct() {
        for text in ["อย่าเพิ่งสร้างโปรเจกต์ใหม่ แก้ main.c ก่อน","อย่าพึ่งสร้างโปรเจคใหม่","ไม่ต้องสร้างโปรเจคใหม่",
            "Create a file in this project","Build this project","ขอพรอมสำหรับสร้างโปรเจกต์ใหม่",
            "ช่วยเช็คว่าทำไมเมื่อสั่งให้สร้างโปรเจคใหม่ถึงไปแก้ไฟล์เดิม",
            "> สร้างโปรเจกต์ใหม่\nช่วยแก้โค้ดเดิม","~~~c\n// create a new project\n~~~\nFix the code"] {
            assert!(!requests_new_project_text(text),"{text}");
        }
        assert!(!requests_new_project(&[user("Create a new project"),user("Change the delay to 100 ms")]));
    }
    #[test]
    fn fresh_selection_is_required_and_writes_cannot_target_the_old_project() {
        let base=tempfile::tempdir().unwrap();
        let old=initialize_project(base.path(),"old_project").unwrap();
        let before=std::fs::read(old.join("main/main.c")).unwrap();
        let fresh=requests_new_project_text("สร้างโปรเจกต์ใหม่โดยไม่แก้โปรเจกต์เดิม");
        assert!(workspace_tool_blocked("write_file",fresh));
        assert!(initialize_project(base.path(),"old_project").is_err());
        assert!(workspace_tool_blocked("generate_simulation",fresh));
        let new=initialize_project(base.path(),"fresh_project").unwrap();
        assert!(project_file_path(&new,&old.join("main/main.c").to_string_lossy()).is_err());
        assert!(project_file_path(&new,"../old_project/main/main.c").is_err());
        std::fs::write(project_file_path(&new,"main/main.c").unwrap(),"new project code").unwrap();
        assert_eq!(std::fs::read(old.join("main/main.c")).unwrap(),before);
    }
    #[test]
    fn an_uncreated_workspace_is_not_reported_as_success() {
        assert!(creation_completion_error(true,true).is_some());
        assert!(creation_completion_error(true,false).is_none());
        assert!(creation_completion_error(false,false).is_none());
    }
}

// Workspace scaffolding is separate from real firmware/simulation generation.
// A failed creation must never trigger checks against the previous workspace.
#[derive(Default)]
pub(crate) struct GenerationProgress {
    workspace_created: bool,
    creation_error: Option<String>,
    attempted: bool,
    generation_error: Option<String>,
}
#[derive(Debug, PartialEq, Eq)]
pub(crate) enum GenerationCheck { None, ScaffoldOnly, Validate }
impl GenerationProgress {
    pub(crate) fn record(&mut self, name: &str, input: &serde_json::Value, result: &serde_json::Value) {
        let error=result.get("error").map(|value|value.as_str().map(str::to_owned).unwrap_or_else(||value.to_string()));
        if name=="create_project_workspace" {
            if let Some(error)=error {self.creation_error=Some(error);}
            else if result["workspace_path"].as_str().is_some_and(|path|!path.is_empty()) {
                self.workspace_created=true;self.creation_error=None;
                self.attempted=false;self.generation_error=None;
            } else {
                self.creation_error=Some("ยังยืนยันการสร้าง workspace ใหม่ไม่สำเร็จ กรุณาลองเลือกโฟลเดอร์และชื่อโปรเจกต์อีกครั้ง".into());
            }
            return;
        }
        if crate::hardware_contract::generation_tool(name,input)||name=="report_simulation_unsupported" {
            self.attempted=true;self.generation_error=error;
        }
    }
    pub(crate) fn before_completion(&self,no_workspace:bool)->Result<GenerationCheck,String> {
        if no_workspace {
            if let Some(error)=&self.creation_error {return Err(error.clone());}
            if let Some(error)=&self.generation_error {return Err(error.clone());}
            if self.attempted||self.workspace_created {
                return Err("ยังไม่มี workspace ที่พร้อมใช้งาน ต้องสร้างโปรเจกต์ใหม่ให้สำเร็จก่อนสร้างโค้ด โปรเจกต์เดิมจะไม่ถูกใช้แทน".into());
            }
            return Ok(GenerationCheck::None);
        }
        if let Some(error)=&self.generation_error {return Err(error.clone());}
        Ok(if self.attempted {GenerationCheck::Validate} else if self.workspace_created {GenerationCheck::ScaffoldOnly} else {GenerationCheck::None})
    }
}


const LOCAL_COMPLETION_ERROR: &str = "[PROJECT_WORKFLOW] ";
pub(crate) fn local_completion_error(error:String)->String {format!("{LOCAL_COMPLETION_ERROR}{error}")}
pub(crate) fn completion_error_message(error:&str)->Option<&str> {error.strip_prefix(LOCAL_COMPLETION_ERROR)}
