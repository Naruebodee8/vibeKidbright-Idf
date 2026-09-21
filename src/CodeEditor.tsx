import { useRef, useEffect, useState } from "react";
import Editor, { OnMount, BeforeMount } from "@monaco-editor/react";
import type { editor } from "monaco-editor";
import { DiffEditor } from "@monaco-editor/react";
import { invoke } from "@tauri-apps/api/core";

// Language detection from file extension
function getLanguageFromPath(filePath: string): string {
    const ext = filePath.split(".").pop()?.toLowerCase() || "";
    const map: Record<string, string> = {
        c:"c",h:"c",cpp:"cpp",cxx:"cpp",cc:"cpp",hpp:"cpp",
        py:"python",rs:"rust",js:"javascript",ts:"typescript",
        tsx:"typescript",jsx:"javascript",json:"json",md:"markdown",
        txt:"plaintext",cmake:"plaintext",yml:"yaml",yaml:"yaml",
        toml:"plaintext",cfg:"ini",ini:"ini",sh:"shell",bat:"bat",
        ps1:"powershell",html:"html",css:"css",xml:"xml",svg:"xml",
    };
    return map[ext] || "plaintext";
}

// ── Diagnostic type ──────────────────────────────────────────────────────────
export interface Diagnostic {
    filePath: string;
    line: number;
    column: number;
    severity: "error" | "warning" | "info";
    message: string;
    messageTh: string;
    endColumn?: number;
}

function translateError(msg: string): string {
    const l = msg.toLowerCase();
    if (l.includes("expected ';'"))         return "Missing semicolon ';'";
    if (l.includes("expected '}'"))         return "Missing closing curly bracket '}'";
    if (l.includes("expected '{'"))         return "Missing opening curly bracket '{'";
    if (l.includes("expected ')'"))         return "Missing closing parenthesis ')'";
    if (l.includes("expected '('"))         return "Missing opening parenthesis '('";
    if (l.includes("undeclared"))            return "Undeclared variable or function";
    if (l.includes("undefined reference"))  return "Undefined reference — not found";
    if (l.includes("unused variable"))      return "Variable declared but never used";
    if (l.includes("implicit declaration")) return "Missing #include for this function";
    if (l.includes("incompatible type"))    return "Type mismatch";
    if (l.includes("no such file"))         return "Header file not found";
    if (l.includes("too many arguments"))   return "Too many arguments";
    if (l.includes("too few arguments"))    return "Too few arguments";
    if (l.includes("redefinition"))         return "Already defined — redefinition";
    if (l.includes("control reaches end"))  return "Missing return statement";
    if (l.includes("format"))               return "Format string mismatch";
    return msg;
}

