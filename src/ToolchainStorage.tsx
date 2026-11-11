import { useCallback, useEffect, useRef, useState } from "react";
import { invoke } from "@tauri-apps/api/core";

export interface StorageOption {
  install_dir: string;
  available_bytes: number | null;
  sufficient: boolean;
  error: string | null;
}
export interface StoragePlan {
  required_bytes: number;
  options: StorageOption[];
  recommended_dir: string | null;
  warning: string | null;
}
const gib = (bytes: number) => (bytes / 1024 ** 3).toFixed(1);

export function useToolchainStorage(enabled: boolean) {
  const [plan, setPlan] = useState<StoragePlan | null>(null);
  const [selected, setSelected] = useState("");
  const [checking, setChecking] = useState(false);
  const [error, setError] = useState("");
  const selection = useRef("");
  const directory = useRef<string | null>(null);
  const select = useCallback((path: string) => { selection.current = path; setSelected(path); }, []);
  const refresh = useCallback(async (newDirectory?: string) => {
    if (newDirectory !== undefined) directory.current = newDirectory;
    setChecking(true);
    setError("");
    try {
      const result = await invoke<StoragePlan>("get_toolchain_storage", { directory: directory.current });
      if (!result || !Array.isArray(result.options)) throw Error("ไม่สามารถอ่านพื้นที่จัดเก็บจากโปรแกรมได้");
      setPlan(result);
      const keep = newDirectory === undefined && result.options.some(o => o.install_dir === selection.current && o.sufficient);
      const path = keep ? selection.current : result.recommended_dir ?? "";
      select(path);
      return { result, path };
    } catch (err) {
      setPlan(null);
      select("");
      setError(String(err));
      throw err;
    } finally { setChecking(false); }
  }, [select]);
  useEffect(() => { if (enabled) void refresh().catch(() => {}); }, [enabled, refresh]);
  const prepare = async () => {
    const { result, path } = await refresh();
    if (!path) throw Error(result.warning ?? "พื้นที่ไม่พอสำหรับติดตั้ง SDK");
    return path;
  };
  const browse = async () => {
    try {
      const path = await invoke<string | null>("pick_directory");
      if (path) await refresh(path);
    } catch (err) { setError(String(err)); }
  };
  return { plan, selected, select, checking, error, refresh, prepare, browse,
    ready: !checking && !!plan?.options.some(o => o.install_dir === selected && o.sufficient) };
}

export function ToolchainStoragePicker({ storage, disabled = false }: {
  storage: ReturnType<typeof useToolchainStorage>; disabled?: boolean;
}) {
  return <div className="my-3 text-xs space-y-2" style={{ color: "#cbd5e1" }}>
    <label className="block">
      ตำแหน่งติดตั้ง SDK
      <select aria-label="SDK installation location" value={storage.selected}
        disabled={disabled || storage.checking} onChange={e => storage.select(e.target.value)}
        className="block w-full mt-1 p-2 rounded bg-slate-800 border border-slate-600">
        <option value="">เลือกตำแหน่งติดตั้ง</option>
        {storage.plan?.options.map(option => <option key={option.install_dir} value={option.install_dir} disabled={!option.sufficient}>
          {option.install_dir} · {option.available_bytes === null ? "ตรวจไม่ได้" : "ว่าง " + gib(option.available_bytes) + " GiB"}{option.sufficient ? "" : " · ใช้ไม่ได้"}
        </option>)}
      </select>
    </label>
    {storage.selected && <p className="break-all">{storage.selected}</p>}
    <p>{storage.checking ? "กำลังตรวจพื้นที่จัดเก็บ…" : storage.plan
      ? "สำรองอย่างน้อย " + gib(storage.plan.required_bytes) + " GiB สำหรับ ZIP และแตกไฟล์ ตรวจขนาดจริงซ้ำก่อนแตกไฟล์" : "ต้องตรวจพื้นที่ก่อนเริ่มดาวน์โหลด"}</p>
    {(storage.error || storage.plan?.warning) && <p role="alert" className="text-amber-300 break-words">{storage.error || storage.plan?.warning}</p>}
    {storage.plan?.options.filter(o => o.error).map(o => <p key={o.install_dir} className="text-amber-300 break-all">{o.error}</p>)}
    <div className="flex gap-2">
      <button type="button" disabled={disabled || storage.checking} onClick={() => void storage.refresh().catch(() => {})}
        className="p-1 border border-slate-600 rounded">ตรวจพื้นที่อีกครั้ง</button>
      <button type="button" disabled={disabled || storage.checking} onClick={() => void storage.browse()}
        className="p-1 border border-slate-600 rounded">เลือกโฟลเดอร์อื่น</button>
    </div>
  </div>;
}
