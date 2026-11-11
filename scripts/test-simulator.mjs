import { test } from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import vm from 'node:vm';
import ts from 'typescript';
import { webcrypto } from 'node:crypto';

const sandbox = vm.createContext({});
vm.runInContext(fs.readFileSync('simulator-assets/JS-Interpreter/acorn.js', 'utf8'), sandbox);
vm.runInContext(fs.readFileSync('simulator-assets/runtime-core.js', 'utf8'), sandbox);
const core = sandbox.KidBrightRuntimeCore;

test('startup script failures are reported and replayed to a late IDE listener with sender validation', () => {
  const html = fs.readFileSync('simulator-assets/UnityBuild/index.html', 'utf8');
  const bootstrap = html.match(/<script data-simulator-bootstrap>([\s\S]*?)<\/script>/)[1];
  const listeners = new Map(), events = [];
  const parent = { postMessage(message, origin) { events.push({ message, origin }); } };
  const context = vm.createContext({ URLSearchParams, parent,
    location: { hash: '#parentOrigin=http%3A%2F%2Flocalhost%3A1420&sessionId=startup-test' } });
  context.window = context;
  context.addEventListener = (type, fn) => listeners.set(type, fn);
  vm.runInContext(bootstrap, context);
  listeners.get('error')({ target: { tagName: 'SCRIPT', src: 'http://127.0.0.1:5000/session/UnityProgress.js' } });
  assert.match(events[0].message.payload.message, /Cannot load local simulator script: UnityProgress.js/);
  assert.equal(events[0].origin, 'http://localhost:1420');
  const hello = { source: parent, origin: 'http://localhost:1420', data: { protocolVersion: 1, sessionId: 'startup-test', requestId: 'late', type: 'HELLO' } };
  listeners.get('message')({ ...hello, origin: 'https://example.invalid' });
  listeners.get('message')({ ...hello, source: {} });
  listeners.get('message')({ ...hello, data: { ...hello.data, sessionId: 'stale' } });
  assert.equal(events.length, 1);
  listeners.get('message')(hello);
  assert.equal(events[1].message.requestId, 'late');
  assert.equal(events[1].message.payload.startup, true);
});
test('accepts the supported basic device programs and rejects firmware/unknown device APIs', () => {
  for (const program of [
    'ports.output2_write(1); vTaskDelay(500); ports.output2_write(0);',
    'while(1){ports.output2_write(button12.is_sw1_pressed()?1:0);vTaskDelay(50);}',
    'var t=lm73_1.get(); if(t>35){ports.usbsw_write(1);}',
  ]) assert.doesNotThrow(() => core.validate(program, sandbox.acorn));
  for (const program of ['void app_main(void){}', 'fetch("https://example.invalid")', 'ht16k33.scroll("hi",true)',
    'sound.play(60)', 'new Function("return window")()', 'ports.output2_write = 1;', 'var ports=0;', 'eval("1")', '', 'a'.repeat(64001)]) {
    assert.throws(() => core.validate(program, sandbox.acorn), undefined, program.slice(0, 80));
  }
});
test('LED bytes map to the reference x-major 128-pixel CSV without swapping corners', () => {
  const pixels = core.matrixCsv(String.fromCharCode(129) + '\0'.repeat(14) + String.fromCharCode(129)).split(',');
  assert.equal(pixels.length, 128);
  assert.deepEqual(pixels.flatMap((value, index) => value === '1' ? [index] : []), [0, 7, 120, 127]);
  assert.throws(() => core.matrixCsv('short'));
  assert.throws(() => core.matrixCsv('ก'.repeat(16)));
});

test('number helper centers readable digits/sign and rejects invalid or overflowing values', () => {
  const buffer = core.numberBuffer(25);
  assert.equal(buffer.length, 16);
  function rows(value) {
    const bytes = core.numberBuffer(value);
    // SceneKV renders bit 7 at the visual top; this asserts visible glyphs.
    return Array.from({length:8}, (_,y) => Array.from({length:16}, (_,x) => ((bytes.charCodeAt(x)>>(7-y))&1) ? '1':'0').join(''));
  }
  assert.deepEqual(rows(0).slice(1,6).map(row => row.slice(6,9)), ['111','101','101','101','111']);
  assert.deepEqual(rows(25).slice(1,6).map(row => row.slice(4,7)), ['111','001','111','100','111']);
  assert.deepEqual(rows(25).slice(1,6).map(row => row.slice(8,11)), ['111','100','111','001','111']);
  assert.equal(core.numberBuffer(24.6), core.numberBuffer(25));
  assert.notEqual(core.numberBuffer(-12), core.numberBuffer(12));
  for (const value of [-999,9999,0,-0]) assert.doesNotThrow(()=>core.numberBuffer(value));
  for (const value of [NaN,Infinity,-Infinity,'25',null,{},-1000,10000]) assert.throws(()=>core.numberBuffer(value));
  assert.doesNotThrow(()=>core.validate('while(1){matrix.show_number(lm73_1.get());vTaskDelay(250);}',sandbox.acorn));
});

