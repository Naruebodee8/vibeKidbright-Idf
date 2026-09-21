# Micromouse — ภาพรวมการทำงาน (Flowchart + Sequence)

เปิดดู Mermaid ได้ใน VS Code (ext. Markdown Preview Mermaid), GitHub, หรือ https://mermaid.live

---

## 0. สถาปัตยกรรม 4 ชั้น (ใครเรียกใคร)

```mermaid
flowchart TD
  subgraph L3["L3 — WallFollower.ino (นักเรียนแก้)"]
    A1["run loop: sense → decide → execute"]
    A2["decideMove (left/right-hand)"]
    A3["pose: posX,posY,heading"]
  end
  subgraph L2["L2 — Micromouse facade (library)"]
    B1["moveCell / turn90 / turnBack"]
    B2["wallFront/Left/Right, wallDistances"]
    B3["_forward / _spin / _settleHeading"]
    B4["waitForStart / begin"]
  end
  subgraph L1["L1 — config.h (ค่าจูน + พิน)"]
    C1["CELL_SIZE, thresholds, FF/PD gains, kick"]
  end
  subgraph L0["L0 — HAL + control loop (ซ่อน)"]
    D1["controlTask 500Hz: profile + FF+PD"]
    D2["steering (CTE), front_wall"]
    D3["hal_motor/encoder/imu/wall_sensor"]
    D4["ble_debug (NUS)"]
  end
  L3 --> L2 --> L0
  L2 -. อ่านค่า .-> L1
  L0 -. อ่านค่า .-> L1
```

---

## 1. Flowchart — ลูปหลัก (loop + run)

```mermaid
flowchart TD
  A["setup(): mouse.begin()"] --> B["loop()"]
  B --> C["waitForStart()<br/>block จน START/'g'<br/>→ encoder_reset + control_arm + heading=0"]
  C --> D["reset pose: posX=posY=heading=0"]
  D --> E["run(): step=0"]
  E --> F{"pos == GOAL (3,2)?"}
  F -- "ใช่" --> G["log 'GOAL reached!'<br/>(T1) return"]
  F -- "ไม่" --> H{"step>0 และ pos==(0,0)?"}
  H -- "ใช่" --> I["log 'returned to START'<br/>(T2) return"]
  H -- "ไม่" --> J["reportPose → UI"]
  J --> K["sense:<br/>wL=wallLeft()<br/>wF=wallFront()<br/>wR=wallRight()"]
  K --> L["act = decideMove(wL,wF,wR)"]
  L --> LOG["logf STEP (x,y) + mm + act"]
  LOG --> M{"act?"}
  M -- "1: ซ้าย" --> N["turn90(LEFT)"]
  M -- "2: ขวา" --> O["turn90(RIGHT)"]
  M -- "3: กลับหลัง" --> P["turnBack()"]
  M -- "0: ตรง" --> Q["moveCell(1)"]
  N --> Q
  O --> Q
  P --> Q
  Q --> R{"สำเร็จ? (ไม่ abort/stall)"}
  R -- "ไม่" --> S["log 'aborted/stalled'<br/>return"]
  R -- "ใช่" --> T["update pose<br/>posX+=DX[heading], posY+=DY[heading]"]
  T --> U{"step < MAX_STEPS (100)?"}
  U -- "ใช่" --> E2["step++"]
  E2 --> F
  U -- "ไม่" --> V["log 'MAX_STEPS' (T0) return"]
  G --> B
  I --> B
  S --> B
  V --> B
```

---

## 2. Flowchart — decideMove (Left-hand rule)

```mermaid
flowchart TD
  A["decideMove(wL,wF,wR)<br/>USE_LEFT_HAND=true"] --> B{"กำแพงซ้าย?"}
  B -- "ไม่มี" --> C["เลี้ยวซ้าย แล้วเดิน (act=1)"]
  B -- "มี" --> D{"กำแพงหน้า?"}
  D -- "ไม่มี" --> E["เดินตรง (act=0)"]
  D -- "มี" --> F{"กำแพงขวา?"}
  F -- "ไม่มี" --> G["เลี้ยวขวา แล้วเดิน (act=2)"]
  F -- "มี" --> H["ตันสามด้าน → กลับหลัง (act=3)"]
```

> หมายเหตุ: `wallRight()` ที่อ่านเพี้ยน (false wall) ทำให้สาขานี้เลือกผิด → พลาดทางไป goal

