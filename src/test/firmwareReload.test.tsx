import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { act, cleanup, fireEvent, render, screen, waitFor } from "@testing-library/react";
import App from "../App";

const fixture = vi.hoisted(() => ({
    invoke: vi.fn(),
    listeners: new Map<string, Set<(event: { payload: unknown }) => unknown>>(),
    filePath: "D:/project/main/main.c",
    disk: "void app_main(void) { /* Hello */ }",
}));
vi.mock("@tauri-apps/api/core", () => ({ invoke: (...args: unknown[]) => fixture.invoke(...args) }));
vi.mock("@tauri-apps/api/event", () => ({ listen: (name: string, callback: (event: { payload: unknown }) => unknown) => {
    const callbacks = fixture.listeners.get(name) || new Set();
    callbacks.add(callback); fixture.listeners.set(name, callbacks);
    return Promise.resolve(() => { callbacks.delete(callback); });
} }));
vi.mock("../pendingReviewRecovery", () => ({}));
vi.mock("../AiChat", () => ({ default: () => null }));
vi.mock("../CodeEditor", () => ({ default: ({ value }: { value: string }) => <pre data-testid="firmware-editor">{value}</pre> }));
vi.mock("../WikiView", () => ({ default: () => null }));
vi.mock("../SimulatorPanel", () => ({ default: () => null }));
vi.mock("../ToolchainSetup", () => ({ default: () => null }));
vi.mock("../ToolchainStorage", () => ({ ToolchainStoragePicker: () => null, useToolchainStorage: () => ({ ready: true }) }));

async function emit(name: string, payload: unknown) {
    await act(async () => {
        await Promise.all([...fixture.listeners.get(name) || []].map(callback => callback({ payload })));
    });
}

beforeEach(() => {
    fixture.listeners.clear(); fixture.invoke.mockReset();
    fixture.disk = "void app_main(void) { /* Hello */ }";
    fixture.filePath = "D:/project/main/main.c";
    localStorage.clear(); localStorage.setItem("vibe-project-dir", "D:/project");
    fixture.invoke.mockImplementation(async (name: string) => {
        if (name === "get_idf_custom_paths") return { idf_path: "", tools_path: "" };
        if (name === "check_toolchain") return { status: "ready", version: "fixture" };
        if (name === "check_esp_idf") return "Ready";
        if (name === "list_serial_ports") return [];
        if (name === "list_project_files") return [{ name: "main.c", path: fixture.filePath, is_dir: false }];
        if (name === "validate_idf_project") return true;
        if (name === "read_project_file") return fixture.disk;
        return null;
    });
});
afterEach(() => { cleanup(); localStorage.clear(); fixture.listeners.clear(); });

async function openFirmware() {
    render(<App />);
    fireEvent.click(await screen.findByText("main.c", { exact: true }));
    await waitFor(() => expect(screen.getByTestId("firmware-editor")).toHaveTextContent("Hello"));
}

describe("firmware kept on disk is reflected in the editor", () => {
    it.each([
        ["D:/project/main/main.c", "//?/D:/project/main/main.c"],
        ["//?/D:/project/main/main.c", "D:/project/main/main.c"],
        ["D:/project/main/main.c", "d:/PROJECT/main/MAIN.c"],
    ])("reloads aliases %s -> %s", async (openPath, eventPath) => {
        fixture.filePath = openPath;
        await openFirmware();
        fixture.disk = 'void app_main(void) { /* SW1 Pressed */ }';
        await emit("file-modified", JSON.stringify({ path: eventPath }));
        await waitFor(() => expect(screen.getByTestId("firmware-editor")).toHaveTextContent("SW1 Pressed"));
        expect(fixture.invoke.mock.calls.some(([name]) => name === "write_project_file")).toBe(false);
    });
    it("reuses the existing tab when a proposal uses a canonical Windows alias", async () => {
        await openFirmware();
        const before = screen.getAllByText("main.c", { exact: true }).length;
        await emit("ai-diff-pending", JSON.stringify({ fullPath: "//?/D:/project/main/main.c", relPath: "main/main.c" }));
        expect(screen.getAllByText("main.c", { exact: true })).toHaveLength(before);
        expect(fixture.invoke.mock.calls.some(([name]) => name === "accept_diff")).toBe(false);
    });
    it("does not replace the editor for a same-name file from another project", async () => {
        await openFirmware();
        const reads = fixture.invoke.mock.calls.filter(([name]) => name === "read_project_file").length;
        fixture.disk = "other project code";
        await emit("file-modified", JSON.stringify({ path: "//?/D:/other/main/main.c" }));
        expect(screen.getByTestId("firmware-editor")).toHaveTextContent("Hello");
        expect(fixture.invoke.mock.calls.filter(([name]) => name === "read_project_file")).toHaveLength(reads);
    });
});
