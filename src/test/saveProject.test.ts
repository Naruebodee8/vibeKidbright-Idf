import { describe, expect, it, vi } from "vitest";
import { saveDirtyFiles } from "../saveProject";
import { executeBuildLifecycle } from "../buildFlash";

describe("save before build", () => {
  it("awaits all dirty writes before starting the existing command", async () => {
    const order: string[] = [];
    let release!: () => void;
    const blocked = new Promise<void>(resolve => { release = resolve; });
    const files = [
      { path: "main/main.c", content: "firmware", savedContent: "old" },
      { path: "simulation/main.sim.js", content: "ports.output2_write(1);", savedContent: "" },
      { path: "CMakeLists.txt", content: "same", savedContent: "same" },
    ];
    const task = executeBuildLifecycle({
      save: () => saveDirtyFiles(files, async path => { order.push(path); if (path === "main/main.c") await blocked; }, () => {}),
      run: async () => { order.push("idf.py build flash"); },
      setResult: vi.fn(), setBuilding: vi.fn(), onFailure: vi.fn(),
    });
    expect(order).toEqual(["main/main.c"]);
    release(); await task;
    expect(order).toEqual(["main/main.c", "simulation/main.sim.js", "idf.py build flash"]);
  });
  it("does not build or report success if a save fails", async () => {
    const run = vi.fn(), onFailure = vi.fn(); const results: string[] = [];
    await executeBuildLifecycle({ save: async () => { throw Error("disk full"); }, run,
      setResult: value => { results.push(value); }, setBuilding: vi.fn(), onFailure });
    expect(run).not.toHaveBeenCalled();
    expect(results).toEqual(["building", "failed"]);
    expect(onFailure).toHaveBeenCalledWith(expect.objectContaining({ message: "disk full" }));
  });
  it("marks only successfully written snapshots as saved and preserves newer edits", async () => {
    const files = [{ path: "main/main.c", content: "v2", savedContent: "v1" },
      { path: "main/other.c", content: "new", savedContent: "old" }];
    const saved: string[] = [];
    await expect(saveDirtyFiles(files, async path => {
      if (path.endsWith("other.c")) throw Error("permission denied");
      files[0].content = "v3";
    }, snapshot => { files.find(file => file.path === snapshot.path)!.savedContent = snapshot.content; saved.push(snapshot.path); })).rejects.toThrow("permission denied");
    expect(saved).toEqual(["main/main.c"]);
    expect(files[0]).toMatchObject({ content: "v3", savedContent: "v2" });
    expect(files[1].savedContent).toBe("old");
  });
});
