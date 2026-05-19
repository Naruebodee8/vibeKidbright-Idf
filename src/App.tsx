import React, { useState, useEffect, useRef, useCallback } from "react";
import { invoke } from "@tauri-apps/api/core";
import { listen } from "@tauri-apps/api/event";
import AiChat from "./AiChat";
import CodeEditor from "./CodeEditor";

interface FileEntry {
  name: string;
  path: string;
  is_dir: boolean;
  children?: FileEntry[];
}

// ── File Icon SVGs by type ──────────────────────────────────────────────────
function FileIcon({ name }: { name: string }) {
  const lower = name.toLowerCase();

  if (lower.endsWith(".c")) {
    return (
      <svg className="w-4 h-4 shrink-0" viewBox="0 0 24 24" fill="none">
        <rect x="2" y="2" width="20" height="20" rx="3" fill="#3b82f6" fillOpacity="0.15" />
        <text x="12" y="16" textAnchor="middle" fill="#60a5fa" fontSize="12" fontWeight="700" fontFamily="monospace">C</text>
      </svg>
    );
  }
  if (lower.endsWith(".h")) {
    return (
      <svg className="w-4 h-4 shrink-0" viewBox="0 0 24 24" fill="none">
        <rect x="2" y="2" width="20" height="20" rx="3" fill="#a78bfa" fillOpacity="0.15" />
        <text x="12" y="16" textAnchor="middle" fill="#a78bfa" fontSize="12" fontWeight="700" fontFamily="monospace">H</text>
      </svg>
    );
  }
  if (lower.endsWith(".py")) {
    return (
      <svg className="w-4 h-4 shrink-0" viewBox="0 0 24 24" fill="none">
        <rect x="2" y="2" width="20" height="20" rx="3" fill="#fbbf24" fillOpacity="0.15" />
        <text x="12" y="16" textAnchor="middle" fill="#fbbf24" fontSize="11" fontWeight="700" fontFamily="monospace">Py</text>
      </svg>
    );
  }
  if (lower.endsWith(".json")) {
    return (
      <svg className="w-4 h-4 shrink-0" viewBox="0 0 24 24" fill="none">
        <rect x="2" y="2" width="20" height="20" rx="3" fill="#fbbf24" fillOpacity="0.12" />
        <text x="12" y="16" textAnchor="middle" fill="#f59e0b" fontSize="8" fontWeight="700" fontFamily="monospace">{'{}'}</text>
      </svg>
    );
  }
  if (lower.endsWith(".md") || lower.endsWith(".markdown")) {
    return (
      <svg className="w-4 h-4 shrink-0" viewBox="0 0 24 24" fill="none">
        <rect x="2" y="2" width="20" height="20" rx="3" fill="#38bdf8" fillOpacity="0.12" />
        <text x="12" y="16" textAnchor="middle" fill="#38bdf8" fontSize="10" fontWeight="700" fontFamily="monospace">M</text>
      </svg>
    );
  }
  if (lower.includes("cmakelists") || lower.endsWith(".cmake")) {
    return (
      <svg className="w-4 h-4 shrink-0" viewBox="0 0 24 24" fill="none">
        <rect x="2" y="2" width="20" height="20" rx="3" fill="#f43f5e" fillOpacity="0.12" />
        <path d="M8 8l4 4-4 4M13 16h4" stroke="#f43f5e" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" />
      </svg>
    );
  }
  if (lower.startsWith("sdkconfig") || lower.endsWith(".cfg") || lower.endsWith(".ini") || lower.endsWith(".conf")) {
    return (
      <svg className="w-4 h-4 shrink-0" viewBox="0 0 24 24" fill="none">
        <rect x="2" y="2" width="20" height="20" rx="3" fill="#a3e635" fillOpacity="0.1" />
        <path d="M12 15a3 3 0 100-6 3 3 0 000 6z" stroke="#a3e635" strokeWidth="1.5" />
        <path d="M12 4v2m0 12v2m-8-8h2m12 0h2m-3.5-5.5l-1.4 1.4m-5.2 5.2l-1.4 1.4m0-8l1.4 1.4m5.2 5.2l1.4 1.4" stroke="#a3e635" strokeWidth="1.5" strokeLinecap="round" />
      </svg>
    );
  }
  if (lower.endsWith(".rs")) {
    return (
      <svg className="w-4 h-4 shrink-0" viewBox="0 0 24 24" fill="none">
        <rect x="2" y="2" width="20" height="20" rx="3" fill="#fb923c" fillOpacity="0.12" />
        <text x="12" y="16" textAnchor="middle" fill="#fb923c" fontSize="11" fontWeight="700" fontFamily="monospace">Rs</text>
      </svg>
    );
  }
  if (lower.endsWith(".toml") || lower.endsWith(".yml") || lower.endsWith(".yaml")) {
    return (
      <svg className="w-4 h-4 shrink-0" viewBox="0 0 24 24" fill="none">
        <rect x="2" y="2" width="20" height="20" rx="3" fill="#94a3b8" fillOpacity="0.1" />
        <path d="M7 8h10M7 12h7M7 16h10" stroke="#64748b" strokeWidth="1.5" strokeLinecap="round" />
      </svg>
    );
  }
  // Default generic file
  return (
    <svg className="w-4 h-4 shrink-0" viewBox="0 0 24 24" fill="none">
      <path d="M14 2H6a2 2 0 00-2 2v16a2 2 0 002 2h12a2 2 0 002-2V8l-6-6z" stroke="#475569" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" />
      <path d="M14 2v6h6" stroke="#475569" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" />
    </svg>
  );
}

function FolderIcon({ isOpen }: { isOpen: boolean }) {
  if (isOpen) {
    return (
      <svg className="w-4 h-4 shrink-0" viewBox="0 0 24 24" fill="none">
        <path d="M5 19a2 2 0 01-2-2V7a2 2 0 012-2h4l2 2h6a2 2 0 012 2v1" stroke="#38bdf8" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" />
        <path d="M5 19h14a2 2 0 002-2l-2-7H5l-2 7a2 2 0 002 2z" fill="#38bdf8" fillOpacity="0.12" stroke="#38bdf8" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" />
      </svg>
    );
  }
  return (
    <svg className="w-4 h-4 shrink-0" viewBox="0 0 24 24" fill="none">
      <path d="M3 7v10a2 2 0 002 2h14a2 2 0 002-2V9a2 2 0 00-2-2h-6l-2-2H5a2 2 0 00-2 2z" fill="#38bdf8" fillOpacity="0.08" stroke="#475569" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" />
    </svg>
  );
}

function ChevronIcon({ isOpen }: { isOpen: boolean }) {
  return (
    <svg
      className={`w-3 h-3 shrink-0 text-[var(--text-muted)] transition-transform duration-200 ${isOpen ? "rotate-90" : ""}`}
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="2.5"
      strokeLinecap="round"
      strokeLinejoin="round"
    >
      <path d="M9 6l6 6-6 6" />
    </svg>
  );
}

