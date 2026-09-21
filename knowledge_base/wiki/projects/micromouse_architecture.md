# Micromouse — Architecture & Flowcharts

> **Raw source**: `raw/micromouse_architecture.md`
> **Related**: [[micromouse]] · [[micromouse_ble]]

---

## 4-Layer Architecture

```mermaid
flowchart TD
  subgraph L3["L3 — WallFollower.ino (นักเรียนแก้)"]
    A1["run loop: sense → decide → execute"]
    A2["decideMove (left/right-hand rule)"]
    A3["pose: posX, posY, heading"]
  end
  subgraph L2["L2 — Micromouse facade (library)"]
    B1["moveCell / turn90 / turnBack"]
    B2["wallFront/Left/Right, wallDistances"]
    B3["_forward / _spin / _settleHeading"]
  end
  subgraph L1["L1 — config.h (ค่าจูน + pins)"]
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

## Flowchart — ลูปหลัก (loop + run)

```mermaid
flowchart TD
  A["setup(): mouse.begin()"] --> B["loop()"]
  B --> C["waitForStart() — block จน START/'g'"]
  C --> D["reset pose: posX=posY=heading=0"]
  D --> E["run(): step=0"]
  E --> F{"pos == GOAL?"}
  F -- "ใช่" --> G["GOAL reached! (T1) return"]
  F -- "ไม่" --> H{"step>0 และ pos==(0,0)?"}
  H -- "ใช่" --> I["returned to START (T2) return"]
  H -- "ไม่" --> J["sense: wL/wF/wR"]
  J --> K["decideMove(wL,wF,wR)"]
  K --> M{"act?"}
  M -- "เลี้ยวซ้าย" --> N["turn90(LEFT) → moveCell"]
  M -- "เลี้ยวขวา" --> O["turn90(RIGHT) → moveCell"]
  M -- "กลับหลัง" --> P["turnBack() → moveCell"]
  M -- "ตรง" --> Q["moveCell(1)"]
  Q --> R{"สำเร็จ?"}
  R -- "ไม่" --> S["aborted/stalled → return"]
  R -- "ใช่" --> T["update pose"]
  T --> U{"step < MAX_STEPS?"}
  U -- "ใช่" --> E
  U -- "ไม่" --> V["MAX_STEPS (T0) return"]
```

---

## Flowchart — decideMove (Left-hand Rule)

```mermaid
flowchart TD
  A["decideMove(wL,wF,wR)"] --> B{"กำแพงซ้าย?"}
  B -- "ไม่มี" --> C["เลี้ยวซ้าย (act=1)"]
  B -- "มี" --> D{"กำแพงหน้า?"}
  D -- "ไม่มี" --> E["เดินตรง (act=0)"]
  D -- "มี" --> F{"กำแพงขวา?"}
  F -- "ไม่มี" --> G["เลี้ยวขวา (act=2)"]
  F -- "มี" --> H["ตันสามด้าน → กลับหลัง (act=3)"]
```

> ⚠️ `wallRight()` ที่อ่านเพี้ยน (false wall) ทำให้เลือกทางผิด → พลาดทาง goal

---

## Sequence — moveCell (เดินหน้า 1 ช่อง)

```mermaid
sequenceDiagram
  participant S as L3 (run loop)
  participant M as Micromouse facade
  participant C as controlTask (500Hz core1)
  participant HW as HAL

  Note over C,HW: controlTask วน @500Hz อิสระจาก L3
  loop ทุก 2ms
    C->>HW: encoder_update + imu_update
    C->>C: profile.update()
    C->>HW: motors_update(FF+PD → PWM)
  end

  S->>M: moveCell(1)
  M->>C: control_start_forward(CELL_SIZE,...)
  loop จน profile เดินจบ
    M->>HW: อ่าน IR ซ้าย/ขวา
    M->>C: control_set_steering(adj)
  end
  M->>HW: fw_wall_present()?
  M-->>S: return true/false
  S->>S: update pose
```

---

## Sequence — turn90 / turnBack

```mermaid
sequenceDiagram
  participant S as L3
  participant M as Micromouse
  participant C as controlTask (500Hz)

  S->>M: turn90(LEFT)
  M->>M: _targetHeading += angle
  M->>C: control_start_rotation(angle, turnDps,...)
  M->>C: control_kick_rotation(kickPwm, kickMs, kickRel)
  Note right of C: kick ดันเต็มแรงเบรก stiction
  loop จนหมุนจบ
    M-->>S: BLE telemetry
  end
  M->>M: stall check
  M->>C: _settleHeading() — แก้ heading ±3°
  M-->>S: return true/false
```

---

## สถานะ Debug ปัจจุบัน

| ส่วน | สถานะ | หมายเหตุ |
|---|---|---|
| 500Hz FF+PD (เดินตรง) | ✅ | err ~±1° |
| heading-hold + settleHeading | ✅ | set_err ~±5° |
| **wallRight() sensing** | ❌ | 37-91mm ที่ช่องเปิด → เลือกทางผิด |
| **turn stall** | ⚠️ | ครั้งเว้นครั้ง (แรงบิด/แบต) |
| pose tracking + termination | ✅ | T0/T1/T2 ถูกต้อง |

---

## See Also

- [[micromouse]] — ภาพรวม + lab guide
- [[micromouse_ble]] — BLE wireless testing
