import { afterEach, describe, expect, it, vi } from "vitest";
import { BuzzerAudio, isBuzzerState } from "../simulator/BuzzerAudio";

function fixture() {
  const oscillators: { type: string; frequency: { setValueAtTime: ReturnType<typeof vi.fn> }; connect: ReturnType<typeof vi.fn>; disconnect: ReturnType<typeof vi.fn>; start: ReturnType<typeof vi.fn>; stop: ReturnType<typeof vi.fn> }[] = [];
  const gain = { gain: { setValueAtTime: vi.fn() }, connect: vi.fn(), disconnect: vi.fn() };
  const context = {
    state: "running", currentTime: 0, destination: {}, createGain: vi.fn(() => gain),
    createOscillator: vi.fn(() => { const node = { type: "sine", frequency: { setValueAtTime: vi.fn() }, connect: vi.fn(), disconnect: vi.fn(), start: vi.fn(), stop: vi.fn() }; oscillators.push(node); return node; }),
    resume: vi.fn(async () => { context.state = "running"; }), close: vi.fn(async () => { context.state = "closed"; }),
  };
  const factory = vi.fn(() => context as unknown as AudioContext);
  return { engine: new BuzzerAudio(factory), factory, context, gain, oscillators };
}
const on = { playing: true, frequency: 1000, volume: 50 };
afterEach(() => { vi.useRealTimers(); vi.unstubAllGlobals(); });

describe("local buzzer Web Audio ownership", () => {
  it("creates no context until Run unlocks it, then produces one square oscillator", async () => {
    const f = fixture(); f.engine.begin("run"); f.engine.update("run", on);
    expect(f.factory).not.toHaveBeenCalled();
    await f.engine.unlock();
    expect(f.oscillators).toHaveLength(1); expect(f.oscillators[0].type).toBe("square");
    expect(f.oscillators[0].frequency.setValueAtTime).toHaveBeenCalledWith(1000, 0);
    f.engine.update("run", { ...on, frequency: 440 });
    expect(f.oscillators).toHaveLength(1);
    expect(f.oscillators[0].frequency.setValueAtTime).toHaveBeenLastCalledWith(440, 0);
    f.engine.dispose();
  });
  it("mutes immediately, respects both volume controls and resumes only the current tone", async () => {
    const f = fixture(); await f.engine.unlock(); f.engine.begin("run"); f.engine.update("run", on);
    expect(f.gain.gain.setValueAtTime).toHaveBeenLastCalledWith(0.05, 0);
    f.engine.setMuted(true); expect(f.oscillators[0].stop).toHaveBeenCalledOnce();
    f.engine.setMuted(false); expect(f.oscillators).toHaveLength(2);
    f.engine.setLevel(0); expect(f.oscillators[1].stop).toHaveBeenCalledOnce();
    f.engine.setLevel(100); expect(f.gain.gain.setValueAtTime).toHaveBeenLastCalledWith(0.1, 0);
    f.engine.update("run", { ...on, volume: 0 });
    expect(f.oscillators[2].stop).toHaveBeenCalledOnce(); f.engine.dispose();
  });
  it("Stop ignores delayed or earlier run events and unmute cannot restart them", async () => {
    const f = fixture(); await f.engine.unlock(); f.engine.begin("old"); f.engine.update("old", on);
    f.engine.stop(); f.engine.stop();
    expect(f.oscillators[0].stop).toHaveBeenCalledOnce();
    expect(f.engine.update("old", on)).toBe(false);
    f.engine.setMuted(false); expect(f.oscillators).toHaveLength(1);
    f.engine.begin("new"); expect(f.engine.update("old", on)).toBe(false);
    f.engine.update("new", on); expect(f.oscillators).toHaveLength(2); f.engine.dispose();
  });
  it("close/unmount disconnects the graph and closes its context exactly once", async () => {
    const f = fixture(); await f.engine.unlock(); f.engine.begin("run"); f.engine.update("run", on);
    f.engine.dispose(); f.engine.dispose();
    expect(f.oscillators[0].stop).toHaveBeenCalledOnce(); expect(f.oscillators[0].disconnect).toHaveBeenCalledOnce();
    expect(f.gain.disconnect).toHaveBeenCalledOnce(); expect(f.context.close).toHaveBeenCalledOnce();
    await f.engine.unlock(); expect(f.factory).toHaveBeenCalledOnce(); expect(f.engine.update("run", on)).toBe(false);
  });
  it("resumes a suspended context and reports blocked autoplay without hanging", async () => {
    vi.useFakeTimers(); const f = fixture(); f.context.state = "suspended";
    await f.engine.unlock(); expect(f.context.resume).toHaveBeenCalledOnce(); f.engine.dispose();
    const blocked = fixture(); blocked.context.state = "suspended";
    blocked.context.resume.mockImplementation(() => new Promise<void>(() => {}));
    const result = expect(blocked.engine.unlock()).rejects.toThrow("เปิดเสียง");
    await vi.advanceTimersByTimeAsync(2001); await result;
    expect(blocked.oscillators).toHaveLength(0); blocked.engine.dispose();
  });
  it("reports missing Web Audio and rejected resume rather than pretending sound played", async () => {
    vi.stubGlobal("AudioContext", undefined);
    const engine = new BuzzerAudio(); await expect(engine.unlock()).rejects.toThrow("Web Audio"); engine.dispose();
    const f = fixture(); f.context.state = "suspended"; f.context.resume.mockRejectedValue(new Error("fixture blocked"));
    await expect(f.engine.unlock()).rejects.toThrow("fixture blocked"); f.engine.dispose();
  });
  it("rejects non-finite and out-of-range audio messages without starting an oscillator", async () => {
    const f = fixture(); await f.engine.unlock(); f.engine.begin("run");
    for (const bad of [{ ...on, frequency: NaN }, { ...on, frequency: 20001 }, { ...on, volume: -1 }, { playing: false, frequency: 440, volume: 50 }, null]) {
      expect(isBuzzerState(bad)).toBe(false); expect(() => f.engine.update("run", bad)).toThrow();
    }
    expect(f.oscillators).toHaveLength(0); expect(() => f.engine.setLevel(Infinity)).toThrow(); f.engine.dispose();
  });
});