function adapterFixture() {
  const listeners = new Set();
  const outgoing = [];
  const target = { postMessage(message, origin) { outgoing.push({ message, origin }); } };
  const win = { addEventListener(_, fn) { listeners.add(fn); }, removeEventListener(_, fn) { listeners.delete(fn); } };
  const context = vm.createContext({ window: win, crypto: webcrypto, setTimeout, clearTimeout, exports: {} });
  const js = ts.transpileModule(fs.readFileSync('src/simulator/SimulatorAdapter.ts', 'utf8'), { compilerOptions: { module: ts.ModuleKind.CommonJS, target: ts.ScriptTarget.ES2020 } }).outputText;
  vm.runInContext(js, context);
  const events = [];
  const adapter = new context.exports.SimulatorAdapter(target, 'http://127.0.0.1:5000', 'session', event => events.push(event));
  const deliver = (data, origin = 'http://127.0.0.1:5000', source = target) => {
    for (const fn of listeners) fn({ data, origin, source });
  };
  return { adapter, outgoing, events, deliver, target, listeners };
}
test('adapter checks the exact source/origin/session and resolves only its own request', async () => {
  const f = adapterFixture();
  const pending = f.adapter.request('HELLO');
  const message = { ...f.outgoing[0].message, type: 'READY', payload: {} };
  f.deliver(message, 'https://example.invalid');
  f.deliver(message, undefined, {});
  f.deliver({ ...message, sessionId: 'old-session' });
  assert.equal(f.events.length, 0);
  f.deliver(message);
  assert.equal((await pending).type, 'READY');
  assert.equal(f.outgoing[0].origin, 'http://127.0.0.1:5000');
  f.adapter.close();
  assert.equal(f.listeners.size, 0);
});
test('adapter propagates errors, times out and cancels pending requests on close', async () => {
  const f = adapterFixture();
  const request = f.adapter.request('LOAD_PROGRAM');
  f.deliver({ ...f.outgoing[0].message, type: 'ERROR', payload: { message: 'Unsupported source' } });
  await assert.rejects(request, /Unsupported source/);
  await assert.rejects(f.adapter.request('HELLO', {}, null, 5), /did not respond/);
  const pending = f.adapter.request('RUN');
  f.adapter.close();
  await assert.rejects(pending, /closed/);
  await assert.rejects(f.adapter.request('RUN'), /closed/);
});

function runtimeFixture({ autoRun = true } = {}) {
  const events = [], calls = [], listeners = new Map(), elements = new Map();
  const parent = { postMessage(data, origin) { events.push({ ...data, origin }); } };
  const element = id => {
    if (!elements.has(id)) elements.set(id, { textContent: '0' });
    return elements.get(id);
  };
  const timers = new Set();
  let clockOffset = 0;
  let bridgeFailure = false;
  class Clock extends Date { static now() { return Date.now() + clockOffset; } }
  const context = vm.createContext({ parent, location: { hash: '#parentOrigin=http%3A%2F%2Flocalhost%3A1420&sessionId=runtime-test' },
    URLSearchParams, crypto: webcrypto, performance, console, Date: Clock,
    setTimeout(fn, delay) { const id = setTimeout(() => { timers.delete(id); fn(); }, delay); timers.add(id); return id; },
    clearTimeout(id) { timers.delete(id); clearTimeout(id); },
    document: { getElementById: element, createElement: () => ({}), head: { appendChild(script) {
      vm.runInContext(fs.readFileSync('simulator-assets/' + script.src.replace('../', ''), 'utf8'), context);
      script.onload();
    } } },
    Call_RunProgram(value) { element('TextRun').textContent = String(value); },
    UnityProgress() {}, LoadFileInit() {}, gameInstance: null,
    UnityLoader: { instantiate(_id, _url, options) {
      queueMicrotask(() => options.onProgress({}, 1));
      return { SendMessage(...args) {
        calls.push(args);
        if (bridgeFailure) throw new Error('Unity bridge unavailable');
        if (args[0] === '$[UIcontroller]' && args[1] === 'ClickRun' && autoRun) queueMicrotask(() => context.Call_RunProgram(1));
        if (args[0] === '$[UIcontroller]' && args[1] === 'ClickStop') queueMicrotask(() => context.Call_RunProgram(0));
      } };
    } },
  });
  context.window = context;
  context.addEventListener = (type, fn) => listeners.set(type, fn);
  vm.runInContext(fs.readFileSync('simulator-assets/runtime-core.js', 'utf8'), context);
  vm.runInContext(fs.readFileSync('simulator-assets/runtime.js', 'utf8'), context);
  const send = (type, payload = {}, runId = null, overrides = {}) => {
    const requestId = webcrypto.randomUUID();
    listeners.get('message')({ source: parent, origin: 'http://localhost:1420',
      data: { protocolVersion: 1, sessionId: 'runtime-test', requestId, runId, type, payload }, ...overrides });
    return events.find(event => event.requestId === requestId);
  };
  return { context, events, calls, send, element, advanceClock(ms) { clockOffset += ms; }, setBridgeFailure(value) { bridgeFailure = value; }, close() { send('DISPOSE'); for (const id of timers) clearTimeout(id); } };
}
async function waitFor(predicate, timeout = 1500) {
  const start = Date.now();
  while (!predicate()) {
    assert(Date.now() - start < timeout, 'runtime event timed out');
    await new Promise(resolve => setTimeout(resolve, 5));
  }
}

