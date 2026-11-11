export interface ProjectFileSnapshot {
  path: string;
  content: string;
  savedContent: string;
}

/** Persist a snapshot; edits made while a write is pending must remain dirty. */
export async function saveDirtyFiles(
  files: readonly ProjectFileSnapshot[],
  write: (path: string, content: string) => Promise<unknown>,
  saved: (file: ProjectFileSnapshot) => void,
): Promise<number> {
  const snapshot = files.filter(file => file.content !== file.savedContent).map(file => ({ ...file }));
  for (const file of snapshot) {
    await write(file.path, file.content);
    saved(file);
  }
  return snapshot.length;
}
