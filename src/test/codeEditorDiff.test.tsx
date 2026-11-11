/**
 * codeEditorDiff.test.tsx — tests for the AI diff review flow in CodeEditor.
 * Tauri invoke/listen and Monaco are mocked; the focus is on the accept/reject UX.
 * Run: npm test
 */

import { describe, it, expect, vi, beforeEach } from "vitest";
import { render, screen, fireEvent, waitFor } from "@testing-library/react";
import CodeEditor from "../CodeEditor";

// ── Mocks ─────────────────────────────────────────────────────────────────────

const invokeMock = vi.fn();
const diffProps = vi.hoisted(() => ({ current: null as null | {
    keepCurrentOriginalModel: boolean;
    keepCurrentModifiedModel: boolean;
    onMount: (instance: unknown) => void;
} }));

vi.mock("@tauri-apps/api/core", () => ({
    invoke: (...args: unknown[]) => invokeMock(...args),
}));

vi.mock("@tauri-apps/api/event", () => ({
    listen: vi.fn().mockResolvedValue(() => {}),
}));

vi.mock("@monaco-editor/react", () => ({
    default: (props: { value?: string }) => (
        <div data-testid="mock-editor" data-value={props.value} />
    ),
    DiffEditor: (props: NonNullable<typeof diffProps.current>) => {
        diffProps.current = props;
        return <div data-testid="mock-diff-editor" />;
    },
}));

describe("CodeEditor diff flow", () => {
    beforeEach(() => {
        vi.clearAllMocks();
    });

    it("shows the Review AI Changes banner when a pending diff exists", async () => {
        invokeMock.mockImplementation((cmd: string) => {
            if (cmd === "check_pending_diff") return Promise.resolve("int fixed = 1;");
            return Promise.resolve(null);
        });

        render(<CodeEditor value="int x = 0;" onChange={() => {}} filePath="/proj/main/main.c" />);

        await waitFor(() => {
            expect(screen.getByText("Review AI Changes")).toBeInTheDocument();
        });

        expect(await screen.findByTestId("mock-diff-editor")).toBeInTheDocument();
        expect(screen.queryByTestId("mock-editor")).not.toBeInTheDocument();
    });

    it("does not show the banner when there is no pending diff", async () => {
        invokeMock.mockResolvedValue(null);

        render(<CodeEditor value="int x = 0;" onChange={() => {}} filePath="/proj/main/main.c" />);

        await waitFor(() => {
            expect(invokeMock).toHaveBeenCalledWith("check_pending_diff", {
                path: "/proj/main/main.c",
            });
        });

        expect(screen.queryByText("Review AI Changes")).not.toBeInTheDocument();
        expect(await screen.findByTestId("mock-editor")).toBeInTheDocument();
    });

    it("Keep button calls accept_diff and dismisses the banner", async () => {
        invokeMock.mockImplementation((cmd: string) => {
            if (cmd === "check_pending_diff") return Promise.resolve("new content");
            if (cmd === "accept_diff") return Promise.resolve("ok");
            return Promise.resolve(null);
        });

        render(<CodeEditor value="old content" onChange={() => {}} filePath="/proj/main/main.c" />);
        await screen.findByText("Review AI Changes");

        fireEvent.click(screen.getByText(/Keep/));

        await waitFor(() => {
            expect(invokeMock).toHaveBeenCalledWith("accept_diff", { path: "/proj/main/main.c", expectedContent: "new content" });
        });

        await waitFor(() => {
            expect(screen.queryByText("Review AI Changes")).not.toBeInTheDocument();
        });
    });

    it("Undo button calls reject_diff and dismisses the banner", async () => {
        invokeMock.mockImplementation((cmd: string) => {
            if (cmd === "check_pending_diff") return Promise.resolve("new content");
            if (cmd === "reject_diff") return Promise.resolve("ok");
            return Promise.resolve(null);
        });

        render(<CodeEditor value="old content" onChange={() => {}} filePath="/proj/main/main.c" />);
        await screen.findByText("Review AI Changes");

        fireEvent.click(screen.getByText(/Undo/));

        await waitFor(() => {
            expect(invokeMock).toHaveBeenCalledWith("reject_diff", { path: "/proj/main/main.c", expectedContent: "new content" });
        });

        await waitFor(() => {
            expect(screen.queryByText("Review AI Changes")).not.toBeInTheDocument();
        });
    });

    it("keeps the banner if accept_diff fails (user can retry)", async () => {
        const errSpy = vi.spyOn(console, "error").mockImplementation(() => {});
        invokeMock.mockImplementation((cmd: string) => {
            if (cmd === "check_pending_diff") return Promise.resolve("new content");
            if (cmd === "accept_diff") return Promise.reject(new Error("disk full"));
            return Promise.resolve(null);
        });

        render(<CodeEditor value="old content" onChange={() => {}} filePath="/proj/main/main.c" />);
        await screen.findByText("Review AI Changes");

        fireEvent.click(screen.getByText(/Keep/));

        await waitFor(() => {
            expect(errSpy).toHaveBeenCalled();
        });
        expect(screen.getByText("Review AI Changes")).toBeInTheDocument();
        errSpy.mockRestore();
    });
});


