import { act, fireEvent, render, screen, waitFor, cleanup } from "@testing-library/react";
import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";

const fixture = vi.hoisted(() => ({
  calls: [] as { name: string; args: any }[], commands: [] as string[],
  customIdf: "",
  write: (async () => {}) as () => Promise<unknown>,
  run: (async () => ({ success: true, code: 0 })) as () => Promise<unknown>,
}));
vi.mock("@tauri-apps/api/core", () => ({ invoke: vi.fn(async (name: string, args: any) => {
  fixture.calls.push({ name, args });
  if (name === "check_toolchain") return { status: "ready", version: "fixture" };
  if (name === "check_esp_idf") return "Ready: fixture SDK";
  if (name === "get_idf_custom_paths") return { idf_path: fixture.customIdf, tools_path: "fixture-tools" };
  if (name === "validate_idf_project") return true;
  if (name === "list_project_files") return [{ name: "main.c", path: "/fixture/project/main/main.c", is_dir: false }];
  if (name === "list_serial_ports") return ["COM_TEST"];
  if (name === "read_project_file") return "void app_main(void) {}";
  if (name === "write_project_file") return fixture.write();
  if (name === "run_shell_command") return fixture.run();
  if (name === "start_simulator_host") return { url: "http://127.0.0.1:5555/session/UnityBuild/index.html", origin: "http://127.0.0.1:5555", session_id: args.sessionId };
  if (name === "load_project_simulation") return { path: "simulation/main.sim.js", source: "ports.output2_write(1);", revision: "fixture", firmware_files: ["main/main.c"] };
  return null;
}) }));
vi.mock("@tauri-apps/api/event", () => ({ listen: vi.fn(async () => () => {}) }));
vi.mock("../ToolchainSetup", async () => {
  const { useEffect } = await import("react");
  return { default: ({ onReady }: { onReady: () => void }) => { useEffect(onReady, []); return null; } };
});
vi.mock("../CodeEditor", () => ({ default: ({ value, onChange }: any) => <textarea aria-label="Firmware editor" value={value} onChange={event => onChange(event.target.value)} /> }));
vi.mock("../AiChat", () => ({ default: ({ onOpenSimulator }: any) => <button onClick={onOpenSimulator}>Chat Run Sim</button> }));
vi.mock("../WikiView", () => ({ default: () => null }));
vi.mock("../simulator/SimulatorAdapter", () => ({ SimulatorAdapter: class {
  constructor(_target: Window, _origin: string, _session: string, private event: (event: any) => void) {}
  async request(type: string, _payload?: unknown, runId?: string | null) {
    fixture.commands.push(type);
    if (type === "HELLO") this.event({ type: "READY", payload: {} });
    if (type === "RUN") this.event({ type: "RUN_STARTED", runId, payload: {} });
    if (type === "STOP") this.event({ type: "RUN_ENDED", runId, payload: { reason: "stopped" } });
    return { type: "ACK", payload: {} };
  }
  close() {}
} }));
import App from "../App";

beforeEach(() => {
  fixture.calls = []; fixture.commands = []; fixture.customIdf = "";
  fixture.write = async () => {};
  fixture.run = async () => ({ success: true, code: 0 });
  localStorage.clear(); localStorage.setItem("vibe-project-dir", "/fixture/project");
});
afterEach(cleanup);
async function editFirmware() {
  render(<App />);
  fireEvent.click(await screen.findByText("main.c"));
  fireEvent.change(await screen.findByLabelText("Firmware editor"), { target: { value: "void app_main(void) { /* edited */ }" } });
  await waitFor(() => expect(screen.getByRole("button", { name: /Build & Flash/ })).not.toBeDisabled());
}
async function openSimulator() {
  fireEvent.click(screen.getByLabelText("Run project simulator"));
  fireEvent.load(await screen.findByTitle("Unity KidBright board"));
  await waitFor(() => expect(screen.getByRole("button", { name: "Run Simulator" })).not.toBeDisabled());
}
describe("Simulator and existing Build/Flash integration", () => {
  it("waits for a dirty save and rejects repeated clicks before the original command", async () => {
    await editFirmware();
    let release!: () => void;
    fixture.write = () => new Promise<void>(resolve => { release = resolve; });
    const button = screen.getByRole("button", { name: /Build & Flash/ });
    fireEvent.click(button); fireEvent.click(button);
    await waitFor(() => expect(fixture.calls.filter(call => call.name === "write_project_file")).toHaveLength(1));
    expect(fixture.calls.some(call => call.name === "run_shell_command")).toBe(false);
    await act(async () => release());
    await waitFor(() => expect(fixture.calls.filter(call => call.name === "run_shell_command")).toHaveLength(1));
    expect(fixture.calls.find(call => call.name === "run_shell_command")?.args).toMatchObject({ cmd: "idf.py", args: ["build", "flash", "-p", "COM_TEST"], cwd: "/fixture/project" });
  });
  it("does not build when saving fails and does not clear the dirty indicator", async () => {
    await editFirmware(); fixture.write = async () => { throw Error("fixture disk full"); };
    fireEvent.click(screen.getByRole("button", { name: /Build & Flash/ }));
    await waitFor(() => expect(screen.getByText(/Build failed:.*fixture disk full/)).toBeInTheDocument());
    expect(fixture.calls.some(call => call.name === "run_shell_command")).toBe(false);
    expect(screen.getByTitle("Unsaved changes")).toBeInTheDocument();
  });
  it("runs/stops the simulator without IDF, then the original Flash command still runs", async () => {
    await editFirmware(); await openSimulator();
    await act(async () => { fireEvent.click(screen.getByRole("button", { name: "Run Simulator" })); });
    await waitFor(() => expect(fixture.commands).toContain("RUN"));
    expect(fixture.calls.some(call => call.name === "run_shell_command" || call.name === "stop_serial_monitor")).toBe(false);
    fireEvent.click(screen.getByRole("button", { name: "Stop" }));
    await waitFor(() => expect(fixture.commands).toContain("STOP"));
    fireEvent.click(screen.getByLabelText("Close Simulator"));
    fireEvent.click(screen.getByRole("button", { name: /Build & Flash/ }));
    await waitFor(() => expect(fixture.calls.filter(call => call.name === "run_shell_command")).toHaveLength(1));
  });
  it("refuses a project simulation if saving its dirty firmware fails", async () => {
    await editFirmware(); await openSimulator();
    fixture.write = async () => { throw Error("fixture permission denied"); };
    fireEvent.click(screen.getByRole("button", { name: "Run Simulator" }));
    await waitFor(() => expect(screen.getByLabelText("Project simulation error")).toHaveTextContent("บันทึกไฟล์ไม่สำเร็จ"));
    expect(fixture.commands).not.toContain("RUN");
    expect(fixture.calls.some(call => call.name === "run_shell_command")).toBe(false);
  });
});

describe("Configured ESP-IDF paths use the same argument-array backend", () => {
  it.each(["/fixture/mac/esp-idf", "C:/Espressif/esp-idf"])("does not invoke cmd.exe/export.bat for %s", async custom => {
    fixture.customIdf=custom;
    await editFirmware();
    fireEvent.click(screen.getByRole("button", { name: /Build & Flash/ }));
    await waitFor(()=>expect(fixture.calls.filter(c=>c.name==="run_shell_command")).toHaveLength(1));
    expect(fixture.calls.find(c=>c.name==="run_shell_command")!.args).toEqual({cmd:"idf.py",args:["build","flash","-p","COM_TEST"],cwd:"/fixture/project"});
  });
});