function FileTreeItem({
  item,
  activeFile,
  openFolders,
  onFileClick,
  onFolderToggle,
  onContextMenu,
  inlineAction,
  inlineInputValue,
  setInlineInputValue,
  onInlineInputSubmit,
  onInlineInputCancel,
  depth = 0
}: {
  item: FileEntry;
  activeFile: string;
  openFolders: Set<string>;
  onFileClick: (path: string) => void;
  onFolderToggle: (path: string) => void;
  onContextMenu: (e: React.MouseEvent, path: string, isDir: boolean) => void;
  inlineAction: { mode: "createFile" | "createDir" | "rename"; path: string } | null;
  inlineInputValue: string;
  setInlineInputValue: (val: string) => void;
  onInlineInputSubmit: () => void;
  onInlineInputCancel: () => void;
  depth?: number;
}) {
  const isOpen = openFolders.has(item.path);
  const isActive = activeFile === item.path;
  const indent = depth * 12;

  const isRenaming = inlineAction?.mode === "rename" && inlineAction.path === item.path;
  const isCreatingInside = (inlineAction?.mode === "createFile" || inlineAction?.mode === "createDir") && inlineAction.path === item.path;

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === "Enter") onInlineInputSubmit();
    if (e.key === "Escape") onInlineInputCancel();
  };

  const renderInput = (iconName: string, isDir: boolean) => (
    <div className="w-full text-left py-[5px] px-2 flex items-center gap-1.5 rounded-md relative text-[var(--text-primary)]" style={{ paddingLeft: `${(isDir ? 8 : 20) + indent}px` }}>
      {isDir ? <FolderIcon isOpen={false} /> : <FileIcon name={iconName} />}
      <input
        autoFocus
        value={inlineInputValue}
        onChange={(e) => setInlineInputValue(e.target.value)}
        onKeyDown={handleKeyDown}
        onBlur={onInlineInputCancel}
        className="flex-1 bg-[var(--bg-elevated)] text-[12px] border border-[#1a56c4] rounded px-1 outline-none relative z-10 w-full"
      />
    </div>
  );

  if (item.is_dir) {
    return (
      <div>
        {isRenaming ? renderInput(item.name, true) : (
          <button
            onClick={() => onFolderToggle(item.path)}
            onContextMenu={(e) => onContextMenu(e, item.path, true)}
            className="w-full text-left py-[5px] px-2 flex items-center gap-1.5 text-[12px] transition-all duration-150 rounded-md hover:bg-[var(--bg-overlay)]/40 text-[var(--text-secondary)] hover:text-[var(--text-primary)] group relative"
            style={{ paddingLeft: `${8 + indent}px` }}
          >
            <ChevronIcon isOpen={isOpen} />
            <FolderIcon isOpen={isOpen} />
            <span className="truncate font-medium">{item.name}</span>
          </button>
        )}
        {(isOpen || isCreatingInside) && (
          <div className="relative">
            <div className="absolute top-0 bottom-0 border-l border-[var(--border-subtle)]" style={{ left: `${16 + indent}px` }} />
            {isCreatingInside && renderInput(inlineAction?.mode === "createDir" ? "folder" : "new.txt", inlineAction?.mode === "createDir")}
            {item.children?.map((child) => (
              <FileTreeItem
                key={child.path}
                item={child}
                activeFile={activeFile}
                openFolders={openFolders}
                onFileClick={onFileClick}
                onFolderToggle={onFolderToggle}
                onContextMenu={onContextMenu}
                inlineAction={inlineAction}
                inlineInputValue={inlineInputValue}
                setInlineInputValue={setInlineInputValue}
                onInlineInputSubmit={onInlineInputSubmit}
                onInlineInputCancel={onInlineInputCancel}
                depth={depth + 1}
              />
            ))}
          </div>
        )}
      </div>
    );
  }

  return isRenaming ? renderInput(item.name, false) : (
    <button
      onClick={() => onFileClick(item.path)}
      onDoubleClick={() => onFileClick(item.path)}
      onContextMenu={(e) => onContextMenu(e, item.path, false)}
      className={`w-full text-left py-[5px] px-2 flex items-center gap-1.5 text-[12px] transition-all duration-150 rounded-md group relative ${
        isActive ? "bg-blue-500/10 text-[var(--accent-300)]" : "text-[var(--text-secondary)] hover:bg-[#162033]/60 hover:text-[var(--text-primary)]"
      }`}
      style={{ paddingLeft: `${20 + indent}px` }}
    >
      {isActive && <div className="absolute left-0 top-1 bottom-1 w-[3px] rounded-full bg-blue-400 shadow-[0_0_6px_rgba(59,130,246,0.6)]" />}
      <FileIcon name={item.name} />
      <span className="truncate">{item.name}</span>
    </button>
  );
}

interface FileTab {
  name: string;
  path: string;
  content: string;
  savedContent: string;
}

// ── Theme presets ──────────────────────────────────────────────────────────
const THEMES = [
  { id: "navy",    label: "Navy Dark",      desc: "สีดำ-กรมท่า (ค่าเริ่มต้น)", color: "#1a56c4", gradient: "from-blue-400 to-cyan-400" },
  { id: "ocean",   label: "Ocean Deep",     desc: "สีน้ำ-ฟ้าเข้ม",             color: "#0e7490", gradient: "from-cyan-400 to-teal-400" },
  { id: "purple",  label: "Midnight Purple",desc: "สีม่วงมืด",                 color: "#6d28d9", gradient: "from-violet-400 to-purple-400" },
  { id: "emerald", label: "Emerald Night",  desc: "สีเขียวมรกต",               color: "#065f46", gradient: "from-emerald-400 to-green-400" },
  { id: "crimson", label: "Crimson",        desc: "สีแดง (ธีมดั้งเดิม)",       color: "#be123c", gradient: "from-rose-400 to-red-400" },
  { id: "light",   label: "Light (Clean)",  desc: "สว่าง (สบายตา)",            color: "#2563eb", gradient: "from-blue-500 to-sky-400" },
];

