export interface BuzzerState { playing: boolean; frequency: number; volume: number }

export function isBuzzerState(value: unknown): value is BuzzerState {
  if (!value || typeof value !== "object") return false;
  const state = value as BuzzerState;
  return typeof state.playing === "boolean" && typeof state.volume === "number" &&
    Number.isFinite(state.volume) && state.volume >= 0 && state.volume <= 100 &&
    typeof state.frequency === "number" && Number.isFinite(state.frequency) &&
    (state.playing ? state.frequency >= 20 && state.frequency <= 20000 : state.frequency === 0);
}

function browserContext(): AudioContext {
  const Context = window.AudioContext || (window as unknown as { webkitAudioContext?: typeof AudioContext }).webkitAudioContext;
  if (!Context) throw new Error("ระบบนี้ไม่รองรับ Web Audio จึงเล่นเสียง buzzer ไม่ได้");
  return new Context();
}

/** Local square-wave renderer. It has no access to firmware, native IPC or Unity. */
export class BuzzerAudio {
  private context: AudioContext | null = null;
  private gain: GainNode | null = null;
  private oscillator: OscillatorNode | null = null;
  private runId: string | null = null;
  private tone: BuzzerState = { playing: false, frequency: 0, volume: 50 };
  private level = 50;
  private muted = false;
  private disposed = false;

  constructor(private createContext: () => AudioContext = browserContext) {}

  /** Call synchronously from Run/unmute, before waiting for native or iframe IPC. */
  async unlock(): Promise<void> {
    if (this.disposed) return;
    if (!this.context) {
      this.context = this.createContext();
      this.gain = this.context.createGain();
      this.gain.gain.setValueAtTime(0, this.context.currentTime);
      this.gain.connect(this.context.destination);
    }
    const context = this.context;
    let timeout: ReturnType<typeof setTimeout> | undefined;
    try {
      if (context.state !== "running") {
        await Promise.race([
          context.resume(),
          new Promise<never>((_, reject) => { timeout = setTimeout(() => reject(new Error("เบราว์เซอร์ยังไม่เปิดเสียง กรุณากดปุ่มเปิดเสียงแล้วลองใหม่")), 2000); }),
        ]);
      }
      if (this.disposed || this.context !== context) return;
      if (context.state !== "running") throw new Error("ยังเปิดเสียงไม่ได้ กรุณากดปุ่มเปิดเสียงอีกครั้ง");
      this.render();
    } finally { if (timeout !== undefined) clearTimeout(timeout); }
  }

  begin(runId: string) {
    this.stop();
    if (!this.disposed) this.runId = runId;
  }

  update(runId: string, state: unknown): boolean {
    if (this.disposed || !this.runId || runId !== this.runId) return false;
    if (!isBuzzerState(state)) throw new Error("ข้อมูล buzzer ไม่ถูกต้อง");
    this.tone = { ...state };
    this.render();
    return true;
  }

  setLevel(level: number) {
    if (!Number.isFinite(level) || level < 0 || level > 100) throw new Error("ระดับเสียงต้องอยู่ในช่วง 0–100");
    this.level = level;
    this.render();
  }

  setMuted(muted: boolean) { this.muted = muted; this.render(); }

  private silence() {
    const node = this.oscillator;
    this.oscillator = null;
    if (node) { try { node.stop(); } finally { node.disconnect(); } }
    if (this.context && this.gain) this.gain.gain.setValueAtTime(0, this.context.currentTime);
  }

  private render() {
    const context = this.context;
    if (!this.runId || !this.tone.playing || this.muted || this.level === 0 || this.tone.volume === 0 ||
        !context || context.state !== "running" || !this.gain) { this.silence(); return; }
    if (!this.oscillator) {
      this.oscillator = context.createOscillator();
      this.oscillator.type = "square";
      this.oscillator.connect(this.gain);
      this.oscillator.frequency.setValueAtTime(this.tone.frequency, context.currentTime);
      this.oscillator.start();
    } else this.oscillator.frequency.setValueAtTime(this.tone.frequency, context.currentTime);
    this.gain.gain.setValueAtTime(0.2 * this.level / 100 * this.tone.volume / 100, context.currentTime);
  }

  stop() {
    this.runId = null;
    this.tone = { playing: false, frequency: 0, volume: 50 };
    this.silence();
  }

  dispose() {
    if (this.disposed) return;
    this.stop(); this.disposed = true;
    const context = this.context;
    this.gain?.disconnect(); this.gain = null; this.context = null;
    if (context && context.state !== "closed") void context.close().catch(() => {});
  }
}