// ── Live Syntax Checker (C/C++ only, client-side, debounced) ────────────────
// Checks: unclosed {}, (), [], block comments, unclosed strings, missing semicolons
function checkSyntaxLive(code: string, monaco: typeof import("monaco-editor")): editor.IMarkerData[] {
    const markers: editor.IMarkerData[] = [];
    const lines = code.split("\n");
    let inBlockComment = false;
    let blockCommentOpenLine = -1;
    let blockCommentOpenCol  = -1;
    const braceStack:  { line: number; col: number }[] = []; // {}
    const parenStack:  { line: number; col: number }[] = []; // ()
    const squareStack: { line: number; col: number }[] = []; // []

    // Lines that end a "statement" and should have a semicolon
    const needsSemiRe = /^\s*(?:(?:return|break|continue|throw)\b.*|[\w\->.\[\]()"']+\s*(?:[+\-*\/&|^]=?|=|\+\+|--).*[^;{},])\s*$/;
    // Lines that are control-flow starters — do NOT require a semicolon
    const noSemiRe   = /^\s*(?:#|\/\/|$|\s*(?:if|else|for|while|do|switch|case|default|class|struct|enum|namespace|template|try|catch|\{|\}))/;

    for (let li = 0; li < lines.length; li++) {
        const raw    = lines[li];
        const lineNum = li + 1;
        let i = 0;
        let inStr: string | null = null;
        let inLineComment = false;
        let lastSignificantCol = -1; // track last non-whitespace col for semicolon check

        while (i < raw.length) {
            if (inBlockComment) {
                if (raw[i] === "*" && raw[i + 1] === "/") {
                    inBlockComment = false;
                    blockCommentOpenLine = -1;
                    i += 2;
                } else i++;
                continue;
            }
            if (inLineComment) break;
            if (inStr) {
                if (raw[i] === "\\") i += 2;
                else { if (raw[i] === inStr) inStr = null; i++; }
                continue;
            }

            const c = raw[i], n = raw[i + 1];

            if (c === "/" && n === "/") { inLineComment = true; break; }
            if (c === "/" && n === "*") {
                inBlockComment = true;
                blockCommentOpenLine = lineNum;
                blockCommentOpenCol  = i + 1;
                i += 2; continue;
            }
            if (c === '"' || c === "'") { inStr = c; i++; continue; }

            // Track brackets
            if (c === "{") {
                braceStack.push({ line: lineNum, col: i + 1 });
            } else if (c === "}") {
                if (braceStack.length > 0) braceStack.pop();
                else markers.push({
                    severity: monaco.MarkerSeverity.Error,
                    startLineNumber: lineNum, startColumn: i + 1,
                    endLineNumber:   lineNum, endColumn:   i + 2,
                    message: "Unexpected '}' — no matching '{'",
                    source: "Live Syntax",
                });
            } else if (c === "(") {
                parenStack.push({ line: lineNum, col: i + 1 });
            } else if (c === ")") {
                if (parenStack.length > 0) parenStack.pop();
                else markers.push({
                    severity: monaco.MarkerSeverity.Error,
                    startLineNumber: lineNum, startColumn: i + 1,
                    endLineNumber:   lineNum, endColumn:   i + 2,
                    message: "Unexpected ')' — no matching '('",
                    source: "Live Syntax",
                });
            } else if (c === "[") {
                squareStack.push({ line: lineNum, col: i + 1 });
            } else if (c === "]") {
                if (squareStack.length > 0) squareStack.pop();
                else markers.push({
                    severity: monaco.MarkerSeverity.Error,
                    startLineNumber: lineNum, startColumn: i + 1,
                    endLineNumber:   lineNum, endColumn:   i + 2,
                    message: "Unexpected ']' — no matching '['",
                    source: "Live Syntax",
                });
            }

            if (c.trim() !== "") lastSignificantCol = i;
            i++;
        }

        // Unclosed string literal on this line
        if (inStr === '"' && !inLineComment && !inBlockComment) {
            const col = raw.lastIndexOf('"');
            markers.push({
                severity: monaco.MarkerSeverity.Error,
                startLineNumber: lineNum, startColumn: col + 1,
                endLineNumber:   lineNum, endColumn:   raw.length + 1,
                message: 'Unclosed string literal — missing closing "',
                source: "Live Syntax",
            });
        }

        // Missing semicolon heuristic:
        // If the line ends with a word/close-bracket and is not a control-flow line,
        // and it's not inside a block comment, flag it as a warning.
        if (
            !inBlockComment && !inLineComment && lastSignificantCol >= 0 &&
            !noSemiRe.test(raw)
        ) {
            const trimmed = raw.trimEnd();
            const lastCh  = trimmed[trimmed.length - 1];
            // Heuristic: ends with identifier char or ) but not ; { } , : \ //
            if (
                lastCh && /[\w)"']/.test(lastCh) &&
                needsSemiRe.test(raw)
            ) {
                markers.push({
                    severity: monaco.MarkerSeverity.Warning,
                    startLineNumber: lineNum, startColumn: trimmed.length,
                    endLineNumber:   lineNum, endColumn:   trimmed.length + 1,
                    message: "Possible missing semicolon ';'",
                    source: "Live Syntax",
                });
            }
        }
    }

    // Unclosed block comment
    if (inBlockComment && blockCommentOpenLine >= 0) {
        markers.push({
            severity: monaco.MarkerSeverity.Error,
            startLineNumber: blockCommentOpenLine, startColumn: blockCommentOpenCol,
            endLineNumber:   blockCommentOpenLine, endColumn:   blockCommentOpenCol + 2,
            message: "Unclosed block comment '/*' — missing closing '*/'",
            source: "Live Syntax",
        });
    }
    // Unclosed curly brace
    if (braceStack.length > 0) {
        const last = braceStack[braceStack.length - 1];
        markers.push({
            severity: monaco.MarkerSeverity.Warning,
            startLineNumber: last.line, startColumn: last.col,
            endLineNumber:   last.line, endColumn:   last.col + 1,
            message: `Unclosed '{' — missing closing '}'`,
            source: "Live Syntax",
        });
    }
    // Unclosed parenthesis
    if (parenStack.length > 0) {
        const last = parenStack[parenStack.length - 1];
        markers.push({
            severity: monaco.MarkerSeverity.Warning,
            startLineNumber: last.line, startColumn: last.col,
            endLineNumber:   last.line, endColumn:   last.col + 1,
            message: `Unclosed '(' — missing closing ')'`,
            source: "Live Syntax",
        });
    }
    // Unclosed square bracket
    if (squareStack.length > 0) {
        const last = squareStack[squareStack.length - 1];
        markers.push({
            severity: monaco.MarkerSeverity.Warning,
            startLineNumber: last.line, startColumn: last.col,
            endLineNumber:   last.line, endColumn:   last.col + 1,
            message: `Unclosed '[' — missing closing ']'`,
            source: "Live Syntax",
        });
    }
    return markers;
}

export function parseBuildDiagnostics(rawErrors: string[]): Diagnostic[] {
    const result: Diagnostic[] = [];
    const re = /^(.+?):(\d+)(?::(\d+))?:\s*(error|warning|note|info):\s*(.+)$/;
    for (const line of rawErrors) {
        const m = line.match(re);
        if (m) {
            const sev = m[4] === "warning" ? "warning" : m[4] === "note" ? "info" : "error";
            result.push({
                filePath: m[1].replace(/\\/g, "/"),
                line: parseInt(m[2], 10),
                column: m[3] ? parseInt(m[3], 10) : 1,
                severity: sev,
                message: m[5],
                messageTh: translateError(m[5]),
            });
        }
    }
    return result;
}

// ── Vibe Dark theme ──────────────────────────────────────────────────────────
const defineVibeDarkTheme: BeforeMount = (monaco) => {
    monaco.editor.defineTheme("vibe-dark", {
        base: "vs-dark", inherit: true,
        rules: [
            { token: "comment",              foreground: "5c6370", fontStyle: "italic" },
            { token: "comment.block",        foreground: "5c6370", fontStyle: "italic" },
            { token: "keyword",              foreground: "c678dd" },
            { token: "keyword.control",      foreground: "c678dd" },
            { token: "keyword.operator",     foreground: "56b6c2" },
            { token: "type",                 foreground: "e5c07b" },
            { token: "type.identifier",      foreground: "e5c07b" },
            { token: "storage.type",         foreground: "c678dd" },
            { token: "entity.name.function", foreground: "61afef" },
            { token: "support.function",     foreground: "61afef" },
            { token: "string",               foreground: "98c379" },
            { token: "string.escape",        foreground: "56b6c2" },
            { token: "number",               foreground: "d19a66" },
            { token: "constant.numeric",     foreground: "d19a66" },
            { token: "keyword.directive",    foreground: "e06c75" },
            { token: "keyword.other",        foreground: "e06c75" },
            { token: "meta.preprocessor",    foreground: "e06c75" },
            { token: "variable",             foreground: "e06c75" },
            { token: "variable.predefined",  foreground: "e5c07b" },
            { token: "identifier",           foreground: "abb2bf" },
            { token: "delimiter",            foreground: "abb2bf" },
            { token: "delimiter.bracket",    foreground: "abb2bf" },
            { token: "operator",             foreground: "56b6c2" },
            { token: "constant",             foreground: "d19a66" },
            { token: "constant.language",    foreground: "d19a66" },
        ],
        colors: {
            "editor.background":                      "#020617",
            "editor.foreground":                      "#abb2bf",
            "editor.selectionBackground":             "#3e4451",
            "editor.selectionHighlightBackground":    "#3e445180",
            "editor.inactiveSelectionBackground":     "#3e445160",
            "editor.lineHighlightBackground":         "#0f172a",
            "editor.lineHighlightBorder":             "#1e293b",
            "editorLineNumber.foreground":            "#3b4252",
            "editorLineNumber.activeForeground":      "#7c8598",
            "editorGutter.background":                "#020617",
            "editorIndentGuide.background":           "#1e293b",
            "editorIndentGuide.activeBackground":     "#334155",
            "editorCursor.foreground":                "#528bff",
            "editorBracketMatch.background":          "#3e445180",
            "editorBracketMatch.border":              "#528bff60",
            "minimap.background":                     "#020617",
            "minimapSlider.background":               "#1e293b40",
            "minimapSlider.hoverBackground":          "#33415560",
            "minimapSlider.activeBackground":         "#47556980",
            "scrollbar.shadow":                       "#00000000",
            "scrollbarSlider.background":             "#1e293b80",
            "scrollbarSlider.hoverBackground":        "#334155a0",
            "scrollbarSlider.activeBackground":       "#475569c0",
            "editorWidget.background":                "#0f172a",
            "editorWidget.border":                    "#1e293b",
            "editorHoverWidget.background":           "#0f172a",
            "editorHoverWidget.border":               "#1e293b",
            "editorSuggestWidget.background":         "#0f172a",
            "editorSuggestWidget.border":             "#1e293b",
            "editorSuggestWidget.selectedBackground": "#1e293b",
            "editor.findMatchBackground":             "#d19a6640",
            "editor.findMatchHighlightBackground":    "#d19a6620",
            "editor.wordHighlightBackground":         "#61afef20",
            "editor.wordHighlightStrongBackground":   "#61afef30",
            "editor.overviewRulerBorder":             "#1e293b",
        },
    });
};

// ── Problems Panel ────────────────────────────────────────────────────────────
interface ProblemsPanelProps {
    diagnostics: Diagnostic[];
    activeFilePath: string;
    onClickDiagnostic: (d: Diagnostic) => void;
    onSendToAi?: (text: string) => void;
}

function ProblemsPanel({ diagnostics, activeFilePath, onClickDiagnostic, onSendToAi }: ProblemsPanelProps) {
    const [copied, setCopied] = useState(false);
    const normActive = activeFilePath.replace(/\\/g, "/").toLowerCase();
    const fileDiags = diagnostics.filter(d => {
        const nd = d.filePath.toLowerCase();
        return nd === normActive || normActive.endsWith(nd) || nd.endsWith(normActive);
    });

    const formatForAi = () => {
        const fname = activeFilePath.split(/[\\/]/).pop() || activeFilePath;
        const lines = fileDiags.map(d => {
            const icon = d.severity === "error" ? "❌" : d.severity === "warning" ? "⚠️" : "ℹ️";
            const loc  = `Line ${d.line}${d.column > 1 ? `:${d.column}` : ""}`;
            return `${icon} [${loc}] ${d.message}`;
        }).join("\n");
        return `The following build errors occurred in \`${fname}\`:\n\n${lines}\n\nPlease analyze and fix the code.`;
    };

    const handleCopy = () => {
        navigator.clipboard.writeText(formatForAi()).then(() => {
            setCopied(true);
            setTimeout(() => setCopied(false), 2000);
        });
    };

    const handleSendToAi = () => {
        if (onSendToAi) onSendToAi(formatForAi());
    };

    if (fileDiags.length === 0) {
        return (
            <div className="flex items-center gap-2 px-4 py-2 text-xs text-slate-500">
                <span className="text-emerald-400">✓</span>
                <span>No errors or warnings in this file</span>
            </div>
        );
    }
    return (
        <div className="flex flex-col">
            {/* Action bar */}
            <div className="flex items-center gap-1.5 px-3 py-1 border-b border-slate-800/60 bg-slate-900/40">
                <span className="text-[10px] text-slate-500 mr-auto">{fileDiags.length} problem{fileDiags.length > 1 ? "s" : ""} in this file</span>
                <button onClick={handleCopy}
                    className="flex items-center gap-1 px-2 py-0.5 text-[10px] rounded bg-slate-700/50 hover:bg-slate-700 text-slate-300 transition-colors">
                    {copied ? "✓ Copied" : "📋 Copy"}
                </button>
                {onSendToAi && (
                    <button onClick={handleSendToAi}
                        className="flex items-center gap-1 px-2 py-0.5 text-[10px] rounded bg-violet-600/30 hover:bg-violet-600/60 text-violet-300 border border-violet-500/30 transition-colors">
                        🤖 Fix with AI
                    </button>
                )}
            </div>
            {/* Diagnostic list */}
            <div className="flex flex-col max-h-36 overflow-y-auto">
                {fileDiags.map((d, i) => (
                    <button key={i} onClick={() => onClickDiagnostic(d)}
                        className="flex items-start gap-2 px-3 py-1.5 text-left hover:bg-slate-800/60 transition-colors group">
                        {d.severity === "error" ? (
                            <svg className="w-3.5 h-3.5 mt-0.5 text-red-400 shrink-0" fill="none" viewBox="0 0 16 16">
                                <circle cx="8" cy="8" r="6.5" stroke="currentColor" strokeWidth="1.5"/>
                                <path d="M5.5 5.5l5 5M10.5 5.5l-5 5" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round"/>
                            </svg>
                        ) : d.severity === "warning" ? (
                            <svg className="w-3.5 h-3.5 mt-0.5 text-yellow-400 shrink-0" fill="none" viewBox="0 0 16 16">
                                <path d="M8 2.5L1.5 13.5h13L8 2.5z" stroke="currentColor" strokeWidth="1.5" strokeLinejoin="round"/>
                                <path d="M8 7v3.5M8 12h.01" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round"/>
                            </svg>
                        ) : (
                            <svg className="w-3.5 h-3.5 mt-0.5 text-blue-400 shrink-0" fill="none" viewBox="0 0 16 16">
                                <circle cx="8" cy="8" r="6.5" stroke="currentColor" strokeWidth="1.5"/>
                                <path d="M8 7v5M8 5h.01" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round"/>
                            </svg>
                        )}
                        <div className="flex-1 min-w-0">
                            <span className={`text-xs font-medium ${
                                d.severity === "error"   ? "text-red-300" :
                                d.severity === "warning" ? "text-yellow-300" : "text-blue-300"
                            }`}>{d.messageTh !== d.message ? d.messageTh : d.message}</span>
                            {d.messageTh !== d.message && (
                                <span className="ml-1.5 text-[10px] text-slate-500 italic">{d.message}</span>
                            )}
                        </div>
                        <span className="text-[10px] text-slate-500 shrink-0 font-mono tabular-nums">
                            :{d.line}:{d.column}
                        </span>
                    </button>
                ))}
            </div>
        </div>
    );
}

// ── CodeEditorProps ───────────────────────────────────────────────────────────
interface CodeEditorProps {
    value: string;
    onChange: (value: string) => void;
    filePath: string;
    onSave?: () => void;
    activeTheme?: string;
    diagnostics?: Diagnostic[];
    showProblems?: boolean;
    onSendToAi?: (text: string) => void;
}

// Helper: compare two file paths by checking if their last 2 segments match,
// or if one path ends with the other (handles absolute vs relative mismatch).
function pathsMatch(a: string, b: string): boolean {
    const norm = (p: string) => p.replace(/\\/g, "/").toLowerCase().replace(/\/+/g, "/");
    const tail = (p: string) => norm(p).split("/").slice(-2).join("/");
    const na = norm(a), nb = norm(b);
    return na === nb || na.endsWith("/" + nb) || nb.endsWith("/" + na) || tail(na) === tail(nb);
}

export default function CodeEditor({
    value, onChange, filePath, onSave,
    activeTheme = "navy",
    diagnostics = [],
    showProblems = true,
    onSendToAi,
}: CodeEditorProps) {
    const editorRef  = useRef<editor.IStandaloneCodeEditor | null>(null);
    const monacoRef  = useRef<typeof import("monaco-editor") | null>(null);
    const language   = getLanguageFromPath(filePath);
    const [pendingContent,  setPendingContent]  = useState<string | null>(null);
    const [problemsOpen,    setProblemsOpen]    = useState(true);
    // Live syntax diagnostics — mirrors what setModelMarkers("live-syntax") sets,
    // so the PROBLEMS panel can show them without waiting for a build.
    const [liveMarkerDiags, setLiveMarkerDiags] = useState<Diagnostic[]>([]);
    const onSaveRef = useRef(onSave);
    useEffect(() => { onSaveRef.current = onSave; }, [onSave]);

    // ── Real-time live syntax check (C/C++ only, debounced 400ms) ────────────
    // Race-condition-safe:
    //   - checkIdRef: incremented before each setTimeout; the callback checks
    //     it hasn't gone stale before writing any state/markers.
    //   - valueRef/filePathRef: always hold the LATEST values so the closure
    //     inside setTimeout never reads a stale snapshot.
    //   - try-catch around checkSyntaxLive so parser crashes surface as an
    //     error marker instead of silently discarding all results.
    const checkIdRef   = useRef(0);
    const valueRef     = useRef(value);
    const filePathRef  = useRef(filePath);
    // Keep refs in sync on every render (no extra effect needed)
    valueRef.current    = value;
    filePathRef.current = filePath;

    useEffect(() => {
        const ext = (filePath.split(".").pop() || "").toLowerCase();
        const isCpp = ["c","cpp","cc","cxx","h","hpp"].includes(ext);

        if (!isCpp) {
            // Immediately clear live markers when switching away from a C/C++ file
            setLiveMarkerDiags([]);
            const ed     = editorRef.current;
            const monaco = monacoRef.current;
            if (ed && monaco) {
                const model = ed.getModel();
                if (model) monaco.editor.setModelMarkers(model, "live-syntax", []);
            }
            return;
        }

        // Stamp this check round so a stale timer can self-discard
        const myId = ++checkIdRef.current;

        const timer = setTimeout(() => {
            // Discard if a newer check has been scheduled since this timer started
            if (myId !== checkIdRef.current) return;

            const ed     = editorRef.current;
            const monaco = monacoRef.current;
            if (!ed || !monaco) return;
            const model = ed.getModel();
            if (!model) return;

            // Read from refs (not closure) to always get the freshest value
            const currentValue    = valueRef.current;
            const currentFilePath = filePathRef.current;

            let liveMarkers: editor.IMarkerData[];
            try {
                liveMarkers = checkSyntaxLive(currentValue, monaco as any);
            } catch (err) {
                // Parser threw unexpectedly — surface as a single error marker
                // instead of silently swallowing it (which would look like "no errors")
                console.warn("[Live Syntax] checkSyntaxLive threw:", err);
                liveMarkers = [{
                    severity: (monaco as any).MarkerSeverity.Error,
                    startLineNumber: 1, startColumn: 1,
                    endLineNumber:   1, endColumn:   2,
                    message: `Live syntax checker crashed: ${String(err)}`,
                    source: "Live Syntax",
                }];
            }

            // Discard again after the (synchronous) parse in case a render snuck in
            if (myId !== checkIdRef.current) return;

            // 1. Push squiggles into Monaco
            monaco.editor.setModelMarkers(model, "live-syntax", liveMarkers);

            // 2. Mirror as Diagnostic[] for the PROBLEMS panel
            const liveDiags: Diagnostic[] = liveMarkers.map(m => ({
                filePath:  currentFilePath,
                line:      m.startLineNumber,
                column:    m.startColumn,
                endColumn: m.endColumn,
                severity:  m.severity === (monaco as any).MarkerSeverity.Error   ? "error"   :
                           m.severity === (monaco as any).MarkerSeverity.Warning ? "warning" : "info",
                message:   m.message,
                messageTh: m.message,
            }));
            setLiveMarkerDiags(liveDiags);
        }, 400);

        return () => clearTimeout(timer);
    }, [value, filePath]);

    // Apply build diagnostics as Monaco markers whenever diagnostics or filePath change
    useEffect(() => {
        const ed     = editorRef.current;
        const monaco = monacoRef.current;
        if (!ed || !monaco) return;
        const model = ed.getModel();
        if (!model) return;
        const fileMarkers = diagnostics.filter(d => pathsMatch(d.filePath, filePath));
        const markers: editor.IMarkerData[] = fileMarkers.map(d => ({
            severity: d.severity === "error"   ? monaco.MarkerSeverity.Error   :
                      d.severity === "warning" ? monaco.MarkerSeverity.Warning  :
                                                 monaco.MarkerSeverity.Info,
            startLineNumber: d.line,
            startColumn:     d.column,
            endLineNumber:   d.line,
            endColumn:       (d.endColumn ?? d.column + 1),
            message: d.messageTh !== d.message ? `${d.messageTh}\n\n${d.message}` : d.message,
            source: "ESP-IDF Build",
        }));
        monaco.editor.setModelMarkers(model, "esp-idf-build", markers);
    }, [diagnostics, filePath]);

    // Listen for AI diffs
    useEffect(() => {
        if (!filePath) { setPendingContent(null); return; }
        let isMounted = true;
        const checkDiff = () => {
            invoke<string | null>("check_pending_diff", { path: filePath })
                .then(res => { if (isMounted) setPendingContent(res); })
                .catch(() => { if (isMounted) setPendingContent(null); });
        };
        checkDiff();
        let unlisten: (() => void) | null = null;
        import("@tauri-apps/api/event").then(({ listen }) => {
            if (!isMounted) return;
            listen("ai-diff-pending", (event) => {
                if (!isMounted) return;
                try {
                    const data = typeof event.payload === "string" ? JSON.parse(event.payload) : event.payload as any;
                    const evPath  = String(data.fullPath || "");
                    const curPath = String(filePath || "");
                    if (evPath && curPath) {
                        const norm = (p: string) => p.replace(/\\/g, "/").toLowerCase().replace(/\/+/g, "/");
                        const tail = (p: string) => p.split("/").slice(-2).join("/");
                        const ne = norm(evPath), nc = norm(curPath);
                        if (ne === nc || nc.endsWith(ne) || ne.endsWith(nc) || tail(ne) === tail(nc)) checkDiff();
                    }
                } catch {}
            }).then(fn => { if (!isMounted) fn(); else unlisten = fn; });
        });
        return () => { isMounted = false; if (unlisten) unlisten(); };
    }, [filePath]);

    const handleAcceptDiff  = async () => { try { await invoke("accept_diff",  { path: filePath }); setPendingContent(null); } catch {} };
    const handleAcceptAll   = async () => { try { await invoke("accept_all_diffs"); setPendingContent(null); } catch {} };
    const handleRejectDiff  = async () => { try { await invoke("reject_diff",  { path: filePath }); setPendingContent(null); } catch {} };

    const handleEditorMount: OnMount = (ed, monaco) => {
        editorRef.current  = ed;
        monacoRef.current  = monaco as any;
        ed.addAction({
            id: "vibe-save-file", label: "Save File",
            keybindings: [monaco.KeyMod.CtrlCmd | monaco.KeyCode.KeyS],
            run: () => onSaveRef.current?.(),
        });

        const model = ed.getModel();
        if (model) {
            // Apply build diagnostics on mount using shared pathsMatch helper
            const fileMarkers = diagnostics.filter(d => pathsMatch(d.filePath, filePath));
            if (fileMarkers.length > 0) {
                monaco.editor.setModelMarkers(model, "esp-idf-build", fileMarkers.map(d => ({
                    severity: d.severity === "error"   ? monaco.MarkerSeverity.Error   :
                              d.severity === "warning" ? monaco.MarkerSeverity.Warning : monaco.MarkerSeverity.Info,
                    startLineNumber: d.line, startColumn: d.column,
                    endLineNumber:   d.line, endColumn: (d.endColumn ?? d.column + 1),
                    message: d.messageTh !== d.message ? `${d.messageTh}\n\n${d.message}` : d.message,
                    source: "ESP-IDF Build",
                })));
            }

            // Run live syntax check immediately on mount for C/C++ files
            // and populate liveMarkerDiags so PROBLEMS panel shows errors right away
            const ext = (filePath.split(".").pop() || "").toLowerCase();
            if (["c","cpp","cc","cxx","h","hpp"].includes(ext)) {
                const liveMarkers = checkSyntaxLive(value, monaco as any);
                monaco.editor.setModelMarkers(model, "live-syntax", liveMarkers);
                const liveDiags: Diagnostic[] = liveMarkers.map(m => ({
                    filePath:  filePath,
                    line:      m.startLineNumber,
                    column:    m.startColumn,
                    endColumn: m.endColumn,
                    severity:  m.severity === monaco.MarkerSeverity.Error   ? "error"   :
                               m.severity === monaco.MarkerSeverity.Warning ? "warning" : "info",
                    message:   m.message,
                    messageTh: m.message,
                }));
                setLiveMarkerDiags(liveDiags);
            }
        }
        ed.focus();
    };

    const handleClickDiagnostic = (d: Diagnostic) => {
        const ed = editorRef.current;
        if (!ed) return;
        ed.revealLineInCenter(d.line);
        ed.setPosition({ lineNumber: d.line, column: d.column });
        ed.focus();
    };

    // Merge build diagnostics + live syntax diagnostics for PROBLEMS panel
    const buildDiags  = diagnostics.filter(d => pathsMatch(d.filePath, filePath));
    const allDiags    = [
        ...buildDiags,
        // Only show live diags that don't duplicate a build diag on the same line
        ...liveMarkerDiags.filter(ld =>
            !buildDiags.some(bd => bd.line === ld.line && bd.severity === ld.severity)
        ),
    ];
    const errorCount   = allDiags.filter(d => d.severity === "error").length;
    const warningCount = allDiags.filter(d => d.severity === "warning").length;

    const editorOptions = {
        fontFamily: "'JetBrains Mono','Fira Code','Cascadia Code',Consolas,monospace",
        fontSize: 13, fontWeight: "400" as const, fontLigatures: true,
        lineHeight: 22, letterSpacing: 0.3,
        minimap: { enabled: true, maxColumn: 80, renderCharacters: false, scale: 1 },
        smoothScrolling: true, scrollBeyondLastLine: false,
        wordWrap: "off" as const, autoIndent: "full" as const, formatOnPaste: true,
        tabSize: 4, insertSpaces: true,
        bracketPairColorization: { enabled: true },
        autoClosingBrackets: "always" as const, autoClosingQuotes: "always" as const,
        matchBrackets: "always" as const,
        cursorBlinking: "smooth" as const, cursorSmoothCaretAnimation: "on" as const,
        cursorStyle: "line" as const, cursorWidth: 2,
        renderWhitespace: "selection" as const, renderLineHighlight: "all" as const,
        guides: { indentation: true, bracketPairs: true },
        padding: { top: 12, bottom: 12 },
        scrollbar: { verticalScrollbarSize: 10, horizontalScrollbarSize: 10, useShadows: false },
        quickSuggestions: false, suggestOnTriggerCharacters: false,
        parameterHints: { enabled: false },
        hover: { enabled: true, delay: 300 },
        glyphMargin: true,
    };

    return (
        <div className="relative w-full h-full flex flex-col border-t border-[var(--border-subtle)]">
            {/* AI diff toolbar */}
            {pendingContent !== null && (
                <div className="absolute top-4 right-8 z-10 flex gap-2 p-2 bg-[var(--bg-elevated)] border border-[var(--border-normal)] rounded-lg shadow-xl backdrop-blur-sm shadow-black/50">
                    <div className="px-3 py-1 bg-violet-600/20 text-violet-400 text-xs font-bold rounded flex items-center mr-2">Review AI Changes</div>
                    <button onClick={handleRejectDiff}  className="px-3 py-1.5 bg-rose-500/10 hover:bg-rose-500/20 text-rose-400 border border-rose-500/30 rounded-md text-xs font-medium flex items-center gap-1.5 transition-colors"><span>❌</span> Undo</button>
                    <button onClick={handleAcceptDiff}  className="px-3 py-1.5 bg-emerald-500/10 hover:bg-emerald-500/30 text-emerald-400 border border-emerald-500/30 rounded-md text-xs font-medium flex items-center gap-1.5 transition-colors"><span>✅</span> Accept</button>
                    <button onClick={handleAcceptAll}   className="px-3 py-1.5 bg-emerald-600 hover:bg-emerald-500 text-white rounded-md text-xs font-medium flex items-center gap-1.5 transition-colors"><span>✨</span> Accept All</button>
                </div>
            )}

            {/* Editor */}
            <div className="flex-1 min-h-0">
                {pendingContent !== null ? (
                    <DiffEditor height="100%" language={language} original={value} modified={pendingContent}
                        theme={activeTheme === "light" ? "light" : "vibe-dark"} beforeMount={defineVibeDarkTheme}
                        options={{ ...editorOptions, readOnly: false, originalEditable: false, renderSideBySide: true, diffWordWrap: "off" }}/>
                ) : (
                    <Editor height="100%" language={language} value={value}
                        theme={activeTheme === "light" ? "light" : "vibe-dark"} beforeMount={defineVibeDarkTheme}
                        onMount={handleEditorMount} onChange={(val) => onChange(val ?? "")}
                        options={{ ...editorOptions, lineNumbers: "on", folding: true, foldingHighlight: true,
                            lineDecorationsWidth: 8, lineNumbersMinChars: 4, overviewRulerLanes: 3 }}/>
                )}
            </div>

            {/* Problems panel */}
            {showProblems && (
                <div className={`border-t border-slate-800 bg-[#060d1a] shrink-0`}>
                    <button onClick={() => setProblemsOpen(p => !p)}
                        className="w-full flex items-center gap-2 px-3 py-1 text-xs text-slate-400 hover:text-slate-200 hover:bg-slate-800/40 transition-colors select-none">
                        <svg className={`w-3 h-3 transition-transform duration-200 ${problemsOpen ? "" : "-rotate-90"}`}
                            fill="none" viewBox="0 0 12 12">
                            <path d="M2 4l4 4 4-4" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round"/>
                        </svg>
                        <span className="font-semibold uppercase tracking-wider text-[10px]">Problems</span>
                        {errorCount > 0 && (
                            <span className="flex items-center gap-1 text-red-400 font-medium">
                                <svg className="w-3 h-3" fill="none" viewBox="0 0 12 12">
                                    <circle cx="6" cy="6" r="5" stroke="currentColor" strokeWidth="1.5"/>
                                    <path d="M4 4l4 4M8 4L4 8" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round"/>
                                </svg>
                                {errorCount}
                            </span>
                        )}
                        {warningCount > 0 && (
                            <span className="flex items-center gap-1 text-yellow-400 font-medium">
                                <svg className="w-3 h-3" fill="none" viewBox="0 0 12 12">
                                    <path d="M6 1.5L1 10.5h10L6 1.5z" stroke="currentColor" strokeWidth="1.5" strokeLinejoin="round"/>
                                    <path d="M6 5v2.5M6 9h.01" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round"/>
                                </svg>
                                {warningCount}
                            </span>
                        )}
                        {errorCount === 0 && warningCount === 0 && (
                            <span className="text-emerald-500 text-[10px]">✓ No problems</span>
                        )}
                    </button>
                    {problemsOpen && (
                        <ProblemsPanel diagnostics={allDiags} activeFilePath={filePath}
                            onClickDiagnostic={handleClickDiagnostic}
                            onSendToAi={onSendToAi}/>
                    )}
                </div>
            )}
        </div>
    );
}

