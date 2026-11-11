import { StrictMode } from "react";
import { act, cleanup, fireEvent, render, screen, waitFor } from "@testing-library/react";
import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
const f = vi.hoisted(() => ({
  emit: null as null | ((event: any) => void), runId: "", failStop: false, commands: [] as string[],
}));
vi.mock("@tauri-apps/api/core", () => ({ invoke: vi.fn(async (name: string, args: any) => name === "start_simulator_host"
  ? { url: "http://127.0.0.1:5555/session/UnityBuild/index.html", origin: "http://127.0.0.1:5555", session_id: args.sessionId } : null) }));
vi.mock("@tauri-apps/api/event", () => ({ listen: vi.fn(async () => () => {}) }));
vi.mock("../simulator/SimulatorAdapter", () => ({ SimulatorAdapter: class {
  constructor(_target: Window, _origin: string, _session: string, event: (event: any) => void) { f.emit = event; }
  async request(type: string, _payload?: unknown, runId?: string) {
    f.commands.push(type);
    if (type === "HELLO") f.emit?.({ type: "READY", payload: {} });
    if (type === "RUN") {
      f.runId = runId!;
      f.emit?.({ type: "RUN_STARTED", runId, payload: {} });
      f.emit?.({ type: "BUZZER", runId, payload: { playing: true, frequency: 1000, volume: 50 } });
    }
    if (type === "STOP" && f.failStop) throw new Error("fixture disconnected");
    return { type: "ACK", payload: {} };
  }
  close() {}
} }));
import SimulatorPanel from "../SimulatorPanel";

const nodes: { start: ReturnType<typeof vi.fn>; stop: ReturnType<typeof vi.fn> }[] = [];
const contexts: FakeContext[] = [];
class FakeContext {
  state = "running"; currentTime = 0; destination = {};
  close = vi.fn(async () => { this.state = "closed"; });
  resume = vi.fn(async () => { this.state = "running"; });
  constructor() { contexts.push(this); }
  createGain() { return { gain: { setValueAtTime: vi.fn() }, connect: vi.fn(), disconnect: vi.fn() }; }
  createOscillator() {
    const node = { type: "sine", frequency: { setValueAtTime: vi.fn() }, connect: vi.fn(), disconnect: vi.fn(), start: vi.fn(), stop: vi.fn() };
    nodes.push(node); return node;
  }
}
beforeEach(() => { f.emit = null; f.runId = ""; f.failStop = false; f.commands = []; nodes.length = 0; contexts.length = 0; vi.stubGlobal("AudioContext", FakeContext); });
afterEach(() => { cleanup(); vi.unstubAllGlobals(); });
async function open(strict = false) {
  const onClose = vi.fn();
  const panel = <SimulatorPanel projectDir="." initialSource="sound.tone(1000);vTaskDelay(5000);sound.off();" onDraftChange={vi.fn()} onClose={onClose} />;
  const view = render(strict ? <StrictMode>{panel}</StrictMode> : panel);
  fireEvent.load(await screen.findByTitle("Unity KidBright board"));
  await waitFor(() => expect(screen.getByRole("button", { name: "Run Simulator" })).toBeEnabled());
  fireEvent.click(screen.getByRole("button", { name: "Run Simulator" }));
  await waitFor(() => expect(nodes).toHaveLength(1));
  expect(screen.getByLabelText("Buzzer status")).toHaveTextContent("1000 Hz");
  return { ...view, onClose };
}
describe("Simulator panel local buzzer controls", () => {
  it("stops audio before a failed Stop acknowledgement and ignores late messages", async () => {
    await open(); f.failStop = true;
    fireEvent.click(screen.getByRole("button", { name: "Stop" }));
    expect(nodes[0].stop).toHaveBeenCalledOnce();
    await screen.findByText("Error: fixture disconnected");
    await act(async () => f.emit?.({ type: "BUZZER", runId: f.runId, payload: { playing: true, frequency: 2000, volume: 50 } }));
    expect(nodes).toHaveLength(1);
  });
  it("mute/volume work while running, and Reset prevents a stale tone from returning", async () => {
    await open();
    fireEvent.click(screen.getByLabelText("Mute simulator")); expect(nodes[0].stop).toHaveBeenCalledOnce();
    fireEvent.click(screen.getByLabelText("Unmute simulator")); await waitFor(() => expect(nodes).toHaveLength(2));
    fireEvent.change(screen.getByLabelText("Simulator volume"), { target: { value: "0" } }); expect(nodes[1].stop).toHaveBeenCalledOnce();
    fireEvent.change(screen.getByLabelText("Simulator volume"), { target: { value: "50" } }); expect(nodes).toHaveLength(3);
    fireEvent.click(screen.getByRole("button", { name: "Reset" }));
    await waitFor(() => expect(screen.getByLabelText("Buzzer status")).toHaveTextContent("หยุด"));
    expect(nodes[2].stop).toHaveBeenCalledOnce();
    await act(async () => f.emit?.({ type: "BUZZER", runId: "earlier-run", payload: { playing: true, frequency: 1000, volume: 50 } }));
    expect(nodes).toHaveLength(3);
  });
  it("close stops immediately, unmount closes the context, and StrictMode can still run", async () => {
    const view = await open(true);
    fireEvent.click(screen.getByLabelText("Close Simulator"));
    expect(nodes[0].stop).toHaveBeenCalledOnce(); expect(view.onClose).toHaveBeenCalledOnce();
    view.unmount(); expect(contexts[0].close).toHaveBeenCalledOnce();
  });
});