test('legacy scene selection retries after the splash and stops on automatic Unity start or Dispose', async () => {
  const f = runtimeFixture();
  try {
    await waitFor(() => f.events.some(event => event.type === 'READY'));
    const sceneCalls = () => f.calls.filter(call => call[1] === 'SceneGame').length;
    const initial = sceneCalls();
    await new Promise(resolve => setTimeout(resolve, 300));
    assert(sceneCalls() > initial);
    f.send('LOAD_PROGRAM', { source: 'ports.output1_write(1);', scene: 0 });
    f.send('RUN', {}, 'scene-ready');
    await waitFor(() => f.events.some(event => event.type === 'RUN_ENDED'));
    const started = sceneCalls();
    await new Promise(resolve => setTimeout(resolve, 300));
    assert.equal(sceneCalls(), started);
  } finally { f.close(); }
  const disposed = runtimeFixture();
  await waitFor(() => disposed.events.some(event => event.type === 'READY'));
  disposed.send('DISPOSE');
  const afterClose = disposed.calls.length;
  await new Promise(resolve => setTimeout(resolve, 300));
  assert.equal(disposed.calls.length, afterClose);
  disposed.close();
});

test('one RUN requests Unity ClickRun, waits for its acknowledgement and executes the real interpreter', async () => {
  const f = runtimeFixture();
  try {
    await waitFor(() => f.events.some(event => event.type === 'READY'));
    f.element('TextSwitch1').textContent = '1'; f.element('TextTempSensor').textContent = '40';
    assert.equal(f.send('LOAD_PROGRAM', { source: 'ports.output2_write(button12.is_sw1_pressed()?1:0);ports.usbsw_write(lm73_1.get()>35?1:0);vTaskDelay(50);ports.output2_write(0);', scene: 0, sourceRevision: 'snapshot' }).type, 'PROGRAM_LOADED');
    assert.equal(f.send('RUN', {}, 'one').type, 'RUN_ARMED');
    assert(!f.events.some(event => event.type === 'RUN_STARTED'));
    assert.equal(f.send('LOAD_PROGRAM', { source: 'ports.output1_write(1);', scene: 0 }).type, 'ERROR');
    assert(f.calls.some(call => call[0] === '$[UIcontroller]' && call[1] === 'ClickRun'));
    await waitFor(() => f.events.some(event => event.type === 'RUN_ENDED'));
    assert(f.events.some(event => event.type === 'OUTPUT' && event.payload.out2 === 1 && event.payload.usb === 1));
    assert(f.calls.some(call => call[0] === '$[IDEcomunication]' && call[1] === 'MessageWriteIO2' && call[2] === 'Output2_On'));
    const ended = f.events.find(event => event.type === 'RUN_ENDED');
    assert.equal(ended.payload.reason, 'completed'); assert.equal(ended.payload.sourceRevision, 'snapshot');
  } finally { f.close(); }
});

test('Stop clears delayed callbacks, is idempotent and permits a fresh Run; Reset clears outputs', async () => {
  const f = runtimeFixture();
  try {
    await waitFor(() => f.events.some(event => event.type === 'READY'));
    f.send('LOAD_PROGRAM', { source: 'ports.output1_write(1);vTaskDelay(120);ports.output2_write(1);', scene: 0 });
    f.send('RUN', {}, 'old');
    await waitFor(() => f.events.some(event => event.type === 'OUTPUT' && event.payload.out1 === 1));
    assert.equal(f.send('RUN', {}, 'duplicate').type, 'ERROR');
    assert.equal(f.send('STOP', {}, 'stale').type, 'ERROR');
    assert(!f.events.some(event => event.type === 'RUN_ENDED'));
    f.send('STOP', {}, 'old'); f.send('STOP', {}, 'old');
    await new Promise(resolve => setTimeout(resolve, 150));
    assert.equal(f.events.filter(event => event.type === 'RUN_ENDED' && event.runId === 'old').length, 1);
    assert(!f.events.some(event => event.type === 'OUTPUT' && event.payload.out2 === 1));
    f.send('LOAD_PROGRAM', { source: 'ports.usbsw_write(1);', scene: 0 });
    f.send('RUN', {}, 'new');
    await waitFor(() => f.events.some(event => event.type === 'RUN_ENDED' && event.runId === 'new'));
    f.send('RESET');
    await waitFor(() => f.events.some(event => event.type === 'OUTPUT' && event.payload.out1 === 0 && event.payload.usb === 0));
  } finally { f.close(); }
});

test('runtime rejects stale senders, unsupported scene/source, duplicate Run and reports execution errors', async () => {
  const f = runtimeFixture();
  try {
    await waitFor(() => f.events.some(event => event.type === 'READY'));
    assert.equal(f.send('LOAD_PROGRAM', {}, null, { origin: 'https://example.invalid' }), undefined);
    assert.equal(f.send('LOAD_PROGRAM', {}, null, { source: {} }), undefined);
    assert.equal(f.send('LOAD_PROGRAM', { source: 'ports.output2_write(1);', scene: 1 }).type, 'ERROR');
    assert.equal(f.send('LOAD_PROGRAM', { source: 'fetch("http://example.invalid")', scene: 0 }).type, 'ERROR');
    f.send('LOAD_PROGRAM', { source: 'vTaskDelay(-1);', scene: 0 });
    f.send('RUN', {}, 'bad');
    assert.equal(f.send('RUN', {}, 'duplicate').type, 'ERROR');
    await waitFor(() => f.events.some(event => event.type === 'RUN_ENDED' && event.payload.reason === 'error'));
    assert.equal(f.send('RUN', {}, 'bad').type, 'ERROR');
  } finally { f.close(); }
});

