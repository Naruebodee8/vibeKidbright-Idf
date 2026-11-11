import { invoke } from "@tauri-apps/api/core";
import { normPath } from "./utils";
const KEY = "vibekidbright-pending-review-recovery";
type Snapshot = { project:string; entries:{path:string;content:string;baseSha256:string}[] };
export async function restorePendingReviewBackup() {
  const data=localStorage.getItem(KEY);
  if(!data)return;
  const snapshot=JSON.parse(data) as Snapshot;
  if(normPath(snapshot.project)!==normPath(localStorage.getItem("vibe-project-dir")??""))return;
  await invoke("restore_pending_diff_backup",{projectDir:snapshot.project,entries:snapshot.entries});
  // Retain the snapshot across dev reloads until the user explicitly Keep/Undo.
}
export function clearRecoveredReview(path:string) {
  const data=localStorage.getItem(KEY);
  if(!data)return;
  try {
    const snapshot=JSON.parse(data) as Snapshot;
    snapshot.entries=snapshot.entries.filter(entry=>normPath(entry.path)!==normPath(path));
    if(snapshot.entries.length)localStorage.setItem(KEY,JSON.stringify(snapshot));else localStorage.removeItem(KEY);
  } catch { /* Leave unrecognized recovery data intact. */ }
}
if(typeof window!=="undefined"&&(window as unknown as {__TAURI_INTERNALS__?:unknown}).__TAURI_INTERNALS__) {
  void restorePendingReviewBackup().catch(error=>console.error("Pending review recovery remains available:",error));
}
