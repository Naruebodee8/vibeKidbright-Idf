import { useEffect, useRef, useState } from "react";
import { invoke } from "@tauri-apps/api/core";
import { listen } from "@tauri-apps/api/event";
import { normPath } from "./utils";
import { BuzzerAudio, BuzzerState } from "./simulator/BuzzerAudio";
import { SimulatorAdapter, SimulatorEvent } from "./simulator/SimulatorAdapter";

const EXAMPLES = [
  { name: "Buzzer บี๊บ 1000 Hz", source: "sound.tone(1000);\nvTaskDelay(5000);\nsound.off();\n" },
  { name: "Buzzer โน้ต C–E–G", source: "sound.note(0); sound.rest(2);\nsound.note(4); sound.rest(2);\nsound.note(7); sound.rest(2);\nsound.off();\n" },
  { name: "ปุ่ม SW1 → Buzzer", source: "var pressed = 0;\nwhile (1) {\n  if (button12.sw1_get() && !pressed) {\n    sound.tone(1000); vTaskDelay(200); sound.off();\n  }\n  pressed = button12.sw1_get();\n  vTaskDelay(20);\n}\n" },
  { name: "OUT2 เปิด–ปิด", source: "ports.output2_write(1);\nvTaskDelay(500);\nports.output2_write(0);\n" },
  { name: "ปุ่ม S1 ควบคุม OUT2", source: "while (1) {\n  ports.output2_write(button12.is_sw1_pressed() ? 1 : 0);\n  vTaskDelay(50);\n}\n" },
  { name: "อุณหภูมิ → ตัวเลข LED", source: "while (1) {\n  matrix.show_number(lm73_1.get());\n  vTaskDelay(250);\n}\n" },
  { name: "LED สี่มุม", source: 'ht16k33.show("\\x81\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x81");\n' },
  { name: "Analog IN1 → ค่าดิบบน LED", source: "while (1) {\n  matrix.show_number(kb_specio.input1_read());\n  vTaskDelay(100);\n}\n" },
  { name: "Digital IN1 → 0/1 บน LED", source: "while (1) {\n  matrix.show_number(ports.input1_read());\n  vTaskDelay(100);\n}\n" },
];
interface HostInfo { url: string; origin: string; session_id: string }
interface PendingReview { path: string; relative_path: string }
interface ProjectSimulation { path: string; source: string; revision: string; firmware_files: string[] }
type Phase = "connecting" | "ready" | "loading" | "armed" | "running" | "stopping" | "error";