test('temperature changes update the actual interpreter LED number during the same run and Unity mode can be restored', async () => {
  const f = runtimeFixture();
  try {
    await waitFor(()=>f.events.some(event=>event.type==='READY'));
    assert.equal(f.send('SET_INPUTS',{temperature:25}).type,'INPUTS_UPDATED');
    assert.equal(f.send('SET_INPUTS',{temperature:'oops'}).type,'ERROR');
    assert.equal(f.send('SET_INPUTS',{temperature:151}).type,'ERROR');
    f.send('LOAD_PROGRAM',{source:'while(1){matrix.show_number(lm73_1.get());vTaskDelay(20);}',scene:0});
    f.send('RUN',{},'thermometer');
    await waitFor(()=>f.events.some(e=>e.type==='OUTPUT'&&e.payload.displayNumber===25));
    f.send('SET_INPUTS',{temperature:42.4});
    await waitFor(()=>f.events.some(e=>e.type==='OUTPUT'&&e.runId==='thermometer'&&e.payload.displayNumber===42));
    assert(f.calls.some(call=>call[1]==='MessageLED'&&call[2]===core.matrixCsv(core.numberBuffer(42))));
    f.element('TextTempSensor').textContent='-12';
    f.send('SET_INPUTS',{temperature:null});
    await waitFor(()=>f.events.some(e=>e.type==='OUTPUT'&&e.payload.displayNumber===-12));
    f.send('STOP',{},'thermometer');
    const terminal=f.events.filter(e=>e.type==='RUN_ENDED'); assert.equal(terminal.length,1);
  } finally { f.close(); }
});

test('missing Unity start acknowledgement times out without running JS; Stop cancels retries and ignores late Play', async () => {
  const f = runtimeFixture({ autoRun: false });
  try {
    await waitFor(() => f.events.some(event => event.type === 'READY'));
    f.send('LOAD_PROGRAM', { source: 'ports.output1_write(1);', scene: 0 });
    f.send('RUN', {}, 'no-ack');
    assert(!f.events.some(event => event.type === 'RUN_STARTED'));
    const before = f.calls.filter(call => call[1] === 'ClickRun').length;
    await waitFor(() => f.calls.filter(call => call[1] === 'ClickRun').length > before);
    f.advanceClock(16000);
    await waitFor(() => f.events.some(event => event.type === 'RUN_ENDED' && event.runId === 'no-ack'));
    const ended = f.events.find(event => event.type === 'RUN_ENDED' && event.runId === 'no-ack');
    assert.equal(ended.payload.reason, 'error');
    assert.match(ended.payload.message, /15/);
    assert(!f.events.some(event => event.type === 'OUTPUT' && event.payload.out1 === 1));
    f.send('RUN', {}, 'cancelled');
    f.send('STOP', {}, 'cancelled');
    const stopped = f.calls.filter(call => call[1] === 'ClickRun').length;
    f.context.Call_RunProgram(1); // A command already queued in Unity may finish after Stop.
    await new Promise(resolve => setTimeout(resolve, 300));
    assert.equal(f.calls.filter(call => call[1] === 'ClickRun').length, stopped);
    assert(!f.events.some(event => event.type === 'RUN_STARTED'));
    assert.equal(f.events.filter(event => event.type === 'RUN_ENDED' && event.runId === 'cancelled').length, 1);
    f.send('RUN', {}, 'retry');
    f.context.Call_RunProgram(1);
    await waitFor(() => f.events.some(event => event.type === 'RUN_ENDED' && event.runId === 'retry'));
    assert(f.events.some(event => event.type === 'OUTPUT' && event.payload.out1 === 1));
  } finally { f.close(); }
});

test('a throwing Unity bridge ends startup once with an error and permits retry after recovery', async () => {
  const f = runtimeFixture();
  try {
    await waitFor(() => f.events.some(event => event.type === 'READY'));
    f.send('LOAD_PROGRAM', { source: 'matrix.show_number(25);', scene: 0 });
    f.setBridgeFailure(true);
    f.send('RUN', {}, 'bridge-error');
    const ended = f.events.filter(event => event.type === 'RUN_ENDED' && event.runId === 'bridge-error');
    assert.equal(ended.length, 1);
    assert.equal(ended[0].payload.reason, 'error');
    assert.match(ended[0].payload.message, /Unity bridge unavailable/);
    assert(!f.events.some(event => event.type === 'RUN_STARTED'));
    f.setBridgeFailure(false);
    f.send('RUN', {}, 'recovered');
    await waitFor(() => f.events.some(event => event.type === 'RUN_ENDED' && event.runId === 'recovered'));
    assert(f.events.some(event => event.type === 'OUTPUT' && event.runId === 'recovered' && event.payload.displayNumber === 25));
  } finally { f.setBridgeFailure(false); f.close(); }
});


