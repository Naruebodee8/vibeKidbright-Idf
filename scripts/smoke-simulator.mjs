// Runs the supplied Unity build in a separate headless browser/profile. No IDE data is used.
import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import { spawn } from 'node:child_process';
import { once } from 'node:events';
import assert from 'node:assert/strict';

const root = process.cwd();
const assets = path.join(root, 'simulator-assets');
const profile = fs.mkdtempSync(path.join(os.tmpdir(), 'kidbright-sim-smoke-'));
const screenshot = path.join(root, 'simulator-smoke.png');
let browser, socket, unityServer, parentServer;
const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));
async function listen(server) { server.listen(0, '127.0.0.1'); await once(server, 'listening'); return `http://127.0.0.1:${server.address().port}`; }
try {
  let parentOrigin;
  unityServer = http.createServer((req, res) => {
    const file = path.resolve(assets, '.' + new URL(req.url, 'http://localhost').pathname);
    if (!file.startsWith(assets + path.sep) || !fs.existsSync(file) || !fs.statSync(file).isFile()) { res.writeHead(404).end(); return; }
    const kind = file.endsWith('.wasm.code.unityweb') ? 'application/wasm' : file.endsWith('.js') || file.endsWith('.framework.unityweb') ? 'text/javascript' : file.endsWith('.html') ? 'text/html' : file.endsWith('.json') ? 'application/json' : file.endsWith('.css') ? 'text/css' : 'application/octet-stream';
    res.writeHead(200, { 'Content-Type': kind, 'Content-Security-Policy': `default-src 'self'; script-src 'self' 'unsafe-inline' 'unsafe-eval' blob:; style-src 'self' 'unsafe-inline'; img-src 'self' data:; connect-src 'self'; worker-src 'self' blob:; frame-ancestors ${parentOrigin}` });
    fs.createReadStream(file).pipe(res);
  });
  const unityOrigin = await listen(unityServer);
  const session = 'smoke-session';
  const src = `${unityOrigin}/UnityBuild/index.html#${new URLSearchParams({ parentOrigin: 'PLACEHOLDER', sessionId: session })}`;
  parentServer = http.createServer((_req, res) => {
    const frameUrl = src.replace('PLACEHOLDER', encodeURIComponent(parentOrigin));
    res.writeHead(200, { 'Content-Type': 'text/html' }).end(`<!doctype html><html><body style="margin:0;background:#142033;color:white"><p id="status">Simulator smoke test</p><iframe id="sim" title="simulator" style="width:1280px;height:720px;border:0" sandbox="allow-scripts allow-same-origin" src="${frameUrl}"></iframe><script>
window.events=[];
window.addEventListener('message',e=>{if(e.origin===${JSON.stringify(unityOrigin)}&&e.source===document.getElementById('sim').contentWindow){events.push(e.data);document.getElementById('status').textContent=e.data.type;}});
window.command=(type,payload={},runId=null)=>{const requestId=crypto.randomUUID();document.getElementById('sim').contentWindow.postMessage({protocolVersion:1,sessionId:${JSON.stringify(session)},requestId,runId,type,payload},${JSON.stringify(unityOrigin)});return requestId;};
</script></body></html>`);
  });
  parentOrigin = await listen(parentServer);
  const executable = process.env.SIMULATOR_TEST_BROWSER || ['C:/Program Files/Google/Chrome/Application/chrome.exe', 'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe'].find(file => fs.existsSync(file));
  if (!executable) throw Error('Set SIMULATOR_TEST_BROWSER to an installed Chromium browser.');
  browser = spawn(executable, ['--headless=new', '--remote-debugging-port=0', `--user-data-dir=${profile}`, '--no-first-run', '--no-default-browser-check', '--disable-background-networking', '--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--autoplay-policy=no-user-gesture-required', 'about:blank'], { windowsHide: true, stdio: 'ignore' });
  const portFile = path.join(profile, 'DevToolsActivePort');
  for (let i = 0; i < 100 && !fs.existsSync(portFile); i++) await sleep(100);
  const [port, endpoint] = fs.readFileSync(portFile, 'utf8').trim().split('\n');
  socket = new WebSocket(`ws://127.0.0.1:${port}${endpoint}`);
  await once(socket, 'open');
  let sequence = 0;
  const requests = new Map();
  const errors = [];
  const contexts = new Map();
  socket.addEventListener('message', event => {
    const msg = JSON.parse(event.data);
    if (msg.method === 'Runtime.executionContextCreated') contexts.set(msg.params.context.id, msg.params.context);
    if (msg.id && requests.has(msg.id)) { const { resolve, reject } = requests.get(msg.id); requests.delete(msg.id); msg.error ? reject(Error(JSON.stringify(msg.error))) : resolve(msg.result); }
    if (msg.method === 'Runtime.exceptionThrown') errors.push(msg.params.exceptionDetails.text + ' ' + (msg.params.exceptionDetails.exception?.description || ''));
    if (msg.method === 'Runtime.consoleAPICalled' && ['error', 'warning'].includes(msg.params.type)) errors.push(msg.params.args.map(a => a.value || a.description || '').join(' '));
  });
  function cdp(method, params = {}, sessionId) { return new Promise((resolve, reject) => { const id = ++sequence; requests.set(id, { resolve, reject }); socket.send(JSON.stringify({ id, method, params, sessionId })); }); }
  const { targetId } = await cdp('Target.createTarget', { url: 'about:blank' });
  const { sessionId } = await cdp('Target.attachToTarget', { targetId, flatten: true });
  await cdp('Runtime.enable', {}, sessionId);
  await cdp('Page.enable', {}, sessionId);
  await cdp('Emulation.setDeviceMetricsOverride', { width: 1366, height: 900, deviceScaleFactor: 1, mobile: false }, sessionId);
  await cdp('Page.navigate', { url: parentOrigin }, sessionId);
  async function evaluate(expression) {
    const result = await cdp('Runtime.evaluate', { expression, returnByValue: true, awaitPromise: true }, sessionId);
    if (result.exceptionDetails) throw Error(JSON.stringify(result.exceptionDetails));
    return result.result.value;
  }
  async function until(expression, milliseconds = 10000) {
    const start = Date.now();
    while (Date.now() - start < milliseconds) { const result = await evaluate(expression); if (result) return result; await sleep(100); }
    const image = await cdp('Page.captureScreenshot', { format: 'png', captureBeyondViewport: true }, sessionId);
    fs.writeFileSync(screenshot, Buffer.from(image.data, 'base64'));
    throw Error('Timeout: ' + expression + '\nBrowser diagnostics: ' + errors.slice(-8).join('\n') + '\nEvents: ' + JSON.stringify(await evaluate('window.events?.slice(-8)')));
  }
  console.log('Waiting for the actual Unity build (up to 180 seconds)…');
  await until('window.events?.some(e=>e.type==="READY")', 180000);
  console.log('Unity READY received.');
  await sleep(8000); // Allow the supplied Unity splash screen and scene transition to finish.
  const unityContext = [...contexts.values()].find(context => context.origin === unityOrigin && context.auxData?.isDefault);
  assert(unityContext, 'Unity iframe has a browser execution context');
  async function evaluateUnity(expression) {
    const result = await cdp('Runtime.evaluate', { expression, contextId: unityContext.id, returnByValue: true }, sessionId);
    if (result.exceptionDetails) throw Error(JSON.stringify(result.exceptionDetails));
    return result.result.value;
  }
  console.log('Unity input defaults:', await evaluateUnity("['TextTempSensor','TextLightSensor','TextSwitch1','TextSwitch2'].map(id=>[id,document.getElementById(id).textContent])"));
  async function command(type, payload = {}, runId = null) {
    const id = await evaluate(`command(${JSON.stringify(type)},${JSON.stringify(payload)},${JSON.stringify(runId)})`);
    const event = await until(`events.find(e=>e.requestId===${JSON.stringify(id)})`);
    if (event.type === 'ERROR') throw Error(event.payload.message);
    return event;
  }
  const frameRect = await evaluate("(()=>{const r=document.getElementById('sim').getBoundingClientRect();return {x:r.x,y:r.y};})()");
  const canvasRect = await evaluateUnity("(()=>{const r=document.querySelector('canvas').getBoundingClientRect();return {x:r.x,y:r.y,width:r.width,height:r.height};})()");
  console.log('Canvas:', canvasRect);
  async function mouse(type, x, y) {
    await cdp('Input.dispatchMouseEvent', { type, x: frameRect.x + canvasRect.x + canvasRect.width*x/1280, y: frameRect.y + canvasRect.y + canvasRect.height*y/720, button: 'left', clickCount: 1 }, sessionId);
  }
  await command('LOAD_PROGRAM', { source: 'ports.output2_write(1);vTaskDelay(500);ports.output2_write(0);', scene: 0, sourceRevision: 'smoke-1' });
  assert.equal((await command('RUN', {}, 'run-1')).type, 'RUN_ARMED');
  await until('events.some(e=>e.type==="RUN_ENDED"&&e.runId==="run-1"&&e.payload.reason==="completed")');
  assert(await evaluate('events.some(e=>e.type==="OUTPUT"&&e.runId==="run-1"&&e.payload.out2===1)'));
  console.log('OUT2 + async delay + completion passed.');
  await command('LOAD_PROGRAM', { source: 'ports.output1_write(1);vTaskDelay(10000);ports.output1_write(0);', scene: 0, sourceRevision: 'smoke-2' });
  await command('RUN', {}, 'run-2');
  await sleep(100);
  await command('STOP', {}, 'run-2');
  await until('events.some(e=>e.type==="RUN_ENDED"&&e.runId==="run-2"&&e.payload.reason==="stopped")');
  await command('LOAD_PROGRAM', { source: 'ports.usbsw_write(1);', scene: 0, sourceRevision: 'smoke-3' });
  await command('RUN', {}, 'run-3');
  await until('events.some(e=>e.type==="RUN_ENDED"&&e.runId==="run-3"&&e.payload.reason==="completed")');
  assert.equal(await evaluate('events.filter(e=>e.type==="RUN_ENDED"&&e.runId==="run-2").length'), 1);
  console.log('Stop during delay, cleanup and another Run passed.');
  await command('LOAD_PROGRAM', { source: 'while(1){ports.output2_write(button12.is_sw1_pressed()?1:0);vTaskDelay(50);}', scene: 0, sourceRevision: 'smoke-button' });
  await command('RUN', {}, 'run-button');
  await mouse('mouseMoved',600,395); await mouse('mousePressed',600,395);
  await until('events.some(e=>e.type==="OUTPUT"&&e.runId==="run-button"&&e.payload.out2===1)', 4000);
  await mouse('mouseReleased',600,395);
  await command('STOP', {}, 'run-button');
  console.log('Actual Unity S1 press -> interpreter -> OUT2 passed.');
  const image = await cdp('Page.captureScreenshot', { format: 'png', captureBeyondViewport: true }, sessionId);
  fs.writeFileSync(screenshot, Buffer.from(image.data, 'base64'));
  console.log('Screenshot:', screenshot);
  if (errors.length) console.log('Browser diagnostics:', JSON.stringify(errors.slice(-12)));
  assert(!errors.some(error => /SendMessage.*(not found|does not exist)|Could not find.*(object|method)/i.test(error)), 'Unity device target/method error');
  console.log('Simulator browser smoke checks passed.');
} finally {
  socket?.close();
  if (browser?.pid) {
    if (process.platform === 'win32') { const kill = spawn('taskkill.exe', ['/PID', String(browser.pid), '/T', '/F'], { windowsHide: true, stdio: 'ignore' }); await once(kill, 'exit'); }
    else browser.kill();
  }
  unityServer?.closeAllConnections(); unityServer?.close();
  parentServer?.closeAllConnections(); parentServer?.close();
  // The unique test profile is the only recursively removed directory.
  const resolved = fs.realpathSync(profile);
  if (path.dirname(resolved) === fs.realpathSync(os.tmpdir()) && path.basename(resolved).startsWith('kidbright-sim-smoke-')) {
    fs.rmSync(resolved, { recursive: true, force: true, maxRetries: 5, retryDelay: 200 });
  }
}
