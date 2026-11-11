import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { act, cleanup, fireEvent, render, screen, waitFor } from "@testing-library/react";
import ToolchainSetup from "../ToolchainSetup";

const { invokeMock, storageMock, state } = vi.hoisted(() => ({ invokeMock: vi.fn(), storageMock: vi.fn(), state: { handler: null as null | ((event: { payload: { stage: string; percent: number; message: string } }) => void) } }));
vi.mock("@tauri-apps/api/core", () => ({
    invoke: (name: string, ...args: unknown[]) => name === "get_toolchain_storage" ? storageMock(args[0]) : invokeMock(name, ...args),
}));
vi.mock("@tauri-apps/api/event", () => ({ listen: vi.fn((_name, handler) => { state.handler = handler; return Promise.resolve(() => {}); }) }));

beforeEach(() => {
    storageMock.mockResolvedValue({
        required_bytes: 12 * 1024 ** 3,
        options: [{ install_dir: "C:/fixture/toolchain", available_bytes: 20 * 1024 ** 3, sufficient: true, error: null }],
        recommended_dir: "C:/fixture/toolchain", warning: null,
    });
});
afterEach(() => { cleanup(); vi.clearAllMocks(); state.handler = null; });

describe("toolchain activation in the same app session", () => {
    it("verifies readiness instead of trusting a premature completion event", async () => {
        invokeMock.mockResolvedValue({ status: "not_installed", version: null, path: null });
        const onReady = vi.fn();
        render(<ToolchainSetup onReady={onReady} mini />);
        await waitFor(() => expect(invokeMock).toHaveBeenCalledWith("check_toolchain"));
        await act(async () => state.handler?.({ payload: { stage: "done", percent: 100, message: "ready" } }));
        expect(invokeMock.mock.calls.filter(([name]) => name === "check_toolchain")).toHaveLength(2);
        await new Promise(resolve => setTimeout(resolve, 850));
        expect(onReady).not.toHaveBeenCalled();
    });
    it("activates after the download result even when the completion event is missed", async () => {
        let installed = false;
        invokeMock.mockImplementation(async (name) => {
            if (name === "download_toolchain") { installed = true; return "installed"; }
            return { status: installed ? "ready" : "not_installed", version: installed ? "1.0.1" : null, path: installed ? "fixture-toolchain" : null };
        });
        const onReady = vi.fn();
        render(<ToolchainSetup onReady={onReady} mini />);
        fireEvent.click(await screen.findByText(/Auto-installing in/));
        const install = await screen.findByRole("button", { name: /Install Now/ });
        await waitFor(() => expect(install).toBeEnabled());
        fireEvent.click(install);
        await waitFor(() => expect(onReady).toHaveBeenCalled(), { timeout: 1500 });
        expect(invokeMock).toHaveBeenCalledWith("download_toolchain", { url: null, installDir: "C:/fixture/toolchain" });
        expect(installed).toBe(true);
    });
});

describe("manual toolchain installation for Simulator users", () => {
    it("does not automatically download and lets the user defer SDK installation", async () => {
        vi.useFakeTimers();
        try {
            invokeMock.mockResolvedValue({ status: "not_installed", version: null, path: null });
            const onDefer = vi.fn();
            render(<ToolchainSetup onReady={vi.fn()} mini autoInstall={false} onDefer={onDefer} />);
            await act(async () => { await vi.advanceTimersByTimeAsync(6000); });
            expect(invokeMock.mock.calls.some(([name]) => name === "download_toolchain")).toBe(false);
            expect(screen.getByLabelText("Install board toolchain")).toBeInTheDocument();
            fireEvent.click(screen.getByRole("button", { name: "ใช้ Simulator ก่อน" }));
            expect(onDefer).toHaveBeenCalledOnce();
        } finally { vi.useRealTimers(); }
    });
    it("installs only on an explicit click and still activates without reload", async () => {
        let installed = false;
        invokeMock.mockImplementation(async name => {
            if (name === "download_toolchain") { installed = true; return "installed"; }
            return { status: installed ? "ready" : "not_installed", version: installed ? "1.0.1" : null, path: null };
        });
        const onReady = vi.fn();
        render(<ToolchainSetup onReady={onReady} mini autoInstall={false} onDefer={vi.fn()} />);
        const install = await screen.findByLabelText("Install board toolchain");
        await waitFor(() => expect(install).toBeEnabled());
        fireEvent.click(install);
        await waitFor(() => expect(onReady).toHaveBeenCalled(), { timeout: 1500 });
        expect(invokeMock.mock.calls.filter(([name]) => name === "download_toolchain")).toHaveLength(1);
    });
});

describe("broken macOS toolchain",()=>{
    it("does not accept an old marker when native Python cannot run, and shows repair guidance",async()=>{
        const onReady=vi.fn();
        invokeMock.mockResolvedValue({status:"needs_repair",platform:"macos",version:null,path:"/fixture/mac/toolchain",message:"No native Python environment. Install Python 3.12 and use Install board toolchain."});
        render(<ToolchainSetup onReady={onReady} mini autoInstall={false}/>);
        expect(await screen.findByRole("alert")).toHaveTextContent("No native Python environment");
        expect(onReady).not.toHaveBeenCalled();
        expect(invokeMock.mock.calls.some(([name])=>name==="download_toolchain")).toBe(false);
        expect(screen.getByText(/Retry Installation/)).toBeInTheDocument();
    });
});