const compiledCases = JSON.parse(fs.readFileSync('simulator-tests/compiler-cases.json', 'utf8'));
const compiled = name => compiledCases.find(c => c.name === name).source;
function literalFrames(source) {
  const frames=[];
  function visit(node) {
    if (!node || typeof node !== 'object') return;
    if (node.type==='CallExpression' && node.callee.object?.name==='ht16k33' && node.callee.property?.name==='show' && typeof node.arguments[0]?.value==='string')
      frames.push(core.matrixCsv(node.arguments[0].value));
    for (const [key,value] of Object.entries(node)) {
      if(key==='loc')continue;
      if(Array.isArray(value))value.forEach(visit);else if(value&&typeof value==='object')visit(value);
    }
  }
  visit(core.validate(source,sandbox.acorn));
  return frames;
}

test('checked compiler output is valid ES5 for the actual reference interpreter', () => {
  for (const c of compiledCases) assert.doesNotThrow(() => core.validate(c.source, sandbox.acorn, {requireOutput:true}), c.name);
  for (const source of ['while(1){vTaskDelay(500);}', '// ht16k33.show("fake");\nvTaskDelay(1);', 'var s="matrix.show_number(25)";'])
    assert.throws(() => core.validate(source,sandbox.acorn,{requireOutput:true}), /ยังไม่มีคำสั่ง/);
});
test('compiled A-Z runs all 26 bitmaps in order, wraps to A and stops without further frames', async () => {
  const f=runtimeFixture(), source=compiled('alphabet'), expected=literalFrames(source);
  try {
    assert.equal(expected.length,26);
    await waitFor(()=>f.events.some(e=>e.type==='READY'));
    assert.equal(f.send('LOAD_PROGRAM',{source,scene:0,projectCompanion:true}).type,'PROGRAM_LOADED');
    f.send('RUN',{},'letters');
    await waitFor(()=>f.events.filter(e=>e.type==='OUTPUT'&&e.runId==='letters').length>=27,20000);
    const frames=f.events.filter(e=>e.type==='OUTPUT'&&e.runId==='letters').map(e=>e.payload.matrix);
    assert.deepEqual(frames.slice(0,27),[...expected,expected[0]]);
    assert(f.calls.some(c=>c[1]==='MessageLED'&&c[2]===expected[25]));
    f.send('STOP',{},'letters');
    const after=f.calls.filter(c=>c[1]==='MessageLED').length;
    await new Promise(r=>setTimeout(r,550));
    assert.equal(f.calls.filter(c=>c[1]==='MessageLED').length,after);
  } finally { f.close(); }
});
test('compiled sensors, outputs and input conditions react during the same run', async () => {
  const f=runtimeFixture();
  try {
    await waitFor(()=>f.events.some(e=>e.type==='READY'));
    f.send('SET_INPUTS',{temperature:25});
    f.send('LOAD_PROGRAM',{source:compiled('temperature'),scene:0,projectCompanion:true});f.send('RUN',{},'compiled-temperature');
    await waitFor(()=>f.events.some(e=>e.type==='OUTPUT'&&e.payload.displayNumber===25&&e.payload.usb===0));
    f.send('SET_INPUTS',{temperature:42});
    await waitFor(()=>f.events.some(e=>e.type==='OUTPUT'&&e.payload.displayNumber===42&&e.payload.usb===1));
    f.send('STOP',{},'compiled-temperature');
    f.element('TextSwitch1').textContent='1';f.element('TextSwitch2').textContent='0';f.element('TextInput1').textContent='1';f.element('TextInput4').textContent='1';
    f.send('LOAD_PROGRAM',{source:compiled('inputs'),scene:0,projectCompanion:true});f.send('RUN',{},'compiled-inputs');
    await waitFor(()=>f.events.some(e=>e.type==='OUTPUT'&&e.runId==='compiled-inputs'&&e.payload.out1===1&&e.payload.out2===1&&e.payload.displayNumber===2));
    f.element('TextSwitch1').textContent='0';f.element('TextInput4').textContent='0';
    await waitFor(()=>f.events.some(e=>e.type==='OUTPUT'&&e.runId==='compiled-inputs'&&e.payload.out1===0&&e.payload.displayNumber===1));
  } finally {f.close();}
});
test('compiled variables, while/break, dynamic glyphs and scrolling execute without unsupported calls', async () => {
  for (const name of ['while-break','char-counter','scroll','bitmap']) {
    const f=runtimeFixture();
    try {
      await waitFor(()=>f.events.some(e=>e.type==='READY'));
      const response=f.send('LOAD_PROGRAM',{source:compiled(name),scene:0,projectCompanion:true});
      assert.equal(response.type,'PROGRAM_LOADED',JSON.stringify(response));
      f.send('RUN',{},name);
      await waitFor(()=>f.events.some(e=>e.type==='RUN_ENDED'),5000);
      assert.equal(f.events.find(e=>e.type==='RUN_ENDED').payload.reason,'completed',name);
      const outputs=f.events.filter(e=>e.type==='OUTPUT');
      assert(outputs.some(e=>e.payload.matrix.split(',').includes('1')),name);
      if(name==='while-break')assert.equal(outputs.at(-1).payload.displayNumber,2);
    } finally {f.close();}
  }
});
test('compiler inserts yields so Stop works even when a requested loop has no delay', async () => {
  const f=runtimeFixture();
  try {
    await waitFor(()=>f.events.some(e=>e.type==='READY'));
    f.send('LOAD_PROGRAM',{source:compiled('responsive'),scene:0,projectCompanion:true});f.send('RUN',{},'responsive');
    await waitFor(()=>f.events.some(e=>e.type==='OUTPUT'&&e.payload.out1===1));
    f.send('STOP',{},'responsive');
    assert.equal(f.events.find(e=>e.type==='RUN_ENDED').payload.reason,'stopped');
  } finally {f.close();}
});

