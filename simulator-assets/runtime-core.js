/* Prototype semantics shared by the runtime and regression tests. No native APIs. */
(function (root) {
  'use strict';
  const APIS = Object.freeze([
    'sound.note', 'sound.rest', 'sound.off', 'sound.get_volume', 'sound.set_volume', 'sound.tone',
    'vTaskDelay', 'ports.output1_write', 'ports.output2_write', 'ports.usbsw_write',
    'ports.output1_read', 'ports.output2_read', 'ports.usbsw_read',
    'ports.input1_read', 'ports.input2_read', 'ports.input3_read', 'ports.input4_read',
    'kb_specio.input1_read', 'kb_specio.input2_read', 'kb_specio.input3_read', 'kb_specio.input4_read',
    'button12.is_sw1_pressed', 'button12.is_sw2_pressed', 'button12.is_sw1_released',
    'button12.is_sw2_released', 'button12.sw1_get', 'button12.sw2_get',
    'ldr.get', 'lm73_1.get', 'ht16k33.show', 'ht16k33.idle', 'ht16k33.wait_idle', 'matrix.show_number'
  ]);
  function validate(source, parser, options) {
    let outputs = 0;
    if (typeof source !== 'string' || !source.trim() || source.length > 64000) {
      throw new Error('Use a non-empty simulation program of at most 64,000 characters.');
    }
    let ast;
    try { ast = parser.parse(source, { ecmaVersion: 5, locations: true }); }
    catch (error) { throw new Error('Simulation JavaScript syntax: ' + error.message + '. C/C++ is not supported.'); }
    function name(node) {
      if (node.type === 'Identifier') return node.name;
      if (node.type === 'MemberExpression' && !node.computed) return name(node.object) + '.' + node.property.name;
      return '';
    }
    function visit(node) {
      if (!node || typeof node !== 'object') return;
      if (node.type === 'CallExpression' && ['sound.note','sound.tone','sound.off','ht16k33.show','matrix.show_number','ports.output1_write','ports.output2_write','ports.usbsw_write'].includes(name(node.callee))) outputs++;
      if (node.type === 'CallExpression' && !APIS.includes(name(node.callee))) {
        throw new Error('Unsupported simulator call at line ' + node.loc.start.line + ': ' + (name(node.callee) || 'dynamic call'));
      }
      if (['NewExpression', 'FunctionExpression', 'FunctionDeclaration', 'WithStatement', 'DebuggerStatement'].includes(node.type)) {
        throw new Error('Unsupported simulation syntax at line ' + node.loc.start.line + ': ' + node.type);
      }
      if (node.type === 'AssignmentExpression' && node.left.type === 'MemberExpression') {
        throw new Error('Device APIs are read-only. Use the supported device functions.');
      }
      if (node.type === 'VariableDeclarator' && APIS.some(api => api.split('.')[0] === name(node.id))) {
        throw new Error('Do not replace a simulator device name.');
      }
      Object.keys(node).forEach(key => {
        if (key === 'loc') return;
        const value = node[key];
        if (Array.isArray(value)) value.forEach(visit);
        else if (value && typeof value === 'object') visit(value);
      });
    }
    visit(ast);
    if (options && options.requireOutput && outputs === 0) throw new Error('ไฟล์ซิมยังไม่มีคำสั่งแสดง LED, buzzer หรือควบคุม OUT/USB ให้ AI สร้างพฤติกรรมจริงด้วย generate_simulation');
    return ast;
  }
  function matrixCsv(buffer) {
    if (typeof buffer !== 'string' || buffer.length !== 16 || Array.from(buffer).some(c => c.charCodeAt(0) > 255)) {
      throw new Error('ht16k33.show requires a 16-byte string (one byte per column).');
    }
    const pixels = [];
    for (let x = 0; x < 16; x++) for (let y = 0; y < 8; y++) pixels.push((buffer.charCodeAt(x) >> y) & 1);
    return pixels.join(',');
  }
  const DIGITS = {
    '0':['111','101','101','101','111'], '1':['010','110','010','010','111'],
    '2':['111','001','111','100','111'], '3':['111','001','111','001','111'],
    '4':['101','101','111','001','001'], '5':['111','100','111','001','111'],
    '6':['111','100','111','101','111'], '7':['111','001','010','010','010'],
    '8':['111','101','111','101','111'], '9':['111','101','111','001','111'],
    '-':['000','000','111','000','000']
  };
  function numberBuffer(value) {
    if (typeof value !== 'number' || !Number.isFinite(value)) throw new Error('matrix.show_number requires a finite number.');
    const rounded = Math.round(value);
    if (rounded < -999 || rounded > 9999) throw new Error('matrix.show_number supports integers -999 to 9999.');
    const text = String(rounded), bytes = Array(16).fill(0);
    const left = Math.floor((16 - (text.length * 4 - 1)) / 2);
    for (let n = 0; n < text.length; n++) for (let y = 0; y < 5; y++) for (let x = 0; x < 3; x++) {
      // Verified against SceneKV: bit 0 is the visual bottom row. Keep the
      // existing raw ht16k33.show mapping; orient only our numeric helper.
      if (DIGITS[text[n]][y][x] === '1') bytes[left + n * 4 + x] |= 1 << (6 - y);
    }
    return String.fromCharCode.apply(null, bytes);
  }
  // Original kidbright32/sound.js frequencies, C4..C7 (note 0..36).
  const NOTES = Object.freeze([261,277,293,311,329,349,369,391,415,440,466,493,523,554,587,622,659,698,740,784,831,880,932,988,1046,1109,1175,1244,1318,1396,1480,1568,1661,1760,1865,1976,2093]);
  function noteFrequency(note) {
    if (!Number.isInteger(note) || note < 0 || note >= NOTES.length) throw new Error('sound.note requires note 0..36 (C4..C7).');
    return NOTES[note];
  }
  function noteDuration(duration) {
    if (!Number.isInteger(duration) || duration < 0 || duration > 4) throw new Error('sound.rest requires duration 0..4.');
    return [2000,1000,500,250,125][duration]; // Reference host: fixed 120 BPM.
  }
  root.KidBrightRuntimeCore = Object.freeze({ APIS, validate, matrixCsv, numberBuffer, noteFrequency, noteDuration });
})(typeof globalThis === 'undefined' ? window : globalThis);
