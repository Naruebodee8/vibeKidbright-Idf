import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { act, cleanup, fireEvent, render, screen, waitFor } from "@testing-library/react";
import ToolchainSetup from "../ToolchainSetup";
import type { StoragePlan } from "../ToolchainStorage";

const { invokeMock, storageMock, state } = vi.hoisted(() => ({
  invokeMock: vi.fn(), storageMock: vi.fn(),
  state: { handler: null as null | ((event: { payload: { stage: string; percent: number; message: string } }) => void) }
}));
vi.mock("@tauri-apps/api/core", () => ({
  invoke: (name: string, args?: unknown) => name === "get_toolchain_storage" ? storageMock(args) : invokeMock(name, args),
}));
vi.mock("@tauri-apps/api/event", () => ({ listen: vi.fn((_name, handler) => { state.handler = handler; return Promise.resolve(() => {}); }) }));
const C = "C:/fixture/toolchain";
const D = "D:/fixture/toolchain";
const plan = (c = true, d = true): StoragePlan => ({
  required_bytes: 12 * 1024 ** 3,
  options: [C, D].map((dir, i) => ({ install_dir: dir, available_bytes: (i === 0 ? c : d) ? 20 * 1024 ** 3 : 1024 ** 3,
    sufficient: i === 0 ? c : d, error: null })),
  recommended_dir: c ? C : d ? D : null,
  warning: c ? null : d ? "ตำแหน่งเดิมมีพื้นที่ไม่พอ เปลี่ยนตำแหน่งแนะนำเป็น D:" : "พื้นที่ไม่พอในทุกไดรฟ์",
});
beforeEach(() => { storageMock.mockResolvedValue(plan()); });
afterEach(() => { cleanup(); vi.clearAllMocks(); state.handler = null; });

function installFixture() {
  let installed = false;
  invokeMock.mockImplementation(async name => {
    if (name === "download_toolchain") { installed = true; return "installed"; }
    return { status: installed ? "ready" : "not_installed", version: installed ? "1.0.1" : null, path: null };
  });
}
describe("SDK disk capacity preflight", () => {
  it("selects the second drive if the first is full and passes it to the installer", async () => {
    storageMock.mockResolvedValue(plan(false, true));
    installFixture();
    render(<ToolchainSetup onReady={vi.fn()} mini autoInstall={false} />);
    const install = await screen.findByLabelText("Install board toolchain");
    await waitFor(() => expect(install).toBeEnabled());
    expect(screen.getByLabelText("SDK installation location")).toHaveValue(D);
    fireEvent.click(install);
    await waitFor(() => expect(invokeMock).toHaveBeenCalledWith("download_toolchain", { url: null, installDir: D }));
  });
  it("warns and blocks download when both drives are full, while allowing Simulator", async () => {
    storageMock.mockResolvedValue(plan(false, false));
    installFixture();
    const defer = vi.fn();
    render(<ToolchainSetup onReady={vi.fn()} mini autoInstall={false} onDefer={defer} />);
    expect(await screen.findByRole("alert")).toHaveTextContent("ทุกไดรฟ์");
    expect(screen.getByLabelText("Install board toolchain")).toBeDisabled();
    fireEvent.click(screen.getByLabelText("Install board toolchain"));
    expect(invokeMock.mock.calls.some(([name]) => name === "download_toolchain")).toBe(false);
    fireEvent.click(screen.getByText("ใช้ Simulator ก่อน"));
    expect(defer).toHaveBeenCalledOnce();
  });
  it("rechecks stale capacity before starting any download", async () => {
    installFixture();
    render(<ToolchainSetup onReady={vi.fn()} mini autoInstall={false} />);
    const install = await screen.findByLabelText("Install board toolchain");
    await waitFor(() => expect(install).toBeEnabled());
    storageMock.mockResolvedValue(plan(false, false));
    fireEvent.click(install);
    await screen.findAllByText(/ทุกไดรฟ์/);
    expect(invokeMock.mock.calls.some(([name]) => name === "download_toolchain")).toBe(false);
  });
  it("does not download if checking disk space fails", async () => {
    storageMock.mockRejectedValue(Error("drive offline"));
    installFixture();
    render(<ToolchainSetup onReady={vi.fn()} mini autoInstall={false} />);
    expect(await screen.findByRole("alert")).toHaveTextContent("drive offline");
    expect(screen.getByLabelText("Install board toolchain")).toBeDisabled();
    expect(invokeMock.mock.calls.some(([name]) => name === "download_toolchain")).toBe(false);
  });
  it("honours an explicitly selected drive", async () => {
    installFixture();
    render(<ToolchainSetup onReady={vi.fn()} mini autoInstall={false} />);
    const install = await screen.findByLabelText("Install board toolchain");
    await waitFor(() => expect(install).toBeEnabled());
    fireEvent.change(screen.getByLabelText("SDK installation location"), { target: { value: D } });
    fireEvent.click(install);
    await waitFor(() => expect(invokeMock).toHaveBeenCalledWith("download_toolchain", { url: null, installDir: D }));
  });
  it("does not loop failed preflights when a legacy automatic countdown expires", async () => {
    vi.useFakeTimers();
    try {
      storageMock.mockResolvedValue(plan(false, false));
      installFixture();
      render(<ToolchainSetup onReady={vi.fn()} mini />);
      await act(async () => { await vi.advanceTimersByTimeAsync(15000); });
      expect(invokeMock.mock.calls.some(([name]) => name === "download_toolchain")).toBe(false);
      expect(storageMock.mock.calls.length).toBeLessThan(6);
    } finally { vi.useRealTimers(); }
  });
});
