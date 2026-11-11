import {beforeEach,describe,expect,it,vi} from "vitest";
const invokeMock=vi.hoisted(()=>vi.fn());
vi.mock("@tauri-apps/api/core",()=>({invoke:invokeMock}));
import {clearRecoveredReview,restorePendingReviewBackup} from "../pendingReviewRecovery";
const KEY="vibekidbright-pending-review-recovery";
beforeEach(()=>{
  localStorage.clear();invokeMock.mockReset();invokeMock.mockResolvedValue([]);
  localStorage.setItem("vibe-project-dir","D:/project");
});
const entries=[
  {path:"D:/project/main/main.c",content:"new",baseSha256:"old hash"},
  {path:"D:/other/main/main.c",content:"other",baseSha256:"other hash"},
];
describe("Review snapshot recovery without approval",()=>{
  it("restores pending proposals and keeps recovery data until an explicit decision",async()=>{
    localStorage.setItem(KEY,JSON.stringify({project:"D:/project",entries:[entries[0]]}));
    await restorePendingReviewBackup();
    expect(invokeMock).toHaveBeenCalledWith("restore_pending_diff_backup",{projectDir:"D:/project",entries:[entries[0]]});
    expect(localStorage.getItem(KEY)).not.toBeNull();
    expect(invokeMock.mock.calls.map(c=>c[0])).not.toContain("accept_diff");
  });
  it("does not restore a snapshot belonging to a different selected project",async()=>{
    localStorage.setItem(KEY,JSON.stringify({project:"D:/other",entries:[entries[1]]}));
    await restorePendingReviewBackup();expect(invokeMock).not.toHaveBeenCalled();
  });
  it("clears only the explicitly reviewed file including Windows prefix aliases",()=>{
    localStorage.setItem(KEY,JSON.stringify({project:"D:/project",entries}));
    const alias=["","","?","D:","project","main","main.c"].join("\\");
    clearRecoveredReview(alias);
    expect(JSON.parse(localStorage.getItem(KEY)!).entries).toEqual([entries[1]]);
  });
  it("keeps the snapshot if native recovery fails",async()=>{
    localStorage.setItem(KEY,JSON.stringify({project:"D:/project",entries:[entries[0]]}));
    invokeMock.mockRejectedValue(Error("base changed"));
    await expect(restorePendingReviewBackup()).rejects.toThrow("base changed");
    expect(localStorage.getItem(KEY)).not.toBeNull();
  });
});