---

## 3. Sequence — เดินหน้า 1 ช่อง (moveCell) + control loop ที่รันคู่ขนาน

```mermaid
sequenceDiagram
  autonumber
  participant S as L3 (run loop)
  participant M as Micromouse (facade)
  participant C as controlTask (500Hz core1)
  participant FW as front_wall / steering
  participant HW as HAL (motor/enc/imu/IR)

  Note over C,HW: controlTask วนตลอด @500Hz (อิสระจาก L3)
  loop ทุก 2 ms
    C->>HW: encoder_update + imu_update
    C->>C: forward/rotation profile.update()
    C->>C: fwd_change(enc), rot_change(gyro), heading += rot
    C->>HW: motors_update(FF+PD → PWM)
  end

  S->>M: moveCell(1)
  M->>C: control_arm() (ถ้ายังไม่ arm)
  M->>FW: steering_enable(true) + heading_hold(target)
  M->>C: control_start_forward(CELL_SIZE,...)
  loop จน profile เดินจบ
    M->>HW: อ่าน IR ซ้าย/ขวา
    M->>FW: steering_update → CTE adj
    M->>C: control_set_steering(adj)
    M-->>S: BLE "T,..." telemetry (~20Hz)
  end
  M->>HW: fw_wall_present()?
  alt มีกำแพงหน้า
    M->>FW: fw_correct() (ตั้งหัว + รีเซ็ตระยะกับกำแพง)
  end
  M-->>S: BLE "D,F,...err" diagnostic
  M-->>S: return true (หรือ false=abort)
  S->>S: update pose
```

---

## 4. Sequence — หมุน 90/180 (turn90 / turnBack) + settle

```mermaid
sequenceDiagram
  autonumber
  participant S as L3
  participant M as Micromouse (facade)
  participant C as controlTask (500Hz)
  participant HW as HAL

  S->>M: turn90(LEFT)  // หรือ RIGHT / turnBack
  M->>M: _targetHeading += angle
  M->>C: control_start_rotation(angle, turnDps,...)
  M->>C: control_kick_rotation(kickPwm, kickMs, kickRel)
  Note right of C: kick ดันเต็มแรงเบรก stiction<br/>แล้ว profile+FF+PD คุมต่อ
  loop จนหมุนจบ
    M-->>S: BLE telemetry
  end
  M->>M: stall check (act < 50% ของ angle → abort)
  alt ไม่ stall และ settle เปิด
    M->>C: _settleHeading() — direct PWM หมุนแก้จน heading=target ±3°
  end
  M-->>S: BLE "D,L/R/U,...turn_err,set_err"
  M-->>S: return true/false
```

---

## 5. Sequence — control task หนึ่ง tick (หัวใจ 500Hz)

```mermaid
sequenceDiagram
  autonumber
  participant T as controlTask tick
  participant E as encoder
  participant I as imu (gyro)
  participant P as profiles
  participant MC as motors_controller
  participant MO as hal_motor

  T->>E: encoder_update()
  T->>I: imu_update()
  T->>P: forward.update() + rotation.update()
  T->>T: fwd_change = Δ encoder ; rot_change = gyroZ·dt
  T->>T: heading += rot_change
  T->>T: battery critical? → disarm
  T->>MC: motors_update(v, ω, fwd_change, rot_change, steering)
  MC->>MC: PD on position error + per-wheel FF + battery comp
  MC->>MO: motor_set_speed(L_pwm, R_pwm)
  T->>T: snapshot telemetry (fwd, heading, ...)
```

---

## สรุปจุดที่กำลัง debug (ณ ปัจจุบัน)

| ส่วน | สถานะ |
|---|---|
| 500Hz control + FF+PD (เดินตรง) | ✅ ดี (err ~±1°) |
| heading-hold + _settleHeading (หลังเทิร์น) | ✅ ทำงาน (set_err ~±5°) เมื่อเทิร์นไม่ stall |
| **wallRight() sensing** | ❌ อ่านเพี้ยน (37-91mm ที่ช่องเปิด) → เลือกทางผิด → ไม่ถึง goal |
| **turn stall** | ⚠️ ครั้งเว้นครั้ง (แรงบิด/แบต) |
| pose tracking + termination (T0/T1/T2) | ✅ ถูกต้อง |