test('buzzer note frequencies and durations match the reference sound.js host', () => {
  const expected=[261,277,293,311,329,349,369,391,415,440,466,493,523,554,587,622,659,698,740,784,831,880,932,988,1046,1109,1175,1244,1318,1396,1480,1568,1661,1760,1865,1976,2093];
  assert.deepEqual(expected.map((_,i)=>core.noteFrequency(i)),expected);
  assert.deepEqual([0,1,2,3,4].map(core.noteDuration),[2000,1000,500,250,125]);
  for(const invalid of [-1,37,1.5,NaN,Infinity,'9']) assert.throws(()=>core.noteFrequency(invalid));
  for(const invalid of [-1,5,1.5,NaN,'2']) assert.throws(()=>core.noteDuration(invalid));
  core.validate('sound.set_volume(30);sound.note(9);sound.rest(4);sound.off();',sandbox.acorn,{requireOutput:true});
  core.validate('sound.tone(1000);vTaskDelay(200);sound.off();',sandbox.acorn,{requireOutput:true});
  for(const source of ['var sound=0;', 'sound.play(1000);','sound.note=function(){};','sound.set_volume(50);','var s="sound.tone(1000)";']) {
    assert.throws(()=>core.validate(source,sandbox.acorn,{requireOutput:true}),undefined,source);
  }
});

test('real interpreter emits a note, waits its reference duration, then becomes silent', async () => {
  const f=runtimeFixture();
  try {
    await waitFor(()=>f.events.some(e=>e.type==='READY'));
    f.send('LOAD_PROGRAM',{source:'sound.set_volume(30);sound.note(9);sound.rest(4);',scene:0,projectCompanion:true});
    const began=Date.now();f.send('RUN',{},'note');
    await waitFor(()=>f.events.some(e=>e.type==='BUZZER'&&e.payload.playing));
    const played=f.events.find(e=>e.type==='BUZZER'&&e.payload.playing);
    assert.equal(played.runId,'note');assert.equal(played.payload.frequency,440);assert.equal(played.payload.volume,30);
    assert(!f.events.some(e=>e.type==='RUN_ENDED'));
    await waitFor(()=>f.events.some(e=>e.type==='RUN_ENDED'));
    assert(Date.now()-began>=100,'rest must actually pause the interpreter');
    assert.equal(f.events.filter(e=>e.type==='BUZZER').at(-1).payload.playing,false);
  } finally {f.close();}
});

test('Stop/Reset/Dispose silence a sustained buzzer and late note callbacks cannot restart it', async () => {
  for(const terminal of ['STOP','RESET','DISPOSE']) {
    const f=runtimeFixture();
    try {
      await waitFor(()=>f.events.some(e=>e.type==='READY'));
      f.send('LOAD_PROGRAM',{source:'sound.note(0);sound.rest(3);sound.tone(2000);vTaskDelay(1000);',scene:0});
      f.send('RUN',{},terminal);
      await waitFor(()=>f.events.some(e=>e.type==='BUZZER'&&e.payload.playing));
      f.send(terminal,{},terminal==='STOP'?terminal:null);
      const atStop=f.events.length;
      assert.equal(f.events.filter(e=>e.type==='BUZZER').at(-1).payload.playing,false);
      await new Promise(resolve=>setTimeout(resolve,300));
      assert(!f.events.slice(atStop).some(e=>e.type==='BUZZER'&&e.payload.playing));
      assert.equal(f.events.filter(e=>e.type==='RUN_ENDED').length,1);
      if(terminal==='STOP') {
        f.send('RUN',{},'again');
        await waitFor(()=>f.events.some(e=>e.type==='BUZZER'&&e.payload.playing&&e.runId==='again'));
        f.send('STOP',{},'again');
      }
    } finally {f.close();}
  }
});

test('buzzer follows SW1 at runtime and accepts supported Hz endpoints', async () => {
  const f=runtimeFixture();
  try {
    await waitFor(()=>f.events.some(e=>e.type==='READY'));
    f.send('LOAD_PROGRAM',{source:'while(1){if(button12.sw1_get()){sound.tone(1000);}else{sound.off();}vTaskDelay(20);}',scene:0});
    f.send('RUN',{},'button');
    await new Promise(resolve=>setTimeout(resolve,60));
    assert(!f.events.some(e=>e.type==='BUZZER'&&e.payload.playing));
    f.element('TextSwitch1').textContent='1';
    await waitFor(()=>f.events.some(e=>e.type==='BUZZER'&&e.payload.frequency===1000));
    f.element('TextSwitch1').textContent='0';
    await waitFor(()=>f.events.filter(e=>e.type==='BUZZER').at(-1)?.payload.playing===false);
    f.send('STOP',{},'button');
    f.send('LOAD_PROGRAM',{source:'sound.tone(20);vTaskDelay(50);sound.tone(20000);vTaskDelay(50);',scene:0});
    f.send('RUN',{},'endpoints');
    await waitFor(()=>f.events.some(e=>e.type==='RUN_ENDED'&&e.runId==='endpoints'));
    for(const hz of [20,20000]) assert(f.events.some(e=>e.type==='BUZZER'&&e.payload.playing&&e.payload.frequency===hz));
  }finally{f.close();}
});