function App() {
  const [status, setStatus] = useState("Checking ESP-IDF...");
  const [isSettingUpEspIdf, setIsSettingUpEspIdf] = useState(false);
  const [espIdfSetupNote, setEspIdfSetupNote] = useState("");
  const [logs, setLogs] = useState<string[]>([]);
  const [terminalInput, setTerminalInput] = useState("");
  const [openFiles, setOpenFiles] = useState<FileTab[]>([]);
  const [activeFilePath, setActiveFilePath] = useState<string>("");
  const [showAiPanel, setShowAiPanel] = useState(true);
  const [projectDir, setProjectDir] = useState(".");
  const [isBuilding, setIsBuilding] = useState(false);
  const [serialPorts, setSerialPorts] = useState<string[]>([]);
  const [selectedSerialPort, setSelectedSerialPort] = useState("");
  const [serialBaud, setSerialBaud] = useState("115200");
  const [isSerialConnected, setIsSerialConnected] = useState(false);
  const [showNewProjectModal, setShowNewProjectModal] = useState(false);
  const [newProjectName, setNewProjectName] = useState("my_esp_project");
  const [newProjectPath, setNewProjectPath] = useState("");
  const [projectFiles, setProjectFiles] = useState<FileEntry[]>([]);
  const [openFolders, setOpenFolders] = useState<Set<string>>(new Set());
  const [showSetupModal, setShowSetupModal] = useState(false);
  const [customIdfPath, setCustomIdfPath] = useState("");
  const [customToolsPath, setCustomToolsPath] = useState("");
  const [isSavingPaths, setIsSavingPaths] = useState(false);
  const [setupModalError, setSetupModalError] = useState("");
  const [showThemeModal, setShowThemeModal] = useState(false);
  const [activeTheme, setActiveTheme] = useState<string>(() => localStorage.getItem("vibe_theme") || "navy");
  const [setupProgress, setSetupProgress] = useState<{stage: string, percent: number, message: string} | null>(null);

  // Apply theme on mount and change
  useEffect(() => {
    document.documentElement.setAttribute("data-theme", activeTheme === "navy" ? "" : activeTheme);
    localStorage.setItem("vibe_theme", activeTheme);
  }, [activeTheme]);

  const currentTheme = THEMES.find(t => t.id === activeTheme) || THEMES[0];

  const [contextMenu, setContextMenu] = useState<{ x: number; y: number; path: string; isDir: boolean } | null>(null);
  const [inlineAction, setInlineAction] = useState<{ mode: "createFile" | "createDir" | "rename"; path: string } | null>(null);
  const [inlineInputValue, setInlineInputValue] = useState("");

  useEffect(() => {
    const handleClickOutside = () => setContextMenu(null);
    window.addEventListener("click", handleClickOutside);
    return () => window.removeEventListener("click", handleClickOutside);
  }, []);
  const scrollRef = useRef<HTMLDivElement>(null);
  const openFilesRef = useRef<FileTab[]>(openFiles);
  useEffect(() => { openFilesRef.current = openFiles; }, [openFiles]);

  const normPath = (p: string) => p.replace(/\\/g, '/').toLowerCase();
  const activeFile = openFiles.find(f => normPath(f.path) === normPath(activeFilePath));

  // Load saved custom paths on mount
  useEffect(() => {
    invoke("get_idf_custom_paths").then((paths: unknown) => {
      const p = paths as { idf_path: string; tools_path: string };
      if (p.idf_path) setCustomIdfPath(p.idf_path);
      if (p.tools_path) setCustomToolsPath(p.tools_path);
    }).catch(() => {});
  }, []);

  const handleSaveCustomPaths = async () => {
    setIsSavingPaths(true);
    setSetupModalError("");
    try {
      const result = await invoke("set_idf_custom_paths", {
        idfPath: customIdfPath,
        toolsPath: customToolsPath,
      });
      addLog(`✅ ${result}`);
      setShowSetupModal(false);
      await checkEnvironment();
    } catch (err) {
      setSetupModalError(String(err));
      addLog(`❌ Path validation failed: ${err}`);
    } finally {
      setIsSavingPaths(false);
    }
  };

  const handleClearCustomPaths = async () => {
    await invoke("clear_idf_custom_paths").catch(() => {});
    setCustomIdfPath("");
    setCustomToolsPath("");
    addLog("Custom paths cleared. Using auto-detection.");
  };

  const handlePickIdfPath = async () => {
    const path = await invoke("pick_directory").catch(() => null);
    if (path) setCustomIdfPath(path as string);
  };

  const handlePickToolsPath = async () => {
    const path = await invoke("pick_directory").catch(() => null);
    if (path) setCustomToolsPath(path as string);
  };

  // --- 🛠 COMMAND WRAPPER HACK FOR WINDOWS 🛠 ---
  const runIdfWrappedCommand = async (baseCmd: string, args: string[], cwd: string | null) => {
    let idfPathToUse = customIdfPath;
    
    if (!idfPathToUse) {
      try {
        const p: any = await invoke("get_idf_custom_paths");
        if (p && p.idf_path) idfPathToUse = p.idf_path;
      } catch (e) {}
    }

    if (idfPathToUse) {
      const cleanPath = idfPathToUse.replace(/\//g, '\\');
      const exportScript = `${cleanPath}\\export.bat`;
      
      // 🛠 แก้ไข: เอาเครื่องหมาย "" รอบๆ ${exportScript} ออก เพื่อไม่ให้ CMD งงกับตัวอักษร \"
      const fullCmd = `call ${exportScript} && ${baseCmd} ${args.join(" ")}`;
      
      addLog(`[IDF Wrapper] Injecting environment from: ${exportScript}`);
      
      return await invoke("run_shell_command", {
        cmd: "cmd.exe",
        args: ["/c", fullCmd],
        cwd
      });
    } else {
      return await invoke("run_shell_command", { cmd: baseCmd, args, cwd });
    }
  };

  useEffect(() => {
    checkEnvironment();
    loadSerialPorts();

    const unlistenTerminal = listen("terminal-output", (event) => {
      setLogs((prev) => [...prev, event.payload as string]);
    });

    const unlistenProgress = listen("toolchain-progress", (event) => {
      setSetupProgress(event.payload as {stage: string, percent: number, message: string});
    });

    const unlistenFile = listen("file-modified", async (event) => {
      const { path } = JSON.parse(event.payload as string);
      const normPath = path.replace(/\\/g, '/');
      const isOpen = openFilesRef.current.find(
        f => f.path.replace(/\\/g, '/') === normPath
      );
      if (isOpen) {
        reloadFile(isOpen.path);
      }
      loadProjectFiles();
    });

    const unlistenForceDir = listen("force-project-dir", async (event) => {
      const newPath = event.payload as string;
      setProjectDir(newPath);
      setOpenFiles([]);
      setActiveFilePath("");
      addLog(`Switched project to: ${newPath}`);
    });

    const unlistenDiffPending = listen("ai-diff-pending", async (event) => {
      try {
        const data = typeof event.payload === 'string' ? JSON.parse(event.payload) : event.payload as any;
        const fullPath = String(data.fullPath || "");
        if (!fullPath) return;

        const norm = (p: string) => p.replace(/\\/g, '/').toLowerCase();
        const existing = openFilesRef.current.find(f => norm(f.path) === norm(fullPath));

        if (!existing) {
          try {
            const content = await invoke("read_project_file", { path: fullPath });
            const fileName = fullPath.split(/[\/\\]/).pop() || "file";
            const newTab: FileTab = { name: fileName, path: fullPath, content: content as string, savedContent: content as string };
            setOpenFiles(prev => [...prev, newTab]);
          } catch (e) {
            console.error("Failed to auto-open file for diff review:", e);
            return;
          }
        }
        setActiveFilePath(fullPath);
      } catch (e) {
        console.error("Error handling ai-diff-pending in App:", e);
      }
    });

    const handleKeyDown = (e: KeyboardEvent) => {
      if ((e.metaKey || e.ctrlKey) && e.key === "s") {
        e.preventDefault();
        saveAllFiles();
      }
    };
    window.addEventListener("keydown", handleKeyDown);

    return () => {
      unlistenTerminal.then((f) => f());
      unlistenProgress.then((f) => f());
      unlistenFile.then((f) => f());
      unlistenForceDir.then((f) => f());
      unlistenDiffPending.then((f) => f());
      window.removeEventListener("keydown", handleKeyDown);
      invoke("stop_serial_monitor").catch(() => null);
    };
  }, [activeFilePath, openFiles]);

  const reloadFile = async (path: string) => {
    try {
      const content = await invoke("read_project_file", { path });
      setOpenFiles(prev => prev.map(f =>
        f.path === path ? { ...f, content: content as string, savedContent: content as string } : f
      ));
      addLog(`Auto-reloaded: ${path.split("/").pop()}`);
    } catch (err) {
      addLog(`❌ Failed to reload file: ${err}`);
    }
  };

  useEffect(() => {
    if (projectDir !== ".") {
      loadProjectFiles();
    }
  }, [projectDir]);

  useEffect(() => {
    if (scrollRef.current) {
      scrollRef.current.scrollTop = scrollRef.current.scrollHeight;
    }
  }, [logs]);

  const saveAllFiles = useCallback(async () => {
    if (openFiles.length === 0) return;
    try {
      let savedCount = 0;
      for (const file of openFiles) {
        await invoke("write_project_file", {
          path: file.path,
          content: file.content
        });
        savedCount++;
      }
      setOpenFiles(prev => prev.map(f => ({ ...f, savedContent: f.content })));
      addLog(`✨ Saved all ${savedCount} open files`);
    } catch (err) {
      addLog(`❌ Failed to save files: ${err}`);
    }
  }, [openFiles]);

  const handleSaveProjectAs = async () => {
    if (projectDir === ".") {
      addLog("❌ No project open to save.");
      return;
    }

    await saveAllFiles();

    try {
      addLog("Opening folder picker to save project as...");
      const result = await invoke("save_project_as", { sourceDir: projectDir });
      const [newPath, fileCount] = (result as string).split("|");

      addLog(`✨ Project successfully saved to ${newPath} (${fileCount} files copied)`);

      const switchProject = window.confirm(`Project saved successfully!\n\nDo you want to switch to the new project location?\n\n${newPath}`);

      if (switchProject) {
        setProjectDir(newPath);
        setOpenFiles([]);
        setActiveFilePath("");
        addLog(`Switched project to: ${newPath}`);
      }
    } catch (err) {
      addLog(`❌ Failed to save project: ${err}`);
      alert(`Save Project As failed:\n${err}`);
    }
  };

  const handleContextMenu = (e: React.MouseEvent, path: string, isDir: boolean) => {
    e.preventDefault();
    setContextMenu({ x: e.clientX, y: e.clientY, path, isDir });
  };

  const handleDeleteItem = async (path: string, isDir: boolean) => {
    const name = path.split(/[\/\\]/).pop() || "";
    if (name === "CMakeLists.txt" || name === "sdkconfig") {
      const confirmCritical = window.confirm(`⚠️ WARNING: '${name}' is a critical ESP-IDF file.\n\nDeleting it may break your project.\n\nAre you absolutely sure you want to delete '${name}'?`);
      if (!confirmCritical) return;
    } else {
      const confirmDelete = window.confirm(`Are you sure you want to delete '${name}'?`);
      if (!confirmDelete) return;
    }

    try {
      if (isDir) {
        await invoke("delete_directory", { path });
      } else {
        await invoke("delete_file", { path });
      }
      addLog(`✨ Deleted: ${path}`);
      loadProjectFiles();
      if (openFiles.some(f => normPath(f.path) === normPath(path))) {
        setOpenFiles(prev => prev.filter(f => normPath(f.path) !== normPath(path)));
      }
      if (normPath(activeFilePath) === normPath(path)) setActiveFilePath("");
    } catch (err) {
      alert(`Failed to delete:\n${err}`);
    }
  };

  const handleInlineSubmit = async () => {
    if (!inlineAction || !inlineInputValue.trim()) {
      setInlineAction(null);
      return;
    }

    const value = inlineInputValue.trim();
    try {
      if (inlineAction.mode === "rename") {
        const oldName = inlineAction.path.split(/[\/\\]/).pop() || "";
        const newPath = inlineAction.path.substring(0, inlineAction.path.length - oldName.length) + value;
        await invoke("rename_item", { old_path: inlineAction.path, new_path: newPath });
        addLog(`✨ Renamed to ${value}`);
        setOpenFiles(prev => prev.map(f => normPath(f.path) === normPath(inlineAction.path) ? { ...f, path: newPath, name: value } : f));
        if (normPath(activeFilePath) === normPath(inlineAction.path)) setActiveFilePath(newPath);
      } else {
        const parentPath = inlineAction.path;
        const sep = parentPath.includes('\\') ? '\\' : '/';
        const newPath = parentPath + sep + value;
        
        if (inlineAction.mode === "createFile") {
          await invoke("safe_write_project_file", { path: newPath, content: "" })
            .catch(async () => await invoke("write_project_file", { path: newPath, content: "" }));
          addLog(`✨ Created file ${value}`);
          
          await loadProjectFiles();
          
          const newFile: FileTab = { name: value, path: newPath, content: "", savedContent: "" };
          setOpenFiles(prev => [...prev, newFile]);
          setActiveFilePath(newPath);
        } else {
          await invoke("create_directory", { path: newPath });
          addLog(`✨ Created directory ${value}`);
        }
        setOpenFolders(prev => new Set(prev).add(parentPath));
      }
    } catch (err) {
      alert(`Operation failed:\n${err}`);
    }

    setInlineAction(null);
    loadProjectFiles();
  };

  const handleInlineCancel = () => setInlineAction(null);

  const loadProjectFiles = async () => {
    try {
      const files = await invoke("list_project_files", { path: projectDir });
      setProjectFiles(files as FileEntry[]);
    } catch (err) {
      console.error("Failed to load project files:", err);
    }
  };

  const loadSerialPorts = async () => {
    try {
      const ports = await invoke("list_serial_ports");
      const list = ports as string[];
      setSerialPorts(list);
      if (list.length > 0 && !selectedSerialPort) {
        setSelectedSerialPort(list[0]);
      }
    } catch (err) {
      addLog(`❌ Failed to list serial ports: ${err}`);
    }
  };

  const handleFileClick = async (path: string) => {
    const np = normPath(path);
    const existing = openFiles.find(f => normPath(f.path) === np);
    if (existing) {
      setActiveFilePath(existing.path);
      return;
    }

    try {
      const content = await invoke("read_project_file", { path });
      const newTab: FileTab = {
        name: path.split(/[\/\\]/).pop() || "unknown",
        path,
        content: content as string,
        savedContent: content as string
      };
      setOpenFiles(prev => [...prev, newTab]);
      setActiveFilePath(path);
      addLog(`Opened: ${newTab.name}`);
    } catch (err) {
      addLog(`❌ Failed to read file: ${err}`);
    }
  };

  const closeFile = (path: string, e: React.MouseEvent) => {
    e.stopPropagation();
    const np = normPath(path);
    setOpenFiles(prev => {
      const next = prev.filter(f => normPath(f.path) !== np);
      if (normPath(activeFilePath) === np) {
        setActiveFilePath(next.length > 0 ? next[next.length - 1].path : "");
      }
      return next;
    });
  };

  const updateActiveFileContent = async (newContent: string) => {
    if (!activeFilePath) {
      console.warn("Attempted to update content but no file is active.");
      addLog("❌ Vibe Code: No active file selected. Please click on a file in the sidebar to open it first.");
      return;
    }
    const np = normPath(activeFilePath);
    setOpenFiles(prev => prev.map(f =>
      normPath(f.path) === np ? { ...f, content: newContent, savedContent: newContent } : f
    ));

    try {
      await invoke("write_project_file", {
        path: activeFilePath,
        content: newContent
      });
      const fileName = activeFilePath.split(/[\/\\]/).pop() || activeFilePath;
      addLog(`✨ Vibe Code Applied & Saved to: ${fileName}`);
    } catch (err) {
      addLog(`❌ Failed to save injected code: ${err}`);
    }
  };

  const isFileDirty = (file: FileTab): boolean => {
    return file.content !== file.savedContent;
  };

  const handleEditorChange = (newContent: string) => {
    if (!activeFilePath) return;
    const np = normPath(activeFilePath);
    setOpenFiles(prev => prev.map(f =>
      normPath(f.path) === np ? { ...f, content: newContent } : f
    ));
  };

  const handleApplyToFile = async (filePath: string, newContent: string) => {
    const normSlash = (p: string) => p.replace(/\\/g, '/');
    const comparePath = (p: string) => normSlash(p).toLowerCase();

    let absolutePath: string;
    if (filePath.startsWith('/') || filePath.match(/^[a-zA-Z]:[\\\/]/)) {
      absolutePath = normSlash(filePath);
    } else {
      absolutePath = normSlash(`${projectDir}/${filePath}`);
    }

    const confirmApply = window.confirm(`Apply AI code to: ${filePath}?`);
    if (!confirmApply) return;

    try {
      await invoke("safe_write_project_file", { path: absolutePath, content: newContent });
      addLog(`✨ Vibe Code check & overwrite OK: ${filePath}`);

      const existingFile = openFilesRef.current.find(
        (f) => comparePath(f.path) === comparePath(absolutePath)
      );

      if (existingFile) {
        setOpenFiles((prev) =>
          prev.map((f) =>
            comparePath(f.path) === comparePath(absolutePath) ? { ...f, content: newContent } : f
          )
        );
        setActiveFilePath(existingFile.path);
      } else {
        const fileName = filePath.split(/[\/\\]/).pop() || 'file';
        const newTab: FileTab = { name: fileName, path: absolutePath, content: newContent, savedContent: newContent };
        setOpenFiles((prev) => [...prev, newTab]);
        setActiveFilePath(absolutePath);
      }
      loadProjectFiles();
    } catch (err) {
      addLog(`❌ Failed to overwrite file: ${err}`);
      alert(`Safety check failed:\n${err}`);
    }
  };

  const handleOpenProject = async () => {
    try {
      const path = await invoke("pick_directory");
      if (!path) return;

      const isValid = await invoke("validate_idf_project", { path });
      if (isValid) {
        setProjectDir(path as string);
        setOpenFiles([]);
        setActiveFilePath("");
        addLog(`Project opened: ${path}`);
      } else {
        alert("Selected directory is not a valid ESP-IDF project (missing CMakeLists.txt)");
      }
    } catch (err) {
      addLog(`Error opening project: ${err}`);
    }
  };


  const toggleFolder = (path: string) => {
    setOpenFolders((prev) => {
      const next = new Set(prev);
      if (next.has(path)) {
        next.delete(path);
      } else {
        next.add(path);
      }
      return next;
    });
  };

  const runEspIdfSetup = async () => {
    if (isSettingUpEspIdf) return;

    setIsSettingUpEspIdf(true);
    setEspIdfSetupNote("Downloading and installing ESP-IDF toolchain...");
    setSetupProgress(null);
    setStatus("Setting up ESP-IDF...");
    addLog("Starting ESP-IDF automated download and setup...");

    try {
      setEspIdfSetupNote("Fetching latest toolchain release info...");
      const response = await fetch("https://api.github.com/repos/Naruebodee8/vibeKidbright-Idf/releases/latest");
      const releaseData = await response.json();
      
      const toolAsset = releaseData.assets?.find((a: any) => a.name.toLowerCase().includes("tool") && a.name.endsWith(".zip"));
      const frameworkAsset = releaseData.assets?.find((a: any) => a.name.toLowerCase().includes("framework") && a.name.endsWith(".zip"));
      
      if (!toolAsset || !frameworkAsset) {
        throw new Error("Could not find both Tool and Framework zip files in the latest release on GitHub.");
      }
      
      addLog(`Found toolchain: ${toolAsset.name}`);
      setEspIdfSetupNote(`Downloading ${toolAsset.name} (1/2)...`);
      await invoke("download_portable_toolchain", {
        url: toolAsset.browser_download_url,
        force: true // Force only on the first file to clear old corrupted setups
      });

      addLog(`Found framework: ${frameworkAsset.name}`);
      setEspIdfSetupNote(`Downloading ${frameworkAsset.name} (2/2)...`);
      const result = await invoke("download_portable_toolchain", {
        url: frameworkAsset.browser_download_url,
        force: false // Do NOT force on the second file, we want to extract into the same dir
      });
      addLog(`${result}`);
      setEspIdfSetupNote("ESP-IDF installed successfully.");
      setSetupProgress(null);
      await checkEnvironment();
    } catch (err) {
      const message = `ESP-IDF setup failed: ${err}`;
      setStatus("ESP-IDF setup failed");
      setEspIdfSetupNote("Setup failed. Check logs and retry.");
      addLog(`❌ ${message}`);
    } finally {
      setIsSettingUpEspIdf(false);
    }
  };

  const checkEnvironment = async () => {
    try {
      const result = await invoke("check_esp_idf");
      setStatus(result as string);
      setEspIdfSetupNote("");
    } catch (err) {
      setStatus("ESP-IDF not found");
      setEspIdfSetupNote("ESP-IDF is required. Starting bootstrap installer...");
      addLog("ESP-IDF not found. Running bootstrap installer...");
      await runEspIdfSetup();
      console.error(err);
    }
  };

  const toggleSerialMonitor = async () => {
    if (isSerialConnected) {
      try {
        const result = await invoke("stop_serial_monitor");
        addLog(`${result}`);
        setIsSerialConnected(false);
      } catch (err) {
        addLog(`❌ Failed to stop serial monitor: ${err}`);
      }
      return;
    }

    if (!selectedSerialPort) {
      addLog("❌ No serial port selected");
      return;
    }

    try {
      const result = await invoke("start_serial_monitor", {
        port: selectedSerialPort,
        baudRate: Number(serialBaud) || 115200
      });
      addLog(`${result}`);
      setIsSerialConnected(true);
    } catch (err) {
      addLog(`❌ Failed to start serial monitor: ${err}`);
    }
  };

  const sendSerialText = async () => {
    if (!terminalInput.trim()) return;
    if (!isSerialConnected) {
      addLog("❌ Serial monitor is not connected");
      return;
    }

    try {
      let payload = terminalInput;
      if (payload.endsWith("\r\n")) {
      } else if (payload.endsWith("\n")) {
        payload = payload.slice(0, -1) + "\r\n";
      } else {
        payload += "\r\n";
      }
      
      await invoke("send_serial_input", { input: payload });
      addLog(`[SERIAL TX] ${terminalInput}`);
      setTerminalInput("");
    } catch (err) {
      addLog(`❌ Failed to send serial input: ${err}`);
    }
  };

  const addLog = (msg: string) => {
    setLogs((prev) => [...prev, `[${new Date().toLocaleTimeString()}] ${msg}`]);
  };

  const handleTerminalSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!terminalInput.trim()) return;

    if (isSerialConnected) {
      await sendSerialText();
      return;
    }

    const parts = terminalInput.trim().split(" ");
    const cmd = parts[0];
    const args = parts.slice(1);

    addLog(`> ${terminalInput}`);
    setTerminalInput("");

    try {
      // 🛠 ใช้ Wrapper ถ้าเป็นคำสั่งของ IDF
      if (["idf.py", "ninja", "cmake", "esptool.py"].includes(cmd)) {
        await runIdfWrappedCommand(cmd, args, projectDir === "." ? null : projectDir);
      } else {
        await invoke("run_shell_command", {
          cmd,
          args,
          cwd: projectDir === "." ? null : projectDir
        });
      }
    } catch (err) {
      addLog(`Error: ${err}`);
    }
  };


  const handleNewProject = () => {
    setShowNewProjectModal(true);
    setNewProjectName("my_esp_project");
    setNewProjectPath("");
  };

  const handlePickDirectory = async () => {
    try {
      const path = await invoke("pick_directory");
      if (path) {
        setNewProjectPath(path as string);
      }
    } catch (err) {
      addLog(`Directory picker error: ${err}`);
    }
  };

  const confirmCreateProject = async () => {
    if (!newProjectPath || !newProjectName) return;

    setShowNewProjectModal(false);
    addLog(`Attempting to create project '${newProjectName}' at ${newProjectPath}...`);

    try {
      const result = await invoke("create_idf_project", {
        path: newProjectPath,
        name: newProjectName
      });
      addLog(`Success: ${result}`);

      const fullPath = `${newProjectPath}/${newProjectName}`;
      setProjectDir(fullPath);
      addLog(`Active project set to: ${fullPath}`);
    } catch (err) {
      console.error("New project error:", err);
      addLog(`❌ ERROR: ${err}`);
      alert(`Failed to create project: ${err}`);
    }
  };

  const handleBuildFlash = async () => {
    if (isBuilding) return;
    
    if (projectDir === ".") {
      addLog("❌ Error: No project selected to build.");
      alert("Please open or create a project first from the sidebar.");
      return;
    }

    setIsBuilding(true);
    addLog("--- Starting Build & Flash ---");

    try {
      // 🛠 ใช้ Wrapper เพื่อให้มันโหลด export.bat ก่อนสั่ง idf.py เสมอ
      await runIdfWrappedCommand("idf.py", ["build", "flash"], projectDir);
    } catch (err) {
      addLog(`Build failed: ${err}`);
    } finally {
      setIsBuilding(false);
    }
  };

  const handleOpenFile = async (path: string, isKb?: boolean) => {
    const np = normPath(path);
    const existing = openFiles.find(f => normPath(f.path) === np);
    if (existing) {
      setActiveFilePath(existing.path);
      return;
    }

    try {
      let content: string;
      if (isKb) {
        content = await invoke("read_kb_file", { projectDir, fileName: path });
      } else {
        content = await invoke("read_project_file", { path });
      }
      
      const newTab: FileTab = {
        name: path.split(/[\/\\]/).pop() || "unknown",
        path,
        content: content as string,
        savedContent: content as string
      };
      setOpenFiles(prev => [...prev, newTab]);
      setActiveFilePath(path);
      addLog(`Opened ${isKb ? 'KB ' : ''}file: ${newTab.name}`);
    } catch (err) {
      addLog(`❌ Failed to read file: ${err}`);
    }
  };

  return (
    <div className="flex h-screen w-screen bg-[#020617] text-slate-200 overflow-hidden font-sans selection:bg-blue-500/30">
      {/* Sidebar */}
      <div className="w-64 border-r border-[var(--border-subtle)] bg-[var(--bg-surface)]/50 backdrop-blur-xl flex flex-col shrink-0 z-20">
        <div className="p-4 flex items-center gap-3 border-b border-[var(--border-normal)]">
          <div className="w-8 h-8 rounded-lg bg-gradient-to-br from-blue-500 to-cyan-500 flex items-center justify-center shadow-lg shadow-blue-500/20">
            <span className="text-white text-lg font-bold">V</span>
          </div>
          <div>
            <h1 className="text-sm font-bold tracking-tight text-[var(--text-primary)]">vibeKidbright</h1>
            <p className="text-[10px] text-[var(--text-muted)] font-medium">ESP-IDF IDE & AI Assistant</p>
          </div>
        </div>

        <div className="flex-1 overflow-y-auto py-2 no-scrollbar">
          <div className="flex items-center justify-between px-4 py-1 text-[10px] font-bold text-[var(--text-muted)] uppercase tracking-[0.2em] mb-1">
            PROJECT
            <div className="flex items-center gap-1">
              <button
                onClick={handleOpenProject}
                className="text-[10px] bg-[var(--bg-elevated)] hover:bg-[var(--bg-overlay)] text-[var(--text-primary)] px-1.5 py-0.5 rounded transition-colors border border-[var(--border-subtle)]"
              >
                OPEN
              </button>
              <button
                onClick={handleNewProject}
                className="text-[10px] bg-[var(--bg-elevated)] hover:bg-[var(--bg-overlay)] text-[var(--text-primary)] px-1.5 py-0.5 rounded transition-colors border border-[var(--border-subtle)]"
              >
                NEW
              </button>
            </div>
          </div>
          <div className="space-y-1">
            {(inlineAction?.mode === "createFile" || inlineAction?.mode === "createDir") && inlineAction.path === projectDir && (
              <div className="w-full text-left py-[5px] px-2 flex items-center gap-1.5 rounded-md relative text-[var(--text-primary)]">
                {inlineAction.mode === "createDir" ? (
                  <svg className="w-4 h-4 shrink-0" viewBox="0 0 24 24" fill="none"><path d="M3 7v10a2 2 0 002 2h14a2 2 0 002-2V9a2 2 0 00-2-2h-6l-2-2H5a2 2 0 00-2 2z" fill="#38bdf8" fillOpacity="0.08" stroke="#475569" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" /></svg>
                ) : (
                  <svg className="w-4 h-4 shrink-0" viewBox="0 0 24 24" fill="none"><path d="M14 2H6a2 2 0 00-2 2v16a2 2 0 002 2h12a2 2 0 002-2V8l-6-6z" stroke="#475569" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" /><path d="M14 2v6h6" stroke="#475569" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round" /></svg>
                )}
                <input
                  autoFocus
                  value={inlineInputValue}
                  onChange={(e) => setInlineInputValue(e.target.value)}
                  onKeyDown={e => {
                    if (e.key === "Enter") handleInlineSubmit();
                    if (e.key === "Escape") handleInlineCancel();
                  }}
                  onBlur={handleInlineCancel}
                  className="flex-1 bg-[var(--bg-elevated)] text-[12px] border border-[var(--accent-500)] rounded px-1 outline-none relative z-10 w-full"
                />
              </div>
            )}
            {projectFiles.map((file) => (
              <FileTreeItem
                key={file.path}
                item={file}
                activeFile={activeFilePath}
                openFolders={openFolders}
                onFileClick={handleFileClick}
                onFolderToggle={toggleFolder}
                onContextMenu={handleContextMenu}
                inlineAction={inlineAction}
                inlineInputValue={inlineInputValue}
                setInlineInputValue={setInlineInputValue}
                onInlineInputSubmit={handleInlineSubmit}
                onInlineInputCancel={handleInlineCancel}
              />
            ))}
            {projectFiles.length === 0 && !inlineAction && (
              <div className="text-[10px] text-[var(--text-muted)] p-2 italic">
                No files found
              </div>
            )}
          </div>
          <div className="px-2 py-1 text-[10px] text-[var(--text-muted)] truncate italic">
            {projectDir === "." ? "No project selected" : projectDir}
          </div>

          <div className="p-2 text-sm font-medium text-[var(--text-secondary)] mt-4">TOOLS</div>
          <button
            onClick={() => setShowSetupModal(true)}
            disabled={isSettingUpEspIdf}
            className={`w-full text-left p-2 rounded flex items-center gap-2 text-sm transition-colors group ${
              isSettingUpEspIdf
                ? "bg-amber-500/10 text-amber-300 cursor-not-allowed"
                : "hover:bg-[var(--bg-elevated)]/50 text-[var(--text-primary)]"
            }`}
          >
            <span className={`w-4 h-4 flex items-center justify-center rounded text-[10px] font-bold ${isSettingUpEspIdf ? "bg-amber-400/30 text-amber-200" : "bg-[var(--bg-overlay)] text-[var(--text-primary)]"}`}>
              {isSettingUpEspIdf ? "…" : "⚙"}
            </span>
            {isSettingUpEspIdf ? "Installing ESP-IDF..." : "Setup / Repair ESP-IDF"}
          </button>
          <button
            onClick={() => setShowAiPanel(!showAiPanel)}
            className={`w-full text-left p-2 rounded flex items-center gap-2 text-sm transition-colors group ${showAiPanel ? "bg-violet-500/10 text-violet-300" : "hover:bg-[var(--bg-elevated)]/50 text-[var(--text-secondary)]"
              }`}
          >
            <span className={`w-4 h-4 flex items-center justify-center rounded text-[10px] font-bold ${showAiPanel ? "bg-violet-500/30 text-violet-300" : "bg-[var(--bg-overlay)] text-[var(--text-secondary)] group-hover:bg-neutral-600"
              }`}>✦</span>
            Vibe Coder
          </button>
          <button
            onClick={() => setShowThemeModal(true)}
            className="w-full text-left p-2 rounded flex items-center gap-2 text-sm transition-colors hover:bg-[#162033]/60 text-[var(--text-secondary)]"
          >
            <span
              className="w-4 h-4 flex items-center justify-center rounded text-[10px] font-bold"
              style={{ background: currentTheme.color + "33", color: currentTheme.color }}
            >🎨</span>
            Theme: {currentTheme.label}
          </button>
        </div>

        <div className="p-4 border-t border-[var(--border-subtle)] bg-[var(--bg-surface)]/50">
          <div className="flex items-center gap-2 text-xs mb-3">
            <div className={`w-2 h-2 rounded-full ${status.includes("Ready") || status.includes("OK") ? "bg-emerald-500 shadow-[0_0_8px_rgba(16,185,129,0.6)]" : "bg-amber-500"}`}></div>
            <span className="text-[var(--text-secondary)] truncate font-medium">{status.split(":")[0]}</span>
          </div>
          {espIdfSetupNote && (
            <div className="text-[10px] leading-relaxed text-[var(--text-muted)] mb-3 rounded border border-[var(--border-subtle)] bg-[var(--bg-base)]/70 p-2">
              {espIdfSetupNote}
              {setupProgress && (
                <div className="mt-2">
                  <div className="w-full bg-[var(--bg-overlay)] rounded-full h-1.5 mb-1 overflow-hidden">
                    <div className="bg-blue-500 h-1.5 rounded-full transition-all duration-300" style={{ width: `${setupProgress.percent}%` }}></div>
                  </div>
                  <div className="flex justify-between text-[9px] text-[var(--text-secondary)]">
                    <span className="truncate pr-2">{setupProgress.message}</span>
                    <span className="shrink-0">{Math.round(setupProgress.percent)}%</span>
                  </div>
                </div>
              )}
            </div>
          )}
          <button
            onClick={handleBuildFlash}
            disabled={isBuilding || isSettingUpEspIdf}
            className={`w-full justify-center btn-primary glow-pulse ${isBuilding ? "opacity-70 cursor-not-allowed" : ""}`}
            style={{ 
              backgroundColor: isBuilding ? "var(--bg-overlay)" : "var(--btn-primary-bg)",
              boxShadow: isBuilding ? "none" : "0 4px 16px var(--btn-primary-glow)",
              color: isBuilding ? "var(--text-muted)" : "var(--btn-primary-text)"
            }}
          >
            {isBuilding ? (
              <>
                <div className="w-3.5 h-3.5 border-2 border-neutral-500 border-t-[var(--accent-400)] rounded-full animate-spin shrink-0" />
                <span className="tracking-wide font-bold">Building...</span>
              </>
            ) : (
              <>
                <svg className="w-4 h-4 text-amber-400 drop-shadow-md shrink-0" fill="currentColor" viewBox="0 0 20 20">
                  <path fillRule="evenodd" d="M11.3 1.046A1 1 0 0112 2v5h4a1 1 0 01.82 1.573l-7 10A1 1 0 018 18v-5H4a1 1 0 01-.82-1.573l7-10a1 1 0 011.12-.38z" clipRule="evenodd" />
                </svg>
                <span className="tracking-wide font-bold">Build & Flash</span>
              </>
            )}
          </button>
        </div>
      </div>

      {/* Main Area */}
      <div className="flex-1 flex flex-col min-w-0">
        <div className="flex-1 bg-[var(--bg-base)] overflow-hidden relative">
          <div className="absolute inset-0 flex flex-col">
            {/* Tab Bar */}
            <div className="h-10 border-b border-[var(--border-normal)] flex items-center justify-between bg-[var(--bg-surface)]/90 backdrop-blur-sm z-10 overflow-hidden">
              <div className="flex items-center overflow-x-auto no-scrollbar flex-1">
                {openFiles.map((file) => (
                  <div
                    key={file.path}
                    onClick={() => setActiveFilePath(file.path)}
                    className={`flex items-center gap-2 px-4 h-full border-r border-[var(--border-subtle)] cursor-pointer transition-colors text-xs font-medium whitespace-nowrap ${normPath(activeFilePath) === normPath(file.path)
                      ? "bg-[var(--bg-base)] border-b-2"
                      : "text-[var(--text-muted)] hover:text-[var(--text-primary)] hover:bg-[var(--bg-overlay)]/50"
                      }`}
                    style={normPath(activeFilePath) === normPath(file.path) ? { color: "var(--accent-400)", borderBottomColor: "var(--accent-400)" } : {}}
                  >
                    {/* Unsaved changes indicator */}
                    {isFileDirty(file) && (
                      <span className="w-2 h-2 rounded-full bg-amber-400 shadow-[0_0_6px_rgba(251,191,36,0.5)] shrink-0" title="Unsaved changes" />
                    )}
                    <span>{file.name}</span>
                    <button
                      onClick={(e) => closeFile(file.path, e)}
                      className="transition-colors p-0.5 rounded-sm ml-1"
                      style={{ color: "inherit" }}
                      onMouseEnter={(e) => e.currentTarget.style.color = "var(--accent-400)"}
                      onMouseLeave={(e) => e.currentTarget.style.color = "inherit"}
                    >
                      {isFileDirty(file) ? "●" : "×"}
                    </button>
                  </div>
                ))}
                {openFiles.length === 0 && (
                  <div className="px-4 text-xs text-[var(--text-muted)] italic">No files open</div>
                )}
              </div>

              {activeFile && (
                <div className="flex items-center gap-2 px-3 border-l border-[var(--border-subtle)] shrink-0 h-full bg-[var(--bg-base)]/40">
                  <button
                    onClick={() => reloadFile(activeFile.path)}
                    className="p-1.5 text-[var(--text-muted)] transition-colors rounded hover:bg-[var(--bg-overlay)]"
                    style={{ color: "var(--text-secondary)" }}
                    onMouseEnter={(e) => e.currentTarget.style.color = "var(--accent-400)"}
                    onMouseLeave={(e) => e.currentTarget.style.color = "var(--text-secondary)"}
                    title="Reload from disk"
                  >
                    <svg className="w-3.5 h-3.5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                      <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15" />
                    </svg>
                  </button>
                  <button
                    onClick={handleSaveProjectAs}
                    className="flex items-center gap-1.5 px-3 py-1 bg-emerald-600/20 hover:bg-emerald-600 text-[10px] font-bold text-emerald-400 hover:text-white rounded transition-all active:scale-95 uppercase tracking-wider"
                    title="Save Project As... (copies the whole folder to a new location)"
                  >
                    <svg className="w-3 h-3" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                      <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M8 7H5a2 2 0 00-2 2v9a2 2 0 002 2h14a2 2 0 002-2V9a2 2 0 00-2-2h-3m-1 4l-3 3m0 0l-3-3m3 3V4" />
                    </svg>
                    Save Project
                  </button>
                </div>
              )}
            </div>
            {activeFile ? (
              <CodeEditor
                key={activeFile.path}
                value={activeFile.content}
                onChange={handleEditorChange}
                filePath={activeFile.path}
                onSave={saveAllFiles}
                activeTheme={activeTheme}
              />
            ) : (
              <div className="flex-1 flex items-center justify-center bg-[var(--bg-base)]">
                <div className="text-center space-y-3 opacity-40">
                  <div className="text-4xl">✨</div>
                  <p className="text-sm text-[var(--text-muted)] font-medium">
                    {projectDir === "." ? "Open or create a project to start coding" : "Select a file from the sidebar"}
                  </p>
                  <p className="text-xs text-[var(--text-muted)]">
                    Ctrl+S to save • Syntax highlighting for C, Python, JSON & more
                  </p>
                </div>
              </div>
            )}
          </div>
        </div>

        {/* Console & Terminal */}
        <div className="h-80 border-t border-[var(--border-subtle)] bg-[var(--bg-base)] flex flex-col shadow-2xl">
          <div className="h-9 border-b border-[var(--border-subtle)] flex items-center justify-between px-4 bg-[var(--bg-base)]/40">
            <span className="text-[10px] font-bold text-[var(--text-muted)] uppercase tracking-[0.2em]">Interactive Terminal</span>
            <div className="flex items-center gap-2">
              <div className="flex items-center">
                <select
                  value={selectedSerialPort}
                  onChange={(e) => setSelectedSerialPort(e.target.value)}
                  onClick={loadSerialPorts}
                  className="bg-[var(--bg-elevated)] border border-[var(--border-normal)] rounded px-2 py-1 text-[10px] text-[var(--text-primary)] w-[100px] focus:outline-none cursor-pointer"
                  style={{ outlineColor: "var(--accent-500)" }}
                  title="Auto-refreshing Serial Ports"
                >
                  {serialPorts.length === 0 ? (
                    <option value="" disabled>No Ports</option>
                  ) : (
                    <>
                      {!selectedSerialPort && <option value="" disabled>Select Port</option>}
                      {serialPorts.map((port) => (
                        <option key={port} value={port}>{port}</option>
                      ))}
                    </>
                  )}
                </select>
              </div>
              <input
                type="text"
                value={serialBaud}
                onChange={(e) => setSerialBaud(e.target.value)}
                className="w-20 bg-[var(--bg-elevated)] border border-[var(--border-normal)] rounded px-2 py-1 text-[10px] text-[var(--text-primary)]"
              />
              <button
                onClick={loadSerialPorts}
                className="text-[10px] px-2 py-1 rounded bg-[var(--bg-elevated)] text-[var(--text-primary)] hover:bg-[var(--bg-overlay)]"
              >
                Refresh Ports
              </button>
              <button
                onClick={toggleSerialMonitor}
                className={`text-[10px] px-2 py-1 rounded font-bold transition-all ${
                  isSerialConnected
                    ? "bg-red-900/70 text-red-200 hover:bg-red-800/80 border border-red-700/50"
                    : "bg-emerald-900/70 text-emerald-200 hover:bg-emerald-800/80 border border-emerald-700/50"
                }`}
              >
                {isSerialConnected ? "⛔ Disconnect" : "🔌 Connect"}
              </button>
              <button
                onClick={() => setLogs([])}
                className="text-[10px] text-[var(--text-muted)] hover:text-[var(--text-primary)] transition-colors uppercase font-bold"
              >
                Clear Logs
              </button>
            </div>
          </div>
          <div
            ref={scrollRef}
            className="flex-1 overflow-y-auto p-4 font-mono text-xs space-y-1 selection:bg-blue-500/20"
          >
            {logs.length === 0 ? (
              <div className="text-[var(--text-muted)] italic opacity-50">vibeKidbright Terminal Ready. Type 'idf.py --version' to test.</div>
            ) : (
              logs.map((log, i) => (
                <div key={i} className="flex gap-2 text-[var(--text-secondary)]/90 hover:text-[var(--text-primary)] transition-colors">
                  <span className="whitespace-pre-wrap break-all">{log}</span>
                </div>
              ))
            )}
          </div>
          {/* Terminal Input */}
          <div className="p-2 bg-[var(--bg-surface)]/40 border-t border-[var(--border-subtle)] flex items-center gap-2 group">
            <span className="font-bold text-sm ml-2" style={{ color: "var(--accent-500)" }}>$</span>
            <form onSubmit={handleTerminalSubmit} className="flex-1">
              <input
                type="text"
                value={terminalInput}
                onChange={(e) => setTerminalInput(e.target.value)}
                placeholder={isSerialConnected ? "Type message and press Enter to send to board..." : "Type command (e.g. idf.py) and press Enter..."}
                className="w-full bg-transparent border-none focus:outline-none font-mono text-sm text-[var(--text-primary)] placeholder:text-[var(--text-muted)]"
              />
            </form>
            <button
              onClick={sendSerialText}
              className="text-[10px] px-2 py-1 rounded bg-[var(--bg-elevated)] hover:bg-[var(--bg-overlay)] text-[var(--text-primary)]"
            >
              Send Serial
            </button>
          </div>
        </div>
      </div>

      {/* AI Chat Panel */}
      {showAiPanel && (
        <div className="w-96 border-l border-[var(--border-subtle)] relative flex flex-col">
          <AiChat
            projectDir={projectDir}
            onInjectCode={(newCode) => updateActiveFileContent(newCode)}
            onApplyToFile={handleApplyToFile}
            onOpenFile={handleOpenFile}
          />
        </div>
      )}

      {/* New Project Modal */}
      {showNewProjectModal && (
        <div className="absolute inset-0 bg-black/60 backdrop-blur-sm flex items-center justify-center z-50">
          <div className="bg-[var(--bg-surface)] border border-[var(--border-normal)] rounded-xl p-6 w-96 shadow-2xl">
            <h3 className="text-lg font-bold text-[var(--text-primary)] mb-4 flex items-center gap-2">
              <span style={{ color: "var(--accent-400)" }}>📁</span> Create New Project
            </h3>

            <div className="space-y-4">
              <div>
                <label className="text-xs text-[var(--text-secondary)] mb-1 block uppercase font-bold tracking-wider">
                  Project Name
                </label>
                <input
                  type="text"
                  value={newProjectName}
                  onChange={(e) => setNewProjectName(e.target.value)}
                  placeholder="my_esp_project"
                  className="w-full bg-[var(--bg-elevated)] border border-[var(--border-normal)] rounded-lg px-3 py-2 text-sm text-[var(--text-primary)] focus:outline-none transition-colors"
                  style={{ outlineColor: "var(--accent-500)" }}
                />
              </div>

              <div>
                <label className="text-xs text-[var(--text-secondary)] mb-1 block uppercase font-bold tracking-wider">
                  Location
                </label>
                <div className="flex gap-2">
                  <div className="flex-1 bg-[var(--bg-base)] border border-[var(--border-normal)] rounded-lg px-3 py-2 text-xs text-[var(--text-secondary)] truncate flex items-center">
                    {newProjectPath || "No directory selected"}
                  </div>
                  <button
                    onClick={handlePickDirectory}
                    className="px-3 py-2 bg-[var(--bg-overlay)] hover:bg-neutral-600 text-xs text-[var(--text-primary)] rounded-lg transition-colors shrink-0"
                  >
                    Browse
                  </button>
                </div>
              </div>
            </div>

            <div className="flex gap-3 mt-8">
              <button
                onClick={() => setShowNewProjectModal(false)}
                className="flex-1 py-2 bg-[var(--bg-overlay)] hover:bg-neutral-600 text-sm text-[var(--text-primary)] rounded-lg transition-colors font-medium"
              >
                Cancel
              </button>
              <button
                onClick={confirmCreateProject}
                disabled={!newProjectName || !newProjectPath}
                className={`flex-1 py-2 rounded-lg transition-all font-bold text-sm ${!newProjectName || !newProjectPath
                  ? "bg-[var(--bg-overlay)] text-[var(--text-muted)] cursor-not-allowed"
                  : "btn-primary shadow-lg active:scale-95"
                  }`}
              >
                Create Project
              </button>
            </div>
          </div>
        </div>
      )}

      {/* ── Setup / Repair ESP-IDF Modal ── */}
      {showSetupModal && (
        <div className="absolute inset-0 bg-black/60 backdrop-blur-sm flex items-center justify-center z-50">
          <div className="bg-[var(--bg-elevated)] border border-[var(--border-normal)] rounded-xl p-6 w-[480px] shadow-2xl">
            <h3 className="text-lg font-bold text-[var(--text-primary)] mb-1 flex items-center gap-2">
              <span className="text-amber-400">⚙</span> Setup / Repair ESP-IDF
            </h3>
            <p className="text-xs text-[var(--text-muted)] mb-5">
              Choose how to configure your ESP-IDF environment
            </p>

            {/* Tab: Manual Path */}
            <div className="bg-[var(--bg-base)]/60 border border-[var(--border-normal)] rounded-lg p-4 mb-4">
              <div className="flex items-center gap-2 mb-3">
                <span className="text-emerald-400 text-sm font-bold">📁 Manual Path</span>
                <span className="text-[10px] text-[var(--text-muted)] bg-[var(--bg-overlay)] px-1.5 py-0.5 rounded">Recommended if ESP-IDF already installed</span>
              </div>

              {setupModalError && (
                <div className="mb-3 p-2 bg-red-500/10 border border-red-500/20 rounded text-xs text-red-400">
                  {setupModalError}
                </div>
              )}

              <div className="space-y-3">
                <div>
                  <label className="text-[10px] text-[var(--text-secondary)] uppercase font-bold tracking-wider mb-1 block">
                    ESP-IDF Framework Path <span className="text-[var(--text-muted)]">(contains tools/idf.py)</span>
                  </label>
                  <div className="flex gap-2">
                    <div className="flex-1 bg-[var(--bg-elevated)] border border-[var(--border-bright)] rounded-lg px-3 py-2 text-xs text-[var(--text-primary)] truncate flex items-center font-mono">
                      {customIdfPath || <span className="text-[var(--text-muted)] italic">e.g. C:\Espressif\frameworks\esp-idf-v5.4.3</span>}
                    </div>
                    <button
                      onClick={handlePickIdfPath}
                      className="px-3 py-2 bg-[var(--bg-overlay)] hover:bg-neutral-600 text-xs text-emerald-300 rounded-lg transition-colors shrink-0 font-bold border border-emerald-500/20"
                    >
                      Browse
                    </button>
                  </div>
                </div>

                <div>
                  <label className="text-[10px] text-[var(--text-secondary)] uppercase font-bold tracking-wider mb-1 block">
                    ESP-IDF Tools Path <span className="text-[var(--text-muted)]">(contains python_env, tools folders)</span>
                  </label>
                  <div className="flex gap-2">
                    <div className="flex-1 bg-[var(--bg-elevated)] border border-[var(--border-bright)] rounded-lg px-3 py-2 text-xs text-[var(--text-primary)] truncate flex items-center font-mono">
                      {customToolsPath || <span className="text-[var(--text-muted)] italic">e.g. D:\Espressif</span>}
                    </div>
                    <button
                      onClick={handlePickToolsPath}
                      className="px-3 py-2 bg-[var(--bg-overlay)] hover:bg-neutral-600 text-xs text-emerald-300 rounded-lg transition-colors shrink-0 font-bold border border-emerald-500/20"
                    >
                      Browse
                    </button>
                  </div>
                </div>

                <div className="flex gap-2 pt-1">
                  <button
                    onClick={handleSaveCustomPaths}
                    disabled={isSavingPaths || !customIdfPath || !customToolsPath}
                    className={`flex-1 py-2 rounded-lg text-sm font-bold transition-all ${
                      isSavingPaths || !customIdfPath || !customToolsPath
                        ? "bg-[var(--bg-overlay)] text-[var(--text-muted)] cursor-not-allowed"
                        : "bg-emerald-600 hover:bg-emerald-500 text-white shadow-lg shadow-emerald-500/20 active:scale-95"
                    }`}
                  >
                    {isSavingPaths ? "Saving..." : "Save & Apply"}
                  </button>
                  {(customIdfPath || customToolsPath) && (
                    <button
                      onClick={handleClearCustomPaths}
                      className="px-4 py-2 bg-[var(--bg-overlay)] hover:bg-red-900/50 text-xs text-[var(--text-secondary)] hover:text-red-300 rounded-lg transition-colors"
                      title="Clear custom paths and use auto-detection"
                    >
                      Reset
                    </button>
                  )}
                </div>
              </div>
            </div>

            {/* Tab: Auto Install */}
            <div className="bg-[var(--bg-base)]/60 border border-[var(--border-normal)] rounded-lg p-4 mb-5">
              <div className="flex items-center gap-2 mb-2">
                <span className="text-amber-400 text-sm font-bold">⬇ Auto Install</span>
                <span className="text-[10px] text-[var(--text-muted)] bg-[var(--bg-elevated)] px-1.5 py-0.5 rounded">Downloads & installs ESP-IDF v5.2.2</span>
              </div>
              <p className="text-[11px] text-[var(--text-muted)] mb-3">
                Let the app download and configure ESP-IDF automatically. This may take 10–20 minutes depending on your connection speed.
              </p>
              <button
                onClick={() => { setShowSetupModal(false); runEspIdfSetup(); }}
                disabled={isSettingUpEspIdf}
                className="w-full py-2 bg-amber-700 hover:bg-amber-600 text-sm font-bold text-amber-100 rounded-lg transition-all active:scale-95 shadow-lg shadow-amber-500/20"
              >
                Start Auto Install
              </button>
            </div>

            <button
              onClick={() => setShowSetupModal(false)}
              className="w-full py-2 bg-[var(--bg-overlay)] hover:bg-neutral-600 text-sm text-[var(--text-primary)] rounded-lg transition-colors"
            >
              Cancel
            </button>
          </div>
        </div>
      )}

      {/* Context Menu Portal */}
      {contextMenu && (
        <div
          style={{ top: contextMenu.y, left: contextMenu.x }}
          className="fixed z-50 w-48 bg-[#0f172a] border border-[var(--border-normal)] rounded-md shadow-2xl py-1 transform scale-100 origin-top-left flex flex-col text-[13px] font-medium text-[var(--text-primary)]"
        >
          <button
            onClick={() => { setInlineAction({ mode: "rename", path: contextMenu.path }); setInlineInputValue(contextMenu.path.split(/[\/\\]/).pop() || ""); setContextMenu(null); }}
            className="w-full text-left px-3 py-1.5 hover:bg-blue-600/20 hover:text-[var(--accent-300)] transition-colors flex items-center gap-2"
          >
            <span>📝</span> Rename <span className="ml-auto text-[10px] text-[var(--text-muted)] font-sans">เปลี่ยนชื่อ</span>
          </button>
          
          {contextMenu.isDir && (
            <>
              <button
                onClick={() => { setInlineAction({ mode: "createFile", path: contextMenu.path }); setInlineInputValue(""); setContextMenu(null); }}
                className="w-full text-left px-3 py-1.5 hover:bg-blue-600/20 hover:text-[var(--accent-300)] transition-colors flex items-center gap-2 mt-1"
              >
                <span>➕</span> New File
              </button>
              <button
                onClick={() => { setInlineAction({ mode: "createDir", path: contextMenu.path }); setInlineInputValue(""); setContextMenu(null); }}
                className="w-full text-left px-3 py-1.5 hover:bg-blue-600/20 hover:text-[var(--accent-300)] transition-colors flex items-center gap-2"
              >
                <span>📁</span> New Folder
              </button>
            </>
          )}

          <div className="my-1 border-t border-[#1a3060]/60"></div>
          
          <button
            onClick={() => { handleDeleteItem(contextMenu.path, contextMenu.isDir); setContextMenu(null); }}
            className="w-full text-left px-3 py-1.5 hover:bg-red-900/30 text-red-400 hover:text-red-300 transition-colors flex items-center gap-2"
          >
            <span>🗑️</span> Delete <span className="ml-auto text-[10px] text-red-600/80 font-sans">ลบ</span>
          </button>
        </div>
      )}
      {/* ── Theme Settings Modal ── */}
      {showThemeModal && (
        <div className="absolute inset-0 bg-black/70 backdrop-blur-sm flex items-center justify-center z-50">
          <div className="bg-[var(--bg-elevated)] border border-[var(--border-normal)] rounded-xl p-6 w-[400px] shadow-2xl">
            <div className="flex items-center justify-between mb-5">
              <h3 className="text-base font-bold text-[var(--text-primary)] flex items-center gap-2">
                <span>🎨</span> ปรับธีมสีแอป
              </h3>
              <button
                onClick={() => setShowThemeModal(false)}
                className="text-[var(--text-muted)] hover:text-[var(--text-primary)] transition-colors"
              >
                <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" />
                </svg>
              </button>
            </div>

            <p className="text-xs text-[var(--text-muted)] mb-4">เลือกธีมสีที่ต้องการ — การเปลี่ยนธีมจะมีผลทันที</p>

            <div className="space-y-2">
              {THEMES.map(theme => (
                <button
                  key={theme.id}
                  onClick={() => setActiveTheme(theme.id)}
                  className={`w-full flex items-center gap-3 p-3 rounded-lg border transition-all ${
                    activeTheme === theme.id
                      ? "border-opacity-80 bg-opacity-10"
                      : "border-[var(--border-subtle)] bg-[var(--bg-surface)] hover:bg-[var(--bg-overlay)]"
                  }`}
                  style={activeTheme === theme.id ? {
                    borderColor: theme.color,
                    backgroundColor: theme.color + "18",
                  } : {}}
                >
                  {/* Color Swatch */}
                  <div
                    className="w-8 h-8 rounded-lg flex-shrink-0 shadow-lg"
                    style={{ background: `linear-gradient(135deg, ${theme.color}, ${theme.color}88)` }}
                  />
                  <div className="flex-1 text-left">
                    <div className="text-sm font-semibold text-[var(--text-primary)]">{theme.label}</div>
                    <div className="text-[11px] text-[var(--text-muted)]">{theme.desc}</div>
                  </div>
                  {activeTheme === theme.id && (
                    <svg className="w-5 h-5 flex-shrink-0" fill="none" stroke="currentColor" viewBox="0 0 24 24"
                      style={{ color: theme.color }}>
                      <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2.5} d="M5 13l4 4L19 7" />
                    </svg>
                  )}
                </button>
              ))}
            </div>

            {/* HCI Color Guide */}
            <div className="mt-5 p-3 bg-[var(--bg-surface)] border border-[var(--border-subtle)] rounded-lg">
              <p className="text-[10px] font-bold text-[var(--text-secondary)] uppercase tracking-wider mb-2">คำอธิบายสีปุ่มตามหลัก HCI</p>
              <div className="grid grid-cols-2 gap-1.5">
                <div className="flex items-center gap-1.5">
                  <div className="w-3 h-3 rounded" style={{ background: "var(--btn-primary-bg)" }} />
                  <span className="text-[10px] text-[var(--text-secondary)]">สีหลัก — Build, Save, Apply</span>
                </div>
                <div className="flex items-center gap-1.5">
                  <div className="w-3 h-3 rounded" style={{ background: "var(--btn-success-bg)" }} />
                  <span className="text-[10px] text-[var(--text-secondary)]">เขียว — Connect, Create</span>
                </div>
                <div className="flex items-center gap-1.5">
                  <div className="w-3 h-3 rounded" style={{ background: "var(--btn-danger-bg)" }} />
                  <span className="text-[10px] text-[var(--text-secondary)]">แดง — Delete, Disconnect</span>
                </div>
                <div className="flex items-center gap-1.5">
                  <div className="w-3 h-3 rounded" style={{ background: "var(--btn-warning-bg)" }} />
                  <span className="text-[10px] text-[var(--text-secondary)]">ส้ม — Warning, Flash</span>
                </div>
              </div>
            </div>

            <button
              onClick={() => setShowThemeModal(false)}
              className="mt-4 w-full justify-center btn-primary"
            >
              บันทึกธีม
            </button>
          </div>
        </div>
      )}
    </div>
  );
}

export default App;