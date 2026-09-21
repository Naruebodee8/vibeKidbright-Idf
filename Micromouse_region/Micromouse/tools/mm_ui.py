#!/usr/bin/env python3
# =============================================================================
#  mm_ui.py — Micromouse BLE control panel  (one tool for ALL labs)
#
#  Just run:   python mm_ui.py
#  Then:  [Scan] → pick your robot's name → [Connect] → use the lab tab.
#
#  Each robot prints its unique BLE name on the USB Serial Monitor at boot
#  (e.g. "MM_Lab4_IR_3F2A9C") — note it so you pick the right one when scanning.
#
#  Talks to ble_debug.h (Nordic UART Service). Commands are the same single
#  letters you'd type on the Serial Monitor; results stream back to the Log.
#
#  Requires:  pip install bleak     (tkinter ships with Python)
# =============================================================================
import asyncio
import threading
import queue
import time
import math
import tkinter as tk
from tkinter import ttk, filedialog

try:
    from bleak import BleakClient, BleakScanner
except ImportError:
    raise SystemExit("Missing dependency. Install with:  pip install bleak")

NUS_RX = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"   # PC -> robot (write)
NUS_TX = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"   # robot -> PC (notify)
PREFIX = "MM"                                     # advertised-name prefix

# ── palette (modern dark) ────────────────────────────────────────────────────
BG="#0f1220"; CARD="#171b2e"; CARD2="#1e2440"; EDGE="#2b3357"
TXT="#dfe6f5"; DIM="#8b93b0"; ACC="#5b8cff"; OK="#37d39a"; WARN="#ffb454"; ERR="#ff5c7a"
GRID="#9aa3c7"

GRIDN=8; CELL=40


def setup_style(root):
    root.configure(bg=BG)
    s = ttk.Style(); s.theme_use("clam")
    s.configure(".", background=BG, foreground=TXT, fieldbackground=CARD2,
                bordercolor=EDGE, focuscolor=ACC, font=("Segoe UI", 10))
    s.configure("TFrame", background=BG)
    s.configure("Card.TFrame", background=CARD)
    s.configure("TLabel", background=BG, foreground=TXT)
    s.configure("Card.TLabel", background=CARD, foreground=TXT)
    s.configure("Dim.TLabel", background=CARD, foreground=DIM, font=("Segoe UI", 9))
    s.configure("TButton", background=CARD2, foreground=TXT, borderwidth=0, padding=(12, 7),
                font=("Segoe UI Semibold", 10))
    s.map("TButton", background=[("active", EDGE)], foreground=[("active", "#ffffff")])
    s.configure("Accent.TButton", background=ACC, foreground="#0b0e1a")
    s.map("Accent.TButton", background=[("active", "#7aa2ff")])
    s.configure("Good.TButton", background=OK, foreground="#062018")
    s.configure("Danger.TButton", background=ERR, foreground="#2a0410")
    s.configure("TEntry", fieldbackground=CARD2, foreground=TXT, insertcolor=TXT, padding=5)
    s.configure("TCombobox", fieldbackground=CARD2, foreground=TXT, background=CARD2, padding=5,
                arrowcolor=TXT)
    s.map("TCombobox", fieldbackground=[("readonly", CARD2)])
    s.configure("TNotebook", background=BG, borderwidth=0)
    s.configure("TNotebook.Tab", background=CARD, foreground=DIM, padding=(16, 8),
                font=("Segoe UI Semibold", 10))
    s.map("TNotebook.Tab", background=[("selected", CARD2)], foreground=[("selected", ACC)])
    s.configure("TLabelframe", background=CARD, foreground=ACC, bordercolor=EDGE)
    s.configure("TLabelframe.Label", background=CARD, foreground=ACC,
                font=("Segoe UI Semibold", 10))


# =============================================================================
#  BLE manager — persistent asyncio loop on a worker thread.
# =============================================================================
class BleManager:
    def __init__(self, rx_q, ev_q):
        self.rx_q = rx_q          # robot -> PC text lines
        self.ev_q = ev_q          # ("state", status, connected, name) / ("scan", [(name,addr)])
        self.loop = None
        self.client = None
        self._buf = ""

    def start(self):
        threading.Thread(target=self._run, daemon=True).start()

    def _run(self):
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)
        self.loop.run_forever()

    def _submit(self, coro):
        if self.loop:
            return asyncio.run_coroutine_threadsafe(coro, self.loop)

    # ── scan ──
    def scan(self):
        self.ev_q.put(("state", "scanning...", False, ""))
        self._submit(self._scan())

    async def _scan(self):
        try:
            devs = await BleakScanner.discover(timeout=5.0)
            found = sorted({(d.name, d.address) for d in devs
                            if d.name and d.name.startswith(PREFIX)})
            self.ev_q.put(("scan", list(found)))
            self.ev_q.put(("state", f"found {len(found)} robot(s)", False, ""))
        except Exception as e:
            self.ev_q.put(("scan", []))
            self.ev_q.put(("state", f"scan error: {e}", False, ""))

    # ── connect / disconnect ──
    def connect(self, address, name):
        self._submit(self._connect(address, name))

    async def _connect(self, address, name):
        await self._disconnect()
        self.ev_q.put(("state", f"connecting {name}...", False, name))
        try:
            def on_dc(_c):
                self.client = None
                self.ev_q.put(("state", "DISCONNECTED (link lost)", False, name))
            self.client = BleakClient(address, disconnected_callback=on_dc)
            await self.client.connect()
            self._buf = ""
            await self.client.start_notify(NUS_TX, self._notify)
            self.ev_q.put(("state", f"CONNECTED: {name}", True, name))
        except Exception as e:
            self.client = None
            self.ev_q.put(("state", f"connect failed: {e}", False, name))

    def disconnect(self):
        self._submit(self._disconnect())

    async def _disconnect(self):
        if self.client:
            try:
                await self.client.disconnect()
            except Exception:
                pass
        self.client = None
        self.ev_q.put(("state", "disconnected", False, ""))

    # ── send ──
    def send(self, cmd):
        self._submit(self._send(cmd))

    async def _send(self, cmd):
        if self.client and self.client.is_connected:
            try:
                await self.client.write_gatt_char(NUS_RX, (cmd + "\n").encode(), response=True)
            except Exception as e:
                self.ev_q.put(("state", f"write error: {e}", True, ""))

    def _notify(self, _char, data):
        self._buf += data.decode(errors="replace")
        while "\n" in self._buf:
            line, self._buf = self._buf.split("\n", 1)
            line = line.strip("\r")
            if line:
                self.rx_q.put(line)


# =============================================================================
#  MiniPlot — tiny tkinter-Canvas plotter (no matplotlib dependency)
#    series: list of dicts {pts:[(x,y),...], color, mode:'line'|'scatter', label}
# =============================================================================
class MiniPlot:
    PAL = ["#5cc8ff", "#ff7a7a", "#8be58b", "#e8d44d", "#c08bff", "#ff9d4d"]

    def __init__(self, parent, title, xlabel, ylabel, w=440, h=230):
        self.title, self.xlabel, self.ylabel = title, xlabel, ylabel
        self.w, self.h = w, h
        self.series = []
        self.c = tk.Canvas(parent, width=w, height=h, bg="#0a0d18", highlightthickness=0)

    def pack(self, **kw):
        self.c.pack(**kw); return self

    def set_series(self, series):
        self.series = series; self.redraw()

    def clear(self):
        self.series = []; self.redraw()

    def redraw(self):
        c = self.c; c.delete("all")
        ml, mr, mt, mb = 52, 12, 22, 30
        x0, y0, x1, y1 = ml, mt, self.w - mr, self.h - mb
        c.create_text(self.w / 2, 11, text=self.title, fill="#cfe",
                      font=("Segoe UI", 9, "bold"))
        pts = [p for s in self.series for p in s["pts"]]
        if not pts:
            c.create_text(self.w / 2, self.h / 2, text="no data — run the test",
                          fill="#456"); return
        xs = [p[0] for p in pts]; ys = [p[1] for p in pts]
        xmn, xmx = min(xs), max(xs); ymn, ymx = min(ys), max(ys)
        xmn = min(xmn, 0); ymn = min(ymn, 0)              # include origin
        if xmx - xmn < 1e-6: xmx = xmn + 1
        if ymx - ymn < 1e-6: ymx = ymn + 1
        xmx += (xmx - xmn) * 0.05; ymx += (ymx - ymn) * 0.08

        def px(x): return x0 + (x - xmn) / (xmx - xmn) * (x1 - x0)
        def py(y): return y1 - (y - ymn) / (ymx - ymn) * (y1 - y0)

        # grid + ticks
        for i in range(5):
            gx = xmn + (xmx - xmn) * i / 4
            X = px(gx); c.create_line(X, y0, X, y1, fill="#1c2438")
            c.create_text(X, y1 + 9, text=f"{gx:.0f}", fill="#789", font=("Consolas", 7))
            gy = ymn + (ymx - ymn) * i / 4
            Y = py(gy); c.create_line(x0, Y, x1, Y, fill="#1c2438")
            c.create_text(x0 - 5, Y, text=f"{gy:.0f}", fill="#789",
                          font=("Consolas", 7), anchor="e")
        c.create_rectangle(x0, y0, x1, y1, outline="#2b3550")
        c.create_text((x0 + x1) / 2, self.h - 4, text=self.xlabel, fill="#9ab",
                      font=("Segoe UI", 7))
        c.create_text(10, (y0 + y1) / 2, text=self.ylabel, fill="#9ab",
                      font=("Segoe UI", 7), angle=90)

        # series
        ly = y0 + 4
        for s in self.series:
            col = s.get("color", "#5cc8ff")
            sp = [(px(x), py(y)) for x, y in s["pts"]]
            if s.get("mode") == "scatter":
                for X, Y in sp:
                    c.create_oval(X - 3, Y - 3, X + 3, Y + 3, fill=col, outline="")
            else:
                if len(sp) >= 2:
                    flat = [v for xy in sp for v in xy]
                    c.create_line(*flat, fill=col, width=2)
            if s.get("label"):
                c.create_text(x1 - 6, ly, text=s["label"], fill=col,
                              font=("Consolas", 8), anchor="e"); ly += 13


# =============================================================================
#  App
# =============================================================================
class App:
    def __init__(self, root):
        self.root = root
        root.title("Micromouse — BLE Control Panel")
        root.geometry("1080x720")
        setup_style(root)

        self.rx_q = queue.Queue()
        self.ev_q = queue.Queue()
        self.ble = BleManager(self.rx_q, self.ev_q)
        self.ble.start()

        self.devices = {}          # name -> address
        self.connected = False
        self.pose = (0, 0, 0)
        self.walls = {}
        self.tvars = {}
        # Lab6 feedforward plot data
        self.ff_curves = {}        # V -> list of (t_ms, sL, sR)   (step-response curves)
        self.ff_points = []        # list of (V, sL, sR)           (steady-state points)
        self.ff_fit = None         # (ffsL, ffbL, ffsR, ffbR)      (fitted FF lines)

        self._build_conn_bar()
        self._build_bottom()      # persistent telemetry+log+cmd (packed bottom first)
        self._build_tabs()        # notebook fills the middle, above the bottom panel
        self.root.after(60, self._pump)

    # ── connection bar ──
    def _build_conn_bar(self):
        bar = ttk.Frame(self.root, style="Card.TFrame")
        bar.pack(fill="x", padx=10, pady=(10, 6))
        inner = ttk.Frame(bar, style="Card.TFrame"); inner.pack(fill="x", padx=10, pady=10)

        ttk.Button(inner, text="⟳  Scan", style="Accent.TButton",
                   command=self.do_scan).pack(side="left")
        self.dev_cb = ttk.Combobox(inner, state="readonly", width=26, values=[])
        self.dev_cb.pack(side="left", padx=8)
        self.btn_conn = ttk.Button(inner, text="Connect", style="Good.TButton", command=self.do_connect)
        self.btn_conn.pack(side="left", padx=2)
        self.btn_disc = ttk.Button(inner, text="Disconnect", style="Danger.TButton", command=self.do_disconnect)
        self.btn_disc.pack(side="left", padx=2)

        self.dot = tk.Canvas(inner, width=14, height=14, bg=CARD, highlightthickness=0)
        self.dot.pack(side="left", padx=(18, 6))
        self._dot = self.dot.create_oval(2, 2, 12, 12, fill=ERR, outline="")
        self.status = tk.StringVar(value="disconnected")
        ttk.Label(inner, textvariable=self.status, style="Card.TLabel",
                  font=("Segoe UI Semibold", 10)).pack(side="left")

        ttk.Label(inner, text="ดูชื่อหุ่นของคุณบน Serial Monitor ตอนเปิดเครื่อง",
                  style="Dim.TLabel").pack(side="right")

    def _set_state(self, status, connected, name):
        self.status.set(status)
        self.connected = connected
        col = OK if connected else (WARN if ("scan" in status or "connect" in status) else ERR)
        self.dot.itemconfig(self._dot, fill=col)

    # ── tabs ──
    def _scroll_tab(self, nb, title):
        # a vertically-scrollable tab page (so long tabs fit small screens)
        outer = ttk.Frame(nb); nb.add(outer, text=title)
        canvas = tk.Canvas(outer, bg=BG, highlightthickness=0)
        vsb = ttk.Scrollbar(outer, orient="vertical", command=canvas.yview)
        inner = ttk.Frame(canvas)
        win = canvas.create_window((0, 0), window=inner, anchor="nw")
        canvas.configure(yscrollcommand=vsb.set)
        canvas.pack(side="left", fill="both", expand=True)
        vsb.pack(side="right", fill="y")
        inner.bind("<Configure>", lambda e: canvas.configure(scrollregion=canvas.bbox("all")))
        canvas.bind("<Configure>", lambda e: canvas.itemconfig(win, width=e.width))
        # mouse wheel scrolls only while the pointer is over this page
        def _wheel(e): canvas.yview_scroll(int(-1 * (e.delta / 120)), "units")
        canvas.bind("<Enter>", lambda e: canvas.bind_all("<MouseWheel>", _wheel))
        canvas.bind("<Leave>", lambda e: canvas.unbind_all("<MouseWheel>"))
        return inner

    def _build_tabs(self):
        nb = ttk.Notebook(self.root)
        nb.pack(fill="both", expand=True, padx=10, pady=6)

        self._build_lab1(self._scroll_tab(nb, "  Lab1 Motor  "))
        self._build_lab2(self._scroll_tab(nb, "  Lab2 Encoder  "))
        self._build_lab3(self._scroll_tab(nb, "  Lab3 IMU  "))
        self._build_lab4(self._scroll_tab(nb, "  Lab4 IR  "))
        self._build_lab5_ff(self._scroll_tab(nb, "  Lab5 FF  "))
        self._build_lab6_pid(self._scroll_tab(nb, "  Lab6 PID  "))
        self._build_integration(self._scroll_tab(nb, "  Integration  "))

    def _card(self, parent, title):
        f = ttk.Labelframe(parent, text=" " + title + " ")
        f.pack(fill="x", padx=12, pady=8, ipady=4)
        return f

    def _cmdbtn(self, parent, label, cmd, style="TButton"):
        # show the keyboard command distinctly as a [key] badge so it doesn't
        # read as part of the label text
        b = ttk.Button(parent, text="{}   [{}]".format(label, cmd),
                       style=style, command=lambda: self.send(cmd))
        b.pack(side="left", padx=4, pady=4)
        return b

    # ── persistent bottom panel: telemetry + log + cmd  (ALWAYS visible) ──
    #    เห็นผลตอบกลับได้ทุกแท็บ ไม่ต้องสลับกลับมาหน้า Console
    def _build_bottom(self):
        bot = ttk.Frame(self.root)
        bot.pack(side="bottom", fill="x", padx=10, pady=(0, 10))

        tg = ttk.Labelframe(bot, text=" Live telemetry "); tg.pack(fill="x")
        row = ttk.Frame(tg, style="Card.TFrame"); row.pack(fill="x", padx=6, pady=6)
        for k, lab in [("fwd", "fwd mm"), ("hd", "heading"), ("cte", "CTE mm"),
                       ("left", "L mm"), ("right", "R mm"), ("vbat", "batt V"), ("armed", "armed")]:
            cell = ttk.Frame(row, style="Card.TFrame"); cell.pack(side="left", expand=True, fill="x")
            ttk.Label(cell, text=lab, style="Dim.TLabel").pack()
            v = tk.StringVar(value="-"); self.tvars[k] = v
            ttk.Label(cell, textvariable=v, style="Card.TLabel",
                      font=("Consolas", 13, "bold")).pack()

        lg = ttk.Labelframe(bot, text=" Log / response "); lg.pack(fill="x", pady=(6, 0))
        bar = ttk.Frame(lg, style="Card.TFrame"); bar.pack(fill="x", padx=6, pady=(6, 0))
        ttk.Button(bar, text="save", command=self.save_log).pack(side="left", padx=2)
        ttk.Button(bar, text="clear", command=lambda: self.logbox.delete("1.0", "end")).pack(side="left")
        ttk.Button(bar, text="Help   [?]", command=lambda: self.send("?")).pack(side="left", padx=(12, 2))
        ttk.Button(bar, text="telemetry   [t]", command=lambda: self.send("t")).pack(side="left", padx=2)
        ttk.Button(bar, text="ABORT   [x]", style="Danger.TButton",
                   command=lambda: self.send("x")).pack(side="left", padx=2)
        self.logbox = tk.Text(lg, height=10, bg="#0a0d18", fg="#cfe", insertbackground="#cfe",
                              relief="flat", font=("Consolas", 10))
        self.logbox.pack(fill="x", padx=6, pady=6)

        ef = ttk.Frame(bot); ef.pack(fill="x", pady=(6, 0))
        ttk.Label(ef, text="cmd ▸").pack(side="left")
        self.entry = ttk.Entry(ef); self.entry.pack(side="left", fill="x", expand=True, padx=6)
        self.entry.bind("<Return>", lambda e: (self.send(self.entry.get()), self.entry.delete(0, "end")))
        ttk.Button(ef, text="Send", style="Accent.TButton",
                   command=lambda: (self.send(self.entry.get()), self.entry.delete(0, "end"))).pack(side="left")

    # ── Lab1 Motor ──
    def _build_lab1(self, p):
        d = self._card(p, "ทิศทาง / ขับ")
        self._cmdbtn(d, "PWM +", "w"); self._cmdbtn(d, "PWM −", "s")
        self._cmdbtn(d, "Forward", "f"); self._cmdbtn(d, "Back", "b")
        self._cmdbtn(d, "Left", "a"); self._cmdbtn(d, "Right", "d")
        self._cmdbtn(d, "STOP", "x", "Danger.TButton")
        m = self._card(p, "เช็คทีละข้าง / หาค่า")
        self._cmdbtn(m, "Left only", "l"); self._cmdbtn(m, "Right only", "r")
        self._cmdbtn(m, "Min PWM + deadband_V", "m", "Accent.TButton")
        self._cmdbtn(m, "Status", "p")
        b = self._card(p, "Battery (โมเดลคุมเป็นโวลต์)")
        self._cmdbtn(b, "Read Vbat", "v", "Accent.TButton")
        self._cmdbtn(b, "Calibrate ratio", "c")
        h = self._card(p, "เช็ค HW หลังบัดกรี")
        self._cmdbtn(h, "Test START / MODE buttons", "i", "Accent.TButton")

    # ── Lab2 Encoder ──
    def _build_lab2(self, p):
        # STEP 1 — find CPR by spinning a wheel by hand
        s1 = self._card(p, "STEP 1 — หาค่า CPR: กด Reset แล้วหมุนล้อด้วยมือ 1 รอบ → อ่าน count = CPR")
        self._cmdbtn(s1, "Reset", "r", "Accent.TButton")
        self._cmdbtn(s1, "Live counts ON/OFF", "s")
        self._cmdbtn(s1, "Read once", "c")
        # live counts (raw) — updated from the firmware's "E,L,R" stream
        cf = self._card(p, "live counts (raw encoder)")
        row = ttk.Frame(cf, style="Card.TFrame"); row.pack(fill="x", padx=6, pady=4)
        self.enc_l = tk.StringVar(value="----"); self.enc_r = tk.StringVar(value="----")
        for lab, var in [("L", self.enc_l), ("R", self.enc_r)]:
            cell = ttk.Frame(row, style="Card.TFrame"); cell.pack(side="left", expand=True, fill="x")
            ttk.Label(cell, text=lab, style="Dim.TLabel").pack()
            ttk.Label(cell, textvariable=var, style="Card.TLabel",
                      font=("Consolas", 22, "bold")).pack()
        # client-side CALC: diameter + CPR -> mm/count (for config)
        cc = self._card(p, "CALC → ค่าที่ใส่ config")
        ttk.Label(cc, text="⌀ ล้อ (mm)").pack(side="left", padx=(8, 2))
        self.l2_dia = ttk.Entry(cc, width=6); self.l2_dia.insert(0, "34.0"); self.l2_dia.pack(side="left")
        ttk.Label(cc, text="CPR ที่วัดได้").pack(side="left", padx=(10, 2))
        self.l2_cpr = ttk.Entry(cc, width=7); self.l2_cpr.pack(side="left")
        ttk.Button(cc, text="CALC", style="Accent.TButton", command=self._calc_cpr).pack(side="left", padx=10)
        self.l2_calc = tk.StringVar(value="")
        ttk.Label(p, textvariable=self.l2_calc, style="Dim.TLabel", justify="left").pack(anchor="w", padx=16)
        # STEP 2 — verify the CPR (two drive methods); measure with a ruler
        s2 = self._card(p, "STEP 2 — ทดสอบ CPR (หลังใส่ค่าใน config + โหลดโปรแกรมใหม่ก่อน) แล้ววัดด้วยไม้บรรทัด/ตลับเมตร")
        e1 = ttk.Entry(s2, width=6); e1.pack(side="left", padx=(6, 2))
        ttk.Button(s2, text="วิธี 1: กำหนดระยะวิ่ง (mm)",
                   command=lambda: self.send("drive " + e1.get().strip())).pack(side="left", padx=4)
        ttk.Label(s2, text="    ", style="Dim.TLabel").pack(side="left")
        e2 = ttk.Entry(s2, width=6); e2.pack(side="left", padx=(6, 2))
        ttk.Button(s2, text="วิธี 2: ลองค่า CPR ที่ต้องการ",
                   command=lambda: self.send("cpr " + e2.get().strip())).pack(side="left", padx=4)

    def _calc_cpr(self):
        try:
            d = float(self.l2_dia.get()); c = float(self.l2_cpr.get())
            if c <= 0:
                raise ValueError
            circ = math.pi * d
            mpc = circ / c
            self.l2_calc.set(
                "เส้นรอบวง = %.2f mm   ·   mm/count = %.5f\n"
                "config.h:   #define COUNTS_PER_REV  %.0f      #define MM_PER_COUNT  %.5f"
                % (circ, mpc, c, mpc))
        except (ValueError, ZeroDivisionError):
            self.l2_calc.set("กรอก ⌀ ล้อ และ CPR เป็นตัวเลข (CPR > 0)")

    # ── Lab3 IMU ──
    def _build_lab3(self, p):
        c = self._card(p, "Gyro calibration (keep robot still)")
        self._cmdbtn(c, "Calibrate gyro", "c", "Accent.TButton")
        self._cmdbtn(c, "Reset heading", "r")
        self._cmdbtn(c, "Toggle display", "s")
        self._cmdbtn(c, "Status", "p")

    # ── Lab4 IR ──
    def _build_lab4(self, p):
        c = self._card(p, "wall sensors")
        self._cmdbtn(c, "Continuous", "s", "Accent.TButton")
        self._cmdbtn(c, "Read once", "r")
        self._cmdbtn(c, "Raw mV", "v")
        self._cmdbtn(c, "Threshold", "t")
        self._cmdbtn(c, "LUT", "l")

    # ── Lab6 PID (control) ──
    def _build_lab6_pid(self, p):
        c = self._card(p, "control test")
        self._cmdbtn(c, "A/B compare", "c", "Accent.TButton")
        self._cmdbtn(c, "FF + PD", "f")
        self._cmdbtn(c, "FF only", "g")
        self._cmdbtn(c, "Dump CSV", "d")
        self._cmdbtn(c, "Show gains", "s")
        dd = self._card(p, "ระยะวิ่งทดสอบ")
        de = ttk.Entry(dd, width=8); de.pack(side="left", padx=6, pady=4)
        ttk.Button(dd, text="set dist (mm)",
                   command=lambda: self.send("dist " + de.get().strip())).pack(side="left", padx=4)
        t = self._card(p, "tune FWD gain (+/-)")
        for name, up, dn in [("Kp", "p", "o"), ("Kd", "k", "m"), ("Ki", "i", "j")]:
            rowf = ttk.Frame(t, style="Card.TFrame"); rowf.pack(side="left", padx=12, pady=4)
            ttk.Label(rowf, text=name, style="Card.TLabel",
                      font=("Segoe UI Semibold", 11)).pack()
            br = ttk.Frame(rowf, style="Card.TFrame"); br.pack()
            ttk.Button(br, text="−", width=3, command=lambda d=dn: self.send(d)).pack(side="left", padx=2)
            ttk.Button(br, text="+", width=3, command=lambda u=up: self.send(u)).pack(side="left", padx=2)
        rt = self._card(p, "rotation test (ROT PID)")
        ttk.Button(rt, text="Left 90", command=lambda: self.send("turn 90")).pack(side="left", padx=4)
        ttk.Button(rt, text="Right 90", command=lambda: self.send("turn -90")).pack(side="left", padx=4)
        ttk.Button(rt, text="U-turn 180", command=lambda: self.send("turn 180")).pack(side="left", padx=4)
        rg = self._card(p, "ROT gains (set value)")
        for key in ["rkp", "rkd", "rki"]:
            self._setrow(rg, key)
        gc = self._card(p, "คำนวณ FWD_KP/KD จากค่า Lab6 (gaincalc)")
        self.gc_vars = {}
        for lab, dflt in [("Km", "270"), ("Tm", "0.12"), ("zeta", "1.0"), ("TD", "0.12")]:
            ttk.Label(gc, text=lab).pack(side="left", padx=(8, 2))
            e = ttk.Entry(gc, width=6); e.insert(0, dflt); e.pack(side="left")
            self.gc_vars[lab] = e
        ttk.Button(gc, text="calc + apply", style="Accent.TButton",
                   command=self._send_gaincalc).pack(side="left", padx=10)

    # ── Lab5 Feedforward ──
    def _build_lab5_ff(self, p):
        i = self._card(p, "Feedforward step-response (★ บนพื้น ~0.5m, หุ่นวิ่งสลับหน้า-หลัง)")
        self._cmdbtn(i, "Run sweep", "r", "Accent.TButton")
        self._cmdbtn(i, "Read Vbat", "v")
        self._cmdbtn(i, "STOP", "x", "Danger.TButton")
        ttk.Button(i, text="clear plot", command=self._clear_ff).pack(side="left", padx=10)
        s = ttk.Frame(i, style="Card.TFrame"); s.pack(side="left", padx=10)
        se = ttk.Entry(s, width=6); se.pack(side="left", padx=4)
        ttk.Button(s, text="step ff <V>",
                   command=lambda: self.send("ff " + se.get().strip())).pack(side="left")
        # graphs (like the UKMARS slides)
        pf = ttk.Frame(p); pf.pack(fill="both", expand=True, padx=12, pady=6)
        self.plot_step = MiniPlot(pf, "Step response (left wheel)",
                                  "time (ms)", "speed mm/s").pack(side="left", padx=4)
        self.plot_fv = MiniPlot(pf, "Motor Volts vs Speed  (slope=FF_SPEED, intercept=FF_BIAS)",
                                "speed mm/s", "motor V").pack(side="left", padx=4)

    def _clear_ff(self):
        self.ff_curves = {}; self.ff_points = []; self.ff_fit = None; self._redraw_ff()

    def _redraw_ff(self):
        if not hasattr(self, "plot_step"):
            return
        # step-response: one line per voltage (left wheel), color by index
        step = []
        for idx, V in enumerate(sorted(self.ff_curves)):
            col = MiniPlot.PAL[idx % len(MiniPlot.PAL)]
            step.append({"pts": [(t, sL) for (t, sL, sR) in self.ff_curves[V]],
                         "color": col, "mode": "line", "label": f"{V:.0f}V"})
        self.plot_step.set_series(step)
        # Volts-vs-Speed: scatter L/R + fitted lines
        fv = []
        if self.ff_points:
            fv.append({"pts": [(sL, V) for (V, sL, sR) in self.ff_points],
                       "color": "#5cc8ff", "mode": "scatter", "label": "L"})
            fv.append({"pts": [(sR, V) for (V, sL, sR) in self.ff_points],
                       "color": "#ff7a7a", "mode": "scatter", "label": "R"})
        if self.ff_fit and self.ff_points:
            ffsL, ffbL, ffsR, ffbR = self.ff_fit
            smax = max(max(sL, sR) for (V, sL, sR) in self.ff_points)
            fv.append({"pts": [(0, ffbL), (smax, ffsL * smax + ffbL)],
                       "color": "#5cc8ff", "mode": "line"})
            fv.append({"pts": [(0, ffbR), (smax, ffsR * smax + ffbR)],
                       "color": "#ff7a7a", "mode": "line"})
        self.plot_fv.set_series(fv)

    def _send_gaincalc(self):
        v = self.gc_vars
        self.send("gaincalc {} {} {} {}".format(
            v["Km"].get().strip(), v["Tm"].get().strip(),
            v["zeta"].get().strip(), v["TD"].get().strip()))

    # ── Integration ──
    def _build_integration(self, p):
        left = ttk.Frame(p); left.pack(side="left", fill="y", padx=10, pady=10)
        ttk.Label(left, text="Maze (dead-reckoning)", style="TLabel").pack(anchor="w")
        self.canvas = tk.Canvas(left, width=GRIDN * CELL + 2, height=GRIDN * CELL + 2,
                                bg="#0a0d18", highlightthickness=0)
        self.canvas.pack()
        self._draw_maze()

        right = ttk.Frame(p); right.pack(side="left", fill="both", expand=True, padx=6, pady=10)
        run = ttk.Labelframe(right, text=" Run "); run.pack(fill="x")
        rr = ttk.Frame(run, style="Card.TFrame"); rr.pack(fill="x", padx=6, pady=8)
        ttk.Button(rr, text="START  g", style="Good.TButton", command=lambda: self.send("g")).pack(side="left", padx=4)
        ttk.Button(rr, text="ABORT   [x]", style="Danger.TButton", command=lambda: self.send("x")).pack(side="left", padx=4)
        ttk.Button(rr, text="telemetry   [t]", command=lambda: self.send("t")).pack(side="left", padx=4)

        tune = ttk.Labelframe(right, text=" Steering / motion tuning "); tune.pack(fill="x", pady=8)
        for key in ["kp", "kd", "nom", "kick", "kickms", "kickrel", "tdps"]:
            self._setrow(tune, key)
        tgg = ttk.Frame(tune, style="Card.TFrame"); tgg.pack(fill="x", padx=6, pady=4)
        ttk.Label(tgg, text="front-wall:", style="Card.TLabel").pack(side="left")
        ttk.Button(tgg, text="on", command=lambda: self.send("fw 1")).pack(side="left", padx=2)
        ttk.Button(tgg, text="off", command=lambda: self.send("fw 0")).pack(side="left", padx=2)
        ttk.Label(tgg, text="   settle:", style="Card.TLabel").pack(side="left")
        ttk.Button(tgg, text="on", command=lambda: self.send("settle 1")).pack(side="left", padx=2)
        ttk.Button(tgg, text="off", command=lambda: self.send("settle 0")).pack(side="left", padx=2)

    # ── small helpers ──
    def _numrow(self, parent, label, cmd_char, btn, style="TButton"):
        wrap = self._card(parent, label)
        e = ttk.Entry(wrap, width=10); e.pack(side="left", padx=6, pady=4)
        ttk.Button(wrap, text=btn, style=style,
                   command=lambda: self._send_num(cmd_char, e)).pack(side="left", padx=4)

    def _send_num(self, cmd_char, entry):
        val = entry.get().strip()
        self.send(cmd_char)              # firmware enters number-prompt mode
        if val:
            self.root.after(120, lambda: self.send(val))

    def _setrow(self, parent, key):
        f = ttk.Frame(parent, style="Card.TFrame"); f.pack(fill="x", padx=6, pady=2)
        ttk.Label(f, text=key, style="Card.TLabel", width=8).pack(side="left")
        e = ttk.Entry(f, width=10); e.pack(side="left", padx=4)
        ttk.Button(f, text="set", command=lambda: self.send(f"{key} {e.get().strip()}")).pack(side="left")

    # ── actions ──
    def do_scan(self):
        self.dev_cb.set("")
        self.ble.scan()

    def do_connect(self):
        name = self.dev_cb.get()
        if name and name in self.devices:
            self.ble.connect(self.devices[name], name)

    def do_disconnect(self):
        self.ble.disconnect()

    def send(self, cmd):
        cmd = (cmd or "").strip()
        if not cmd:
            return
        if not self.connected:
            self.log("[not connected] " + cmd)
            return
        self.ble.send(cmd)
        self.log("» " + cmd)

    def log(self, s):
        self.logbox.insert("end", s + "\n")
        self.logbox.see("end")

    def save_log(self):
        path = filedialog.asksaveasfilename(defaultextension=".txt",
                                            initialfile=f"mm_log_{int(time.time())}.txt")
        if path:
            with open(path, "w", encoding="utf-8") as f:
                f.write(self.logbox.get("1.0", "end"))
            self.status.set("log saved")

    # ── event pump (tkinter thread) ──
    def _pump(self):
        try:
            while True:
                ev = self.ev_q.get_nowait()
                if ev[0] == "state":
                    self._set_state(ev[1], ev[2], ev[3])
                elif ev[0] == "scan":
                    self.devices = {n: a for (n, a) in ev[1] if n}
                    names = list(self.devices.keys())
                    self.dev_cb["values"] = names
                    if names:
                        self.dev_cb.set(names[0])
        except queue.Empty:
            pass
        try:
            while True:
                self._handle_line(self.rx_q.get_nowait())
        except queue.Empty:
            pass
        self.root.after(60, self._pump)

    def _handle_line(self, line):
        if line.startswith("T,"):
            f = line[2:].split(",")
            for k, v in zip(["fwd", "hd", "cte", "left", "right", "vbat", "armed"], f):
                if k in self.tvars:
                    self.tvars[k].set(v)
        elif line.startswith("P,"):
            try:
                x, y, h = (int(v) for v in line[2:].split(",")[:3])
                self.pose = (x, y, h); self._draw_maze()
            except ValueError:
                pass
        elif line.startswith("W,"):
            try:
                x, y, m = (int(v) for v in line[2:].split(",")[:3])
                self.walls[(x, y)] = m; self._draw_maze()
            except ValueError:
                pass
        elif line.startswith("E,"):          # Lab2 live encoder counts (not logged)
            try:
                l, r = line[2:].split(",")[:2]
                if hasattr(self, "enc_l"):
                    self.enc_l.set(l.strip()); self.enc_r.set(r.strip())
            except ValueError:
                pass
        elif line.startswith("S,"):          # Lab6 step-response sample (not logged)
            try:
                if self.ff_fit is not None:  # a new run started → clear previous
                    self.ff_curves = {}; self.ff_points = []; self.ff_fit = None
                V, t, sL, sR = (float(x) for x in line[2:].split(",")[:4])
                self.ff_curves.setdefault(V, []).append((t, sL, sR))
            except ValueError:
                pass
        elif line.startswith("FP,"):         # Lab6 steady-state point
            try:
                if self.ff_fit is not None:
                    self.ff_curves = {}; self.ff_points = []; self.ff_fit = None
                V, sL, sR = (float(x) for x in line[3:].split(",")[:3])
                self.ff_points.append((V, sL, sR)); self._redraw_ff()
            except ValueError:
                pass
        elif line.startswith("FIT,"):        # Lab6 fitted FF lines
            try:
                self.ff_fit = tuple(float(x) for x in line[4:].split(",")[:4])
                self._redraw_ff()
            except ValueError:
                pass
            self.log(line)
        else:
            self.log(line)

    # ── maze drawing ──
    def _draw_maze(self):
        c = self.canvas
        c.delete("all")
        n = GRIDN
        for gx in range(n):
            for gy in range(n):
                px = gx * CELL + 1; py = (n - 1 - gy) * CELL + 1
                c.create_rectangle(px, py, px + CELL, py + CELL, outline="#222a45", fill="#11152a")
                m = self.walls.get((gx, gy), 0)
                if m & 1: c.create_line(px, py, px + CELL, py, fill=GRID, width=3)
                if m & 2: c.create_line(px + CELL, py, px + CELL, py + CELL, fill=GRID, width=3)
                if m & 4: c.create_line(px, py + CELL, px + CELL, py + CELL, fill=GRID, width=3)
                if m & 8: c.create_line(px, py, px, py + CELL, fill=GRID, width=3)
        x, y, h = self.pose
        if 0 <= x < n and 0 <= y < n:
            cx = x * CELL + 1 + CELL / 2; cy = (n - 1 - y) * CELL + 1 + CELL / 2
            r = CELL * 0.3
            c.create_oval(cx - r, cy - r, cx + r, cy + r, fill=ACC, outline="")
            dx, dy = [(0, -1), (1, 0), (0, 1), (-1, 0)][h % 4]
            c.create_line(cx, cy, cx + dx * r * 1.7, cy + dy * r * 1.7, fill="#fff", width=3)


def main():
    root = tk.Tk()
    App(root)
    root.mainloop()


if __name__ == "__main__":
    main()