test('bad note/frequency/volume/duration errors end the run with an explicit silent buzzer', async () => {
  for(const bad of ['sound.note(60);','sound.tone(0/0);','sound.tone(20001);','sound.set_volume(-1);','sound.rest(5);']) {
    const f=runtimeFixture();
    try {
      await waitFor(()=>f.events.some(e=>e.type==='READY'));
      f.send('LOAD_PROGRAM',{source:'sound.tone(1000);'+bad,scene:0});
      f.send('RUN',{},'bad');
      await waitFor(()=>f.events.some(e=>e.type==='RUN_ENDED'));
      const end=f.events.find(e=>e.type==='RUN_ENDED');
      assert.equal(end.payload.reason,'error',bad);assert(end.payload.message);
      assert.equal(f.events.filter(e=>e.type==='BUZZER').at(-1).payload.playing,false);
    }finally{f.close();}
  }
});

test('actual compiler buzzer fixtures execute through the reference interpreter', async () => {
  const f=runtimeFixture();
  try {
    await waitFor(()=>f.events.some(e=>e.type==='READY'));
    f.send('LOAD_PROGRAM',{source:compiled('buzzer-notes'),scene:0,projectCompanion:true});
    f.send('RUN',{},'compiled-notes');
    await waitFor(()=>f.events.some(e=>e.type==='RUN_ENDED'),3000);
    assert.deepEqual(f.events.filter(e=>e.type==='BUZZER'&&e.payload.playing).map(e=>e.payload.frequency),[261,329,391]);
    f.send('LOAD_PROGRAM',{source:compiled('buzzer-button'),scene:0,projectCompanion:true});
    f.send('RUN',{},'compiled-button');
    const before=f.events.filter(e=>e.type==='BUZZER'&&e.payload.playing).length;
    await new Promise(resolve=>setTimeout(resolve,50));
    assert.equal(f.events.filter(e=>e.type==='BUZZER'&&e.payload.playing).length,before);
    f.element('TextSwitch1').textContent='1';
    await waitFor(()=>f.events.some(e=>e.type==='BUZZER'&&e.runId==='compiled-button'&&e.payload.playing));
    await new Promise(resolve=>setTimeout(resolve,300));
    assert.equal(f.events.filter(e=>e.type==='BUZZER'&&e.runId==='compiled-button'&&e.payload.playing).length,1,'held SW1 must not retrigger');
    f.send('STOP',{},'compiled-button');
  }finally{f.close();}
});


test('checked fan recipes debounce SW1, toggle once per hold and keep USB/OUT1/OUT2 distinct', async () => {
  const recipes=JSON.parse(fs.readFileSync('simulator-tests/fan-recipes.json','utf8'));
  for(const port of ['usb','out1','out2']) {
    const f=runtimeFixture();
    try {
      await waitFor(()=>f.events.some(e=>e.type==='READY'));
      assert.equal(f.send('LOAD_PROGRAM',{source:recipes[port].source,scene:0}).type,'PROGRAM_LOADED');
      f.send('RUN',{},'fan-'+port);
      await waitFor(()=>f.events.some(e=>e.type==='RUN_STARTED'));
      const latest=()=>f.events.filter(e=>e.type==='OUTPUT').at(-1)?.payload;
      let switchValue='0',sampleReads=0;
      Object.defineProperty(f.element('TextSwitch1'),'textContent',{configurable:true,
        get(){sampleReads++;return switchValue;},set(value){switchValue=String(value);sampleReads=0;}});
      await waitFor(()=>latest()?.[port]===0);
      // 5ms glitches are shorter than the 30ms stable sampling window.
      f.element('TextSwitch1').textContent='1';
      await new Promise(resolve=>setTimeout(resolve,5));
      f.element('TextSwitch1').textContent='0';
      await new Promise(resolve=>setTimeout(resolve,100));
      assert.equal(latest()[port],0,'a bounce must not switch the fan');
      f.element('TextSwitch1').textContent='1';
      await waitFor(()=>latest()?.[port]===1);
      await new Promise(resolve=>setTimeout(resolve,150));
      assert.equal(latest()[port],1,'holding must not repeatedly toggle');
      for(const other of ['usb','out1','out2'].filter(p=>p!==port))assert.equal(latest()[other],0);
      f.element('TextSwitch1').textContent='0';
      await new Promise(resolve=>setTimeout(resolve,100));
      await waitFor(()=>sampleReads>=5); // Wait for observed release samples, not host CPU speed.
      assert.equal(latest()[port],1,'release must not toggle');
      f.element('TextSwitch1').textContent='1';
      await waitFor(()=>latest()?.[port]===0);
      assert.equal(f.send('STOP',{},'fan-'+port).type,'ACK');
    } finally {f.close();}
  }
});


