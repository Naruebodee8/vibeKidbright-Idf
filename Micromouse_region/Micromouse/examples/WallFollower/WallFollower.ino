// ═══════════════════════════════════════════════════════════════════════════
//  WallFollower.ino  —  L3 (นักเรียนแก้ไฟล์นี้ไฟล์เดียว)
//
//  อัลกอริทึมเดินตามกำแพง (Left / Right hand rule) — ก้าวแรกสู่การแก้เขาวงกต
//  ทุกอย่างใต้ API (ลูปคุม 500Hz, FF+PD, หมุน gyro, จัดกลาง, BLE) ซ่อนใน
//  ไลบรารี Micromouse แล้ว — ที่นี่มีแต่ "ตรรกะ" ล้วน ๆ
//
//  กติกาแกน (เหมือน flood fill จะใช้ทีหลัง):
//    sense (อ่านกำแพง) -> decide (เลือกก้าว) -> execute -> update pose -> เช็คจบ
//  ของ flood fill ต่างกันแค่ decideMove() เท่านั้น — ที่เหลือ reuse ได้หมด
//
//  Convention (ตาม SDD): heading 0=N(เหนือ) 1=E 2=S 3=W ; เดินหน้าคือไปตาม heading
//
//  คุมไร้สาย: ต่อ BLE "MM_robot" (nRF Connect / Serial BLE Terminal / Python UI)
//    ส่ง 'g' = START, 'x' = ABORT, 't' = สลับ telemetry, "kp 1.5" จูน steering
// ═══════════════════════════════════════════════════════════════════════════
#include <Micromouse.h>

Micromouse mouse;

// ── ตั้งค่าได้ (config) ───────────────────────────────────────────────────────
const bool USE_LEFT_HAND = true;   // true = มือซ้ายแตะกำแพง, false = มือขวา
const int  GOAL_X = 3, GOAL_Y = 2; // เป้าหมาย (สนามเล็ก 4x4 = (2,2)); 16x16 ใช้ 7/8
const int  MAX_STEPS = 100;        // เพดานกันวนไม่จบ (T0 safety)

// ── pose ปัจจุบัน (dead reckoning) ───────────────────────────────────────────
int posX = 0, posY = 0, heading = 0;   // เริ่มที่ (0,0) หันเหนือ
const int DX[4] = {0, 1, 0, -1};       // N,E,S,W
const int DY[4] = {1, 0, -1, 0};

void setup() {
    mouse.begin();
}

// ── ก้าวเดินหน้า 1 ช่อง + อัปเดต pose (อัปเดตเฉพาะเมื่อสำเร็จ!) ──────────────
bool stepForward() {
    if (!mouse.moveCell(1)) return false;        // false = abort/ชน -> อย่าแตะ pose
    posX += DX[heading];
    posY += DY[heading];
    return true;
}
bool stepTurnLeft()  { if (!mouse.turn90(LEFT))  return false; heading = (heading + 3) & 3; return true; }
bool stepTurnRight() { if (!mouse.turn90(RIGHT)) return false; heading = (heading + 1) & 3; return true; }
bool stepTurnBack()  { if (!mouse.turnBack())    return false; heading = (heading + 2) & 3; return true; }

// ── ★ หัวใจอัลกอริทึม: ตัดสินใจจากกำแพงรอบตัว ★ ─────────────────────────────
//   (ทีหลังนักเรียนกลุ่มเก่งแทนฟังก์ชันนี้ด้วย flood fill ได้ โดยไม่แตะส่วนอื่น)
//   คืนค่า: 0=เดินหน้า 1=เลี้ยวซ้ายแล้วเดิน 2=เลี้ยวขวาแล้วเดิน 3=กลับหลังแล้วเดิน
int decideMove(bool wallL, bool wallF, bool wallR) {
    if (USE_LEFT_HAND) {
        if (!wallL)      return 1;   // ซ้ายว่าง -> เลี้ยวซ้าย
        else if (!wallF) return 0;   // ตรงว่าง  -> ตรงไป
        else if (!wallR) return 2;   // ขวาว่าง  -> เลี้ยวขวา
        else             return 3;   // ตันสามด้าน -> กลับหลัง
    } else {
        if (!wallR)      return 2;   // (มือขวา) ขวาว่าง -> เลี้ยวขวา
        else if (!wallF) return 0;
        else if (!wallL) return 1;
        else             return 3;
    }
}

// ── ลูปสำรวจ (ก้าวต่อก้าว) ──────────────────────────────────────────────────
void run() {
    for (int step = 0; step < MAX_STEPS; step++) {
        // เช็คถึงเป้า (T1: dead-reckoning goal)
        if (posX == GOAL_X && posY == GOAL_Y) { mouse.log("GOAL reached!"); return; }
        // เช็คกลับถึงจุดเริ่ม หลังออกเดินแล้ว (T2)
        if (step > 0 && posX == 0 && posY == 0) { mouse.log("returned to START"); return; }

        mouse.reportPose(posX, posY, heading);       // ให้ UI วาดตำแหน่ง

        // 1) sense — อ่านตอนหยุดนิ่งที่กลางช่อง
        bool wL = mouse.wallLeft();
        bool wF = mouse.wallFront();
        bool wR = mouse.wallRight();

        // 2) decide
        int act = decideMove(wL, wF, wR);

        // (optional) log การตัดสินใจแต่ละช่อง — เปิดบรรทัดนี้ถ้าอยากดูว่าหุ่นเลือกอะไร
        // mouse.logf("STEP (%d,%d) h%d  L%d F%d R%d -> act%d\n", posX,posY,heading, wL,wF,wR, act);

        // 3) execute (หมุนก่อนถ้าต้อง แล้วเดินหน้า 1 ช่อง)
        bool ok = true;
        if      (act == 1) ok = stepTurnLeft();
        else if (act == 2) ok = stepTurnRight();
        else if (act == 3) ok = stepTurnBack();
        if (ok) ok = stepForward();

        if (!ok) { mouse.log("aborted/stalled — stopping"); return; }
    }
    mouse.log("MAX_STEPS reached — stopping (T0 cap)");
}

void loop() {
    mouse.waitForStart();          // รอปุ่ม START / 'g' ; arm + zero pose reference
    posX = 0; posY = 0; heading = 0;
    run();
    mouse.log("run done");
}