describe("CodeEditor explicit review requests", () => {
    beforeEach(() => { vi.clearAllMocks(); });
    it("sends only one Keep while the native write is in progress", async () => {
        let release!: () => void;
        invokeMock.mockImplementation((cmd:string) => cmd==="check_pending_diff" ? Promise.resolve("reviewed") :
            cmd==="accept_diff" ? new Promise<void>(resolve=>{release=resolve;}) : Promise.resolve(null));
        render(<CodeEditor value="old" onChange={()=>{}} filePath="/proj/main/main.c"/>);
        await screen.findByText("Review AI Changes");
        const keep=screen.getByTitle("Accept AI proposed changes");
        fireEvent.click(keep);fireEvent.click(keep);
        expect(invokeMock.mock.calls.filter(call=>call[0]==="accept_diff")).toHaveLength(1);
        expect(keep).toBeDisabled();
        release();
        await waitFor(()=>expect(screen.queryByText("Review AI Changes")).not.toBeInTheDocument());
    });
    it("checks the exact file again when Simulator requests its review", async () => {
        let proposal:string|null=null;
        invokeMock.mockImplementation((cmd:string)=>Promise.resolve(cmd==="check_pending_diff"?proposal:null));
        render(<CodeEditor value="old" onChange={()=>{}} filePath="D:/project/main/main.c"/>);
        await screen.findByTestId("mock-editor");
        proposal="review this main";
        const alias=["","","?","D:","project","main","main.c"].join("\\");
        fireEvent(window,new CustomEvent("vibe-review-diff",{detail:{path:alias}}));
        await screen.findByText("Review AI Changes");
        expect(invokeMock.mock.calls.some(call=>call[0]==="accept_diff")).toBe(false);
    });
});


it("disposes diff models only after the diff widget has detached them", async () => {
    invokeMock.mockImplementation((cmd: string) => Promise.resolve(cmd === "check_pending_diff" ? "reviewed" : null));
    render(<CodeEditor value="old" onChange={() => {}} filePath="/proj/main/main.c" />);
    await screen.findByTestId("mock-diff-editor");
    const props = diffProps.current!;
    expect(props.keepCurrentOriginalModel).toBe(true);
    expect(props.keepCurrentModifiedModel).toBe(true);
    const original = { isDisposed: () => false, dispose: vi.fn() };
    const modified = { isDisposed: () => false, dispose: vi.fn() };
    let disposed!: () => void;
    props.onMount({ getModel: () => ({ original, modified }), onDidDispose: (callback: () => void) => { disposed = callback; } });
    disposed();
    expect(original.dispose).not.toHaveBeenCalled();
    expect(modified.dispose).not.toHaveBeenCalled();
    await Promise.resolve();
    expect(original.dispose).toHaveBeenCalledTimes(1);
    expect(modified.dispose).toHaveBeenCalledTimes(1);
});