test('sustained tone overlaps LED and outputs, and nonfinite calculations never enable a port',async()=>{
  const f=runtimeFixture();
  try {
    await waitFor(()=>f.events.some(e=>e.type==='READY'));
    const source='sound.tone(1000);matrix.show_number(25);ports.usbsw_write(1);vTaskDelay(100);sound.off();';
    f.send('LOAD_PROGRAM',{source,scene:0});f.send('RUN',{},'overlap');
    await waitFor(()=>f.events.some(e=>e.type==='OUTPUT'&&e.payload.displayNumber===25&&e.payload.usb===1));
    assert(f.events.some(e=>e.type==='BUZZER'&&e.payload.playing&&e.payload.frequency===1000));
    await waitFor(()=>f.events.some(e=>e.type==='RUN_ENDED'));
    f.send('RESET');
    f.events.length=0;
    f.send('LOAD_PROGRAM',{source:'ports.usbsw_write(0/0);',scene:0});f.send('RUN',{},'nan-output');
    await waitFor(()=>f.events.some(e=>e.type==='RUN_ENDED'&&e.payload.reason==='error'));
    assert(!f.events.some(e=>e.type==='OUTPUT'&&e.payload.usb===1));
  } finally {f.close();}
});

test('original analog API reads all four Unity callback values independently of digital inputs and on-board light', async()=>{
  const f=runtimeFixture();
  try {
    await waitFor(()=>f.events.some(event=>event.type==='READY'));
    f.element('TextLightSensor').textContent='17';
    for(let n=1;n<=4;n++){
      f.element('TextInput'+n).textContent=String(n%2);
      f.element('TextAnalogInput'+n).textContent=String([0,123.75,2048,4095][n-1]);
    }
    const source='matrix.show_number(ldr.get());vTaskDelay(60);'+[1,2,3,4].map(n=>'matrix.show_number(kb_specio.input'+n+'_read());vTaskDelay(60);matrix.show_number(ports.input'+n+'_read());vTaskDelay(60);').join('');
    f.send('LOAD_PROGRAM',{source,scene:0});f.send('RUN',{},'analog-all');
    await waitFor(()=>f.events.some(event=>event.type==='RUN_ENDED'));
    assert.deepEqual(f.events.filter(event=>event.type==='OUTPUT'&&event.payload.displayNumber!==null).map(event=>event.payload.displayNumber),[17,0,1,123,0,2048,1,4095,0]);
    assert(!f.events.some(event=>event.type==='ERROR'));
    assert.throws(()=>core.validate('kb_specio.input5_read();',sandbox.acorn));
    assert.throws(()=>core.validate('var kb_specio=0;',sandbox.acorn));
    assert.throws(()=>core.validate('kb_specio.input1_read=0;',sandbox.acorn));
  } finally {f.close();}
});

test('analog changes affect the same run and Stop cancels further reads and permits a fresh run',async()=>{
  const f=runtimeFixture();
  try {
    await waitFor(()=>f.events.some(event=>event.type==='READY'));
    f.element('TextAnalogInput1').textContent='100';
    f.send('LOAD_PROGRAM',{source:'while(1){matrix.show_number(kb_specio.input1_read());vTaskDelay(20);}',scene:0});f.send('RUN',{},'analog-live');
    await waitFor(()=>f.events.some(event=>event.type==='OUTPUT'&&event.payload.displayNumber===100));
    f.element('TextAnalogInput1').textContent='3000';
    await waitFor(()=>f.events.some(event=>event.type==='OUTPUT'&&event.payload.displayNumber===3000));
    f.send('STOP',{},'analog-live');
    const count=f.events.filter(event=>event.type==='OUTPUT').length;
    f.element('TextAnalogInput1').textContent='4095';
    await new Promise(resolve=>setTimeout(resolve,50));
    assert.equal(f.events.filter(event=>event.type==='OUTPUT').length,count);
    f.send('RUN',{},'analog-retry');
    await waitFor(()=>f.events.some(event=>event.type==='OUTPUT'&&event.runId==='analog-retry'&&event.payload.displayNumber===4095));
  } finally {f.close();}
});

test('missing malformed nonfinite and out-of-range analog values fail visibly and do not leave a buzzer playing',async()=>{
  for(const value of ['', 'NaN', 'Infinity', '-1', '4096', '12oops']){
    const f=runtimeFixture();
    try {
      await waitFor(()=>f.events.some(event=>event.type==='READY'));
      f.element('TextAnalogInput1').textContent=value;
      f.send('LOAD_PROGRAM',{source:'sound.tone(1000);matrix.show_number(kb_specio.input1_read());',scene:0});f.send('RUN',{},'bad-analog');
      await waitFor(()=>f.events.some(event=>event.type==='RUN_ENDED'));
      assert.equal(f.events.find(event=>event.type==='RUN_ENDED').payload.reason,'error',JSON.stringify(value));
      assert.equal(f.events.filter(event=>event.type==='BUZZER').at(-1).payload.playing,false);
    } finally {f.close();}
  }
});