export default function SimulatorPanel({ projectDir, activeSource, activeName, initialSource, onDraftChange, beforeProjectRun, onReviewFile, onClose }: {
  projectDir: string; activeSource?: string; activeName?: string; initialSource?: string;
  onDraftChange: (source: string) => void; onClose: () => void;
  beforeProjectRun?: () => Promise<boolean>;
  onReviewFile?: (path: string) => void;
}) {
  const [host, setHost] = useState<HostInfo | null>(null);
  const [source, setSource] = useState(projectDir === "." ? initialSource ?? EXAMPLES[3].source : "");
  const [sourceMode, setSourceMode] = useState<"project" | "draft">(projectDir === "." ? "draft" : "project");
  const [sourceLoading, setSourceLoading] = useState(false);
  const [projectProgram, setProjectProgram] = useState<ProjectSimulation | null>(null);
  const [projectError, setProjectError] = useState("");
  const [pendingReviews, setPendingReviews] = useState<PendingReview[]>([]);
  const [projectLoadedAt, setProjectLoadedAt] = useState("");
  const [exporting, setExporting] = useState(false);
  const [exportResult, setExportResult] = useState<{ message: string; failed: boolean } | null>(null);
  const exportBusy = useRef(false);
  const [audioLevel, setAudioLevel] = useState(50);
  const [audioMuted, setAudioMuted] = useState(false);
  const [audioError, setAudioError] = useState("");
  const [buzzerState, setBuzzerState] = useState<BuzzerState>({ playing: false, frequency: 0, volume: 50 });
  const audio = useRef<BuzzerAudio | null>(null);
  const [temperature, setTemperature] = useState(25);
  const [temperatureMode, setTemperatureMode] = useState<"test" | "unity">("test");
  const [phase, setPhase] = useState<Phase>("connecting");
  const [progress, setProgress] = useState(0);
  const [error, setError] = useState("");
  const [logs, setLogs] = useState<string[]>([]);
  const [output, setOutput] = useState<Record<string, unknown>>({});
  const [snapshot, setSnapshot] = useState("");
  const frame = useRef<HTMLIFrameElement>(null);
  const adapter = useRef<SimulatorAdapter | null>(null);
  const run = useRef<string | null>(null);
  const ready = useRef(false);
  const generation = useRef(0);
  const busy = useRef(false);
  const connectionFailed = useRef(false);
  const sourceGeneration = useRef(0);
  const modeRef = useRef(sourceMode); modeRef.current = sourceMode;
  const addLog = (text: string) => setLogs(prev => [...prev.slice(-49), text.slice(0, 1000)]);
  const changeSource = (value: string) => { sourceGeneration.current++; setSourceLoading(false); setSourceMode("draft"); setProjectProgram(null); setProjectError(""); setSource(value); onDraftChange(value); setError(""); };

  function audioEngine() {
    if (!audio.current) audio.current = new BuzzerAudio();
    audio.current.setLevel(audioLevel); audio.current.setMuted(audioMuted);
    return audio.current;
  }
  function silenceAudio() {
    audio.current?.stop();
    setBuzzerState({ playing: false, frequency: 0, volume: 50 });
  }
  function closePanel() { silenceAudio(); onClose(); }
  function enableAudio() {
    const engine = audioEngine();
    engine.setMuted(false); setAudioMuted(false); setAudioError("");
    void engine.unlock().catch(error => setAudioError(String(error)));
  }
  async function loadProjectSource() {
    const token = ++sourceGeneration.current;
    setSourceMode("project"); setSourceLoading(true); setProjectError(""); setProjectProgram(null);
    try {
      const program = await invoke<ProjectSimulation>("load_project_simulation", { projectDir });
      if (sourceGeneration.current === token) {
        setSource(program.source); setProjectProgram(program); setPendingReviews([]);
        setProjectLoadedAt(new Date().toLocaleTimeString("th-TH"));
      }
    } catch (err) {
      if (sourceGeneration.current === token) {
        setSource(""); setProjectError(String(err));
        try {
          const reviews = await invoke<PendingReview[]>("list_pending_diffs", { projectDir });
          if (sourceGeneration.current === token) setPendingReviews(Array.isArray(reviews) ? reviews : []);
        } catch { if (sourceGeneration.current === token) setPendingReviews([]); }
      }
    } finally { if (sourceGeneration.current === token) setSourceLoading(false); }
  }
  useEffect(() => {
    if (projectDir !== ".") void loadProjectSource();
    return () => { sourceGeneration.current++; };
  }, [projectDir]);

  useEffect(() => {
    const unlisten = listen<{path:string}>("ai-diff-resolved", event => {
      if (projectDir !== "." && modeRef.current === "project" && !run.current &&
          normPath(event.payload.path).startsWith(normPath(projectDir).replace(/\/$/,"") + "/")) void loadProjectSource();
    });
    return () => { void unlisten.then(fn => fn()).catch(() => {}); };
  }, [projectDir]);

  useEffect(() => {
    if (!ready.current || !adapter.current) return;
    void adapter.current.request("SET_INPUTS", { temperature: temperatureMode === "test" ? temperature : null })
      .catch(err => setError(String(err)));
  }, [host, progress, temperature, temperatureMode]);

  useEffect(() => {
    const session = crypto.randomUUID();
    let disposed = false;
    const startupTimeout = setTimeout(() => {
      if (!disposed && !ready.current && !connectionFailed.current) {
        connectionFailed.current = true;
        setError("โหลดบอร์ดในเครื่องไม่สำเร็จภายใน 3 นาที กรุณาปิดแล้วเปิด Simulator ใหม่");
        setPhase("error");
      }
    }, 180000);
    invoke<HostInfo>("start_simulator_host", { sessionId: session, parentOrigin: window.location.origin })
      .then(info => {
        if (disposed) { void invoke("stop_simulator_host", { sessionId: session }).catch(() => {}); return; }
        setHost(info);
      }).catch(err => { if (!disposed) { connectionFailed.current = true; setError(String(err)); setPhase("error"); } });
    return () => {
      disposed = true;
      clearTimeout(startupTimeout);
      generation.current++;
      audio.current?.dispose(); audio.current = null;
      adapter.current?.close();
      adapter.current = null;
      void invoke("stop_simulator_host", { sessionId: session }).catch(() => {});
    };
  }, [projectDir]);

  function handleEvent(event: SimulatorEvent) {
    if (event.type === "READY") {
      ready.current = true;
      setProgress(100);
      setPhase(prev => prev === "connecting" ? "ready" : prev);
    } else if (event.type === "LOADING") setProgress(Number(event.payload.percent) || 0);
    else if (event.type === "RUN_ARMED" && event.runId === run.current) setPhase("armed");
    else if (event.type === "RUN_STARTED") {
      // Unity's own Run control may also start the loaded program.
      if (!run.current || run.current === event.runId) {
        run.current = event.runId; setPhase("running");
      }
    } else if (event.type === "BUZZER" && event.runId && event.runId === run.current) {
      try {
        if (audio.current?.update(event.runId, event.payload)) setBuzzerState(event.payload as unknown as BuzzerState);
      } catch (error) { silenceAudio(); setAudioError(String(error)); }
    } else if (event.type === "RUN_ENDED" && event.runId === run.current) {
      silenceAudio();
      run.current = null;
      setPhase("ready");
      addLog(event.payload.reason === "completed" ? "โปรแกรมจำลองจบแล้ว" : event.payload.reason === "stopped" ? "หยุดโปรแกรมจำลองแล้ว" : "โปรแกรมจำลองล้มเหลว");
      if (event.payload.message) setError(String(event.payload.message));
    } else if (event.type === "OUTPUT" && (!event.runId || event.runId === run.current)) setOutput(event.payload);
    else if (event.type === "ERROR") {
      silenceAudio();
      setError(String(event.payload.message || "Simulator error"));
      // Startup errors cannot enable Run before Unity has finished loading.
      if (!ready.current) { connectionFailed.current = true; setPhase("error"); }
    } else if (event.type === "CLOSE_REQUESTED") closePanel();
  }

  async function connect() {
    if (!host || !frame.current?.contentWindow) return;
    silenceAudio();
    adapter.current?.close();
    ready.current = false;
    setProgress(0);
    connectionFailed.current = false;
    run.current = null;
    busy.current = false;
    setPhase("connecting");
    setError("");
    const current = new SimulatorAdapter(frame.current.contentWindow, host.origin, host.session_id, handleEvent);
    adapter.current = current;
    const token = ++generation.current;
    const started = Date.now();
    // READY may arrive before or after iframe load. HELLO also supports a late listener.
    while (token === generation.current && adapter.current === current && !ready.current && !connectionFailed.current && Date.now() - started < 180000) {
      try { await current.request("HELLO", {}, null, 1000); }
      catch { /* Retry while the local Unity build is loading. */ }
    }
    if (token === generation.current && !ready.current && !connectionFailed.current) { setError("Unity โหลดไม่สำเร็จภายใน 3 นาที กรุณาปิดแล้วเปิด Simulator ใหม่"); setPhase("error"); }
  }

  async function start() {
    const current = adapter.current;
    if (!current || !ready.current || busy.current || sourceLoading || phase !== "ready" || !source.trim() || (sourceMode === "project" && !projectProgram)) return;
    busy.current = true;
    const engine = audioEngine();
    setAudioError("");
    // Unlock in the click task, before asynchronous disk/native validation.
    const audioReady = audioMuted ? Promise.resolve() : engine.unlock().catch(error => {
      if (adapter.current === current) setAudioError(String(error));
    });
    setPhase("loading"); setError("");
    let projectValidated = sourceMode !== "project";
    try {
      let programSource = source;
      let revision: string = crypto.randomUUID();
      if (sourceMode === "project") {
        // Check pending reviews before saving; never apply a proposed AI diff by saving it here.
        await invoke<ProjectSimulation>("load_project_simulation", { projectDir });
        if (beforeProjectRun && !(await beforeProjectRun())) throw new Error("บันทึกไฟล์ไม่สำเร็จ จึงยังไม่เริ่มจำลอง");
        const program = await invoke<ProjectSimulation>("load_project_simulation", { projectDir });
        programSource = program.source; revision = program.revision;
        setSource(program.source); setProjectProgram(program); setProjectError("");
        setProjectLoadedAt(new Date().toLocaleTimeString("th-TH"));
        projectValidated = true;
      }
      if (adapter.current !== current) return;
      await current.request("SET_INPUTS", { temperature: temperatureMode === "test" ? temperature : null });
      await current.request("LOAD_PROGRAM", { source: programSource, sourceRevision: revision, scene: 0, projectCompanion: sourceMode === "project" });
      if (adapter.current !== current) return;
      await audioReady;
      if (adapter.current !== current) return;
      const id = crypto.randomUUID();
      run.current = id; engine.begin(id);
      setSnapshot(revision.slice(0, 8));
      await current.request("RUN", {}, id);
    } catch (err) {
      if (adapter.current !== current) return;
      silenceAudio();
      if (!projectValidated) { setProjectError(String(err)); setProjectProgram(null); }
      else setError(String(err));
      run.current = null;
      setPhase("ready");
    } finally { if (adapter.current === current) busy.current = false; }
  }
  async function stop(reset = false) {
    silenceAudio();
    if (!adapter.current || !ready.current || busy.current) return;
    busy.current = true;
    const current = adapter.current;
    setPhase("stopping");
    try {
      await current.request(reset ? "RESET" : "STOP", {}, run.current);
      if (adapter.current !== current) return;
      run.current = null; setPhase("ready");
    } catch (err) {
      if (adapter.current !== current) return;
      current.close();
      setHost(null); // Destroy the browsing context if cancellation cannot be confirmed.
      setError(String(err)); setPhase("error");
    } finally { if (adapter.current === current) busy.current = false; }
  }
  async function exportSource() {
    if (exportBusy.current) return;
    exportBusy.current = true;
    setExporting(true); setExportResult(null);
    try {
      const path = await invoke<string | null>("export_simulator_source", { source });
      setExportResult({ message: path ? `บันทึกแล้ว: ${path}` : "ยกเลิกการส่งออกแล้ว", failed: false });
    } catch (err) {
      setExportResult({ message: String(err), failed: true });
    } finally { exportBusy.current = false; setExporting(false); }
  }
  const labels: Record<Phase, string> = { connecting: `กำลังโหลด Unity ${progress}%`, ready: "พร้อม", loading: "กำลังโหลดโปรแกรม", armed: "กำลังรอ Unity เริ่มโปรแกรม…", running: "กำลังจำลอง", stopping: "กำลังหยุด", error: "เชื่อมต่อไม่สำเร็จ" };
  const loadBlockedReason = projectDir === "." ? "เปิดโปรเจกต์ก่อนโหลดไฟล์จำลอง"
    : sourceLoading ? "กำลังอ่านและตรวจไฟล์โปรเจกต์"
    : phase === "loading" ? "กำลังเตรียมโปรแกรม กรุณารอ"
    : phase === "stopping" ? "กำลังหยุดโปรแกรม กรุณารอ"
    : ["armed", "running"].includes(phase) ? "กด Stop ก่อนโหลดโปรแกรมจากโปรเจกต์อีกครั้ง" : "";
  const canRun = phase === "ready" && !sourceLoading && !!source.trim() && (sourceMode !== "project" || !!projectProgram);
  return <div className="fixed inset-0 z-40 bg-black/70 flex items-center justify-center p-4">
    <section role="dialog" aria-modal="true" aria-label="KidBright Simulator" className="w-[96vw] h-[94vh] flex flex-col rounded-xl border border-[var(--border-color)] bg-[var(--bg-modal)] text-[var(--text-primary)] shadow-2xl overflow-hidden">
      <header className="flex items-center justify-between border-b border-[var(--border-color)] px-4 py-3 gap-3">
        <div><strong>KidBright Simulator · ทดลอง</strong><p className="text-xs text-[var(--text-secondary)]">{labels[phase]} {snapshot && `· โปรแกรมรอบ ${snapshot}`}</p></div>
        <div className="flex gap-2">
          <button className="px-3 py-1 rounded bg-emerald-700 disabled:opacity-40" disabled={!canRun} onClick={() => void start()}>Run Simulator</button>
          <button className="px-3 py-1 rounded bg-red-800 disabled:opacity-40" disabled={phase !== "running" && phase !== "armed"} onClick={() => void stop()}>Stop</button>
          <button className="px-3 py-1 rounded bg-[var(--bg-hover)] disabled:opacity-40" disabled={!ready.current || phase === "loading" || phase === "stopping"} onClick={() => void stop(true)}>Reset</button>
          <button className="px-3 py-1 rounded bg-[var(--bg-hover)]" onClick={closePanel} aria-label="Close Simulator">ปิด</button>
        </div>
      </header>
      <p className="px-4 py-2 text-xs bg-amber-500/10 text-amber-200">ใช้โค้ด JavaScript สำหรับจำลอง แยกจาก firmware C/C++ · รองรับปุ่ม แสง อุณหภูมิ Digital/Analog IN1–4 (ค่าดิบ) OUT/USB และภาพ LED 16×8 · รองรับตัวอักษร/ข้อความเลื่อนผ่านไฟล์จำลอง · รองรับเสียง buzzer ในเครื่อง · ยังไม่รองรับ Wi-Fi หรือ FreeRTOS</p>
      {error && <div role="alert" className="px-4 py-2 text-sm text-red-300 bg-red-900/30">{error}</div>}
      {projectError && <div role="alert" aria-label="Project simulation error" className="px-4 py-2 text-sm border-b" style={{ color: "var(--text-primary)", backgroundColor: "var(--bg-hover)", borderColor: "var(--border-color)" }}>
        <p>{projectError}</p>
        {pendingReviews.length > 0 && <div className="mt-2">
          <p>ยังรอตรวจรับ {pendingReviews.length} ไฟล์:</p>
          <ul className="space-y-1 mt-1">{pendingReviews.map(review => <li key={review.path} className="flex items-center gap-3">
            <span className="font-mono text-xs">{review.relative_path}</span>
            {onReviewFile && <button aria-label={"ตรวจ Diff " + review.relative_path} className="px-3 py-1 rounded border text-xs" style={{ borderColor: "var(--border-color)" }} onClick={() => onReviewFile(review.path)}>ตรวจ Diff</button>}
          </li>)}</ul>
        </div>}
      </div>}
      <div className="flex-1 min-h-0 flex">
        <aside className="w-80 shrink-0 flex flex-col border-r border-[var(--border-color)] p-3 gap-2">
          <p className="text-xs text-cyan-300">{sourceMode === "project" ? "โปรแกรมจาก simulation/main.sim.js" : "โค้ดทดลองแยกจาก firmware"}</p>
          <button aria-label="Load project simulation" title={loadBlockedReason || "อ่าน simulation/main.sim.js จากโปรเจกต์และตรวจให้ตรงกับ firmware ล่าสุด"} className="text-xs px-2 py-2 rounded bg-cyan-800 hover:bg-cyan-700 text-white disabled:bg-[var(--bg-hover)] disabled:opacity-40" disabled={!!loadBlockedReason} onClick={() => void loadProjectSource()}>{sourceLoading ? "กำลังตรวจไฟล์โปรเจกต์…" : projectProgram ? "โหลดใหม่จากโปรเจกต์" : "โหลดโปรแกรมจำลองจากโปรเจกต์"}</button>
          {loadBlockedReason && <p className="text-xs text-[var(--text-secondary)]">{loadBlockedReason}</p>}
          {sourceMode === "project" && projectProgram && <div role="status" aria-label="Project simulation status" className="text-xs text-emerald-300 break-words">
            <p>โหลดสำเร็จ: {projectProgram.path} · {projectLoadedAt}</p>
            <p className="text-[var(--text-secondary)]">โปรเจกต์: {projectDir}</p>
            <p>{phase === "running" ? "กำลังจำลองด้วยไฟล์นี้" : phase === "armed" ? "กำลังรอ Unity เริ่มโปรแกรม…" : phase === "connecting" ? "ไฟล์พร้อมแล้ว กำลังโหลดบอร์ดจำลอง…" : phase === "loading" ? "กำลังเตรียมโปรแกรมจำลอง…" : phase === "stopping" ? "กำลังหยุดโปรแกรม…" : "กด Run Simulator เพื่อเริ่มโปรแกรมบนบอร์ดจำลอง"}</p>
          </div>}
          <label className="text-xs">แหล่งอุณหภูมิ
            <select aria-label="Temperature source" className="w-full p-1 bg-[var(--bg-hover)]" value={temperatureMode} onChange={e => setTemperatureMode(e.target.value as "test" | "unity")}>
              <option value="test">ค่าทดสอบในเครื่อง</option><option value="unity">เซนเซอร์ใน Unity</option>
            </select>
          </label>
          {temperatureMode === "test" && <label className="text-xs">อุณหภูมิทดสอบ (ปัดเป็นจำนวนเต็มบน LED)
            <input aria-label="Test temperature" type="number" min={-100} max={150} step={0.5} value={temperature} className="w-full p-1 bg-[var(--bg-hover)]" onChange={e => { const value = Number(e.target.value); if (Number.isFinite(value) && value >= -100 && value <= 150) setTemperature(value); }} />
          </label>}
          <fieldset className="text-xs border border-[var(--border-color)] rounded p-2 space-y-2">
            <legend>เสียง Buzzer ในเครื่อง</legend>
            <div className="flex items-center gap-2">
              <button aria-label={audioMuted ? "Unmute simulator" : "Mute simulator"} className="px-2 py-1 rounded bg-[var(--bg-hover)]" onClick={() => {
                if (audioMuted) enableAudio();
                else { audio.current?.setMuted(true); setAudioMuted(true); }
              }}>{audioMuted ? "เปิดเสียง" : "ปิดเสียง"}</button>
              <span>{audioLevel}%</span>
            </div>
            <input aria-label="Simulator volume" className="w-full" type="range" min={0} max={100} value={audioLevel} onChange={event => {
              const value = Number(event.target.value); setAudioLevel(value); audio.current?.setLevel(value);
            }} />
            <p aria-label="Buzzer status">{buzzerState.playing ? `Buzzer: ${buzzerState.frequency} Hz${audioMuted || audioLevel === 0 || buzzerState.volume === 0 ? " · ปิดเสียง" : ""}` : "Buzzer: หยุด"}</p>
            {audioError && <div role="alert" aria-label="Simulator audio error"><p>{audioError}</p><button className="underline" onClick={enableAudio}>เปิดเสียงอีกครั้ง</button></div>}
          </fieldset>
          <label className="text-xs">ตัวอย่าง <select aria-label="Simulation example" disabled={phase === "loading"} className="w-full mt-1 p-2 rounded bg-[var(--bg-hover)]" defaultValue="" onChange={e => { const example = EXAMPLES[Number(e.target.value)]; if (example) changeSource(example.source); }}>
            <option value="" disabled>เลือกตัวอย่าง</option>{EXAMPLES.map((item, i) => <option key={i} value={i}>{item.name}</option>)}
          </select></label>
          <button className="text-xs px-2 py-2 rounded bg-[var(--bg-hover)] disabled:opacity-40" disabled={!activeName?.endsWith(".sim.js")} onClick={() => changeSource(activeSource || "")}>ใช้ไฟล์ .sim.js ที่เปิดอยู่</button>
          <textarea aria-label="Simulation source" readOnly={phase === "loading"} spellCheck={false} className="flex-1 min-h-24 resize-none bg-[var(--bg-editor)] border border-[var(--border-color)] rounded p-2 font-mono text-xs" value={source} onChange={e => changeSource(e.target.value)} />
          <button aria-label="Export simulation" className="text-xs py-2 rounded bg-[var(--bg-hover)] disabled:opacity-40" disabled={exporting || sourceLoading || !source.trim()} onClick={() => void exportSource()}>{exporting ? "กำลังส่งออก…" : "ส่งออก .sim.js"}</button>
          {exportResult && <p role={exportResult.failed ? "alert" : "status"} aria-label="Export result" className={`text-xs break-words ${exportResult.failed ? "text-red-300" : "text-[var(--text-secondary)]"}`}>{exportResult.message}</p>}
          <p className="text-xs text-[var(--text-secondary)]">การแก้โค้ดใช้รอบถัดไป การจับคู่ไฟล์ตรวจรุ่นโค้ด ยังไม่รับรองว่า JavaScript ทำงานเท่ากับ firmware บนบอร์ดจริง</p>
          {output.displayNumber != null && <p className="text-xs" aria-live="polite">ตัวเลขบน LED: {String(output.displayNumber)}</p>}
          <div className="font-mono text-xs">OUT1: {String(output.out1 ?? 0)} · OUT2: {String(output.out2 ?? 0)} · USB: {String(output.usb ?? 0)}</div>
          <div aria-live="polite" className="max-h-24 overflow-auto text-xs text-[var(--text-secondary)]">{logs.map((line, i) => <p key={i}>{line}</p>)}</div>
        </aside>
        <div className="flex-1 min-w-0 bg-[#231f20] relative">
          {host && <iframe ref={frame} title="Unity KidBright board" className="w-full h-full border-0" sandbox="allow-scripts allow-same-origin" allow="autoplay" src={`${host.url}#${new URLSearchParams({ parentOrigin: window.location.origin, sessionId: host.session_id })}`} onLoad={() => void connect()} />}
          {host && phase === "connecting" && <p role="status" className="absolute inset-0 flex items-center justify-center p-6 pointer-events-none text-sm text-white/70">กำลังโหลดบอร์ดจากไฟล์ในเครื่อง… {progress}%</p>}
          {!host && <p className="p-6">{phase === "error" ? "ปิดแล้วเปิด Simulator ใหม่เพื่อเชื่อมต่ออีกครั้ง" : "กำลังเริ่ม Simulator ภายในเครื่อง…"}</p>}
        </div>
      </div>
    </section>
  </div>;
}
