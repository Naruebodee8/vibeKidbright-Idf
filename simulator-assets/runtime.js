/* Controlled browser interpreter -> reference Unity bridge. No eval of program source. */
(function () {
  'use strict';
  const core = window.KidBrightRuntimeCore;
  const settings = new URLSearchParams(location.hash.slice(1));
  const parentOrigin = settings.get('parentOrigin');
  const sessionId = settings.get('sessionId');
  if (!parentOrigin || !sessionId || parent === window) return;
  const version = 1;
  let ready = false, loaded = null, active = null, armed = null, unityRunning = false;
  const state = { out1: 0, out2: 0, usb: 0, matrix: Array(128).fill(0).join(','), displayNumber: null };
  let temperatureOverride = null;
  const buzzer = { playing: false, frequency: 0, volume: 50 };
  function buzzerOutput(runId) { send('BUZZER', { ...buzzer }, runId || (active && active.id)); }
  function buzzerOff(runId) { buzzer.playing = false; buzzer.frequency = 0; buzzerOutput(runId); }
  function buzzerTone(frequency) {
    if (typeof frequency !== 'number' || !Number.isFinite(frequency) || frequency < 20 || frequency > 20000) throw new Error('sound.tone requires a frequency of 20..20000 Hz.');
    buzzer.frequency = frequency; buzzer.playing = true; buzzerOutput();
  }
  const ownedTimers = new Set();
  const ended = new Set();
  let renderTimer = null, lastOutput = '';
  let sceneBootstrapTimer = null;
  let startTimer = null;
  function cancelStartTimer() {
    clearTimeout(startTimer);
    startTimer = null;
  }
  function requestUnityStart(pending) {
    if (armed !== pending) return;
    if (Date.now() >= pending.deadline) {
      finish('error', 'Unity ไม่ตอบรับการเริ่มโปรแกรมภายใน 15 วินาที กรุณากด Run ใหม่หรือปิดแล้วเปิด Simulator');
      return;
    }
    // ClickRun/ClickStop are UIcontrol methods present in this pinned build's
    // IL2CPP metadata and verified against its real Call_RunProgram callback.
    // Retry while the splash/scene transition has not created the receiver yet.
    try { unity('ClickRun', '', '$[UIcontroller]'); }
    catch (error) { finish('error', error.message); return; }
    if (armed === pending) startTimer = setTimeout(function () { requestUnityStart(pending); }, 250);
  }
  function stopSceneBootstrap() {
    clearTimeout(sceneBootstrapTimer);
    sceneBootstrapTimer = null;
  }
  function bootstrapScene() {
    // This legacy build has no scene-ready acknowledgement. Its original host
    // repeated SceneGame every 1 ms. Retry at 250 ms, bounded to one minute,
    // so a message sent during the Unity splash screen is not lost.
    const deadline = Date.now() + 60000;
    function select() {
      if (unityRunning || Date.now() >= deadline) { stopSceneBootstrap(); return; }
      unity('SceneGame', 'SceneKV', '[SceneManagement]');
      unity('SceneExc', '0', '$[UIcontroller]');
      unity('MessageLanguage', '1');
      sceneBootstrapTimer = setTimeout(select, 250);
    }
    select();
  }
  function send(type, payload, runId, requestId) {
    parent.postMessage({ protocolVersion: version, sessionId, type, payload: payload || {}, runId: runId || null, requestId: requestId || null }, parentOrigin);
  }
  function later(fn, delay, run) {
    const id = setTimeout(function () {
      ownedTimers.delete(id);
      if (active === run) { try { fn(); } catch (error) { finish('error', error.message); } }
    }, delay);
    ownedTimers.add(id);
  }
  function unity(method, value, target) {
    if (ready) gameInstance.SendMessage(target || '$[IDEcomunication]', method, value);
  }
  function flushOutput() {
    const signature = JSON.stringify(state);
    if (signature === lastOutput) return;
    lastOutput = signature;
    unity('MessageWriteIO1', state.out1 ? 'Output1_On' : 'Output1_Off');
    unity('MessageWriteIO2', state.out2 ? 'Output2_On' : 'Output2_Off');
    unity('MessageWriteUSB', state.usb ? 'USB_On' : 'USB_Off');
    unity('MessageLED', state.matrix);
    send('OUTPUT', { ...state }, active && active.id);
  }
  function output() {
    if (renderTimer === null) renderTimer = setTimeout(function () { renderTimer = null; flushOutput(); }, 33);
  }
  function finish(reason, message) {
    const run = active || armed;
    const wasArmed = armed !== null;
    if (run) buzzerOff(run.id);
    cancelStartTimer();
    if (renderTimer !== null) clearTimeout(renderTimer);
    renderTimer = null;
    try { flushOutput(); }
    catch (error) { reason = 'error'; message = message || error.message; }
    active = null;
    armed = null;
    for (const id of ownedTimers) clearTimeout(id);
    ownedTimers.clear();
    document.getElementById('TextRun').textContent = '0';
    // Clear JS ownership before calling Unity: ClickStop can synchronously
    // call back into Call_RunProgram(0). Late Play callbacks cannot restart it.
    try { if (unityRunning || wasArmed) unity('ClickStop', '', '$[UIcontroller]'); }
    catch (error) { reason = 'error'; message = message || error.message; }
    // A terminal event belongs to exactly one run; repeated Stop is harmless.
    if (run && !ended.has(run.id)) {
      ended.add(run.id);
      send('RUN_ENDED', { reason, message: message || '', sourceRevision: run.revision }, run.id);
    }
  }
  function reset() {
    finish('stopped');
    state.out1 = state.out2 = state.usb = 0;
    state.matrix = Array(128).fill(0).join(',');
    state.displayNumber = null;
    output();
  }
  function sensor(id) {
    if (id === 'TextTempSensor' && temperatureOverride !== null) return temperatureOverride;
    const text = document.getElementById(id).textContent;
    if (!text.trim()) throw new Error('Missing input: ' + id);
    const value = Number(text);
    if (!Number.isFinite(value)) throw new Error('Invalid input: ' + id);
    return value;
  }
  function start(runId) {
    if (!ready || !loaded) throw new Error('Load a simulation program after Unity is ready.');
    if (active) throw new Error('A simulation is already running.');
    if (!runId || ended.has(runId)) throw new Error('Use a new run ID.');
    const run = { id: runId, revision: loaded.revision, steps: 0, started: Date.now(), interpreter: null };
    buzzer.playing = false; buzzer.frequency = 0; buzzer.volume = 50;
    const interpreter = new Interpreter(loaded.source, function (it, scope) {
      const objects = {};
      function expose(path, fn, async) {
        const parts = path.split('.');
        let owner = scope;
        if (parts.length === 2) {
          if (!objects[parts[0]]) {
            objects[parts[0]] = it.createObject(it.OBJECT);
            it.setProperty(scope, parts[0], objects[parts[0]]);
          }
          owner = objects[parts[0]];
        }
        it.setProperty(owner, parts[parts.length - 1], async ? it.createAsyncFunction(fn) : it.createNativeFunction(function () {
          return it.createPrimitive(fn.apply(null, arguments));
        }));
      }
      it.setProperty(scope, 'portTICK_RATE_MS', it.createPrimitive(1));
      expose('vTaskDelay', function (ms, callback) {
        if (!Number.isFinite(ms) || ms < 0 || ms > 60000) throw new Error('Delay must be between 0 and 60,000 ms.');
        later(callback, ms, run);
      }, true);
      for (const [name, key] of [['output1', 'out1'], ['output2', 'out2'], ['usbsw', 'usb']]) {
        expose('ports.' + name + '_write', function (value) { const numeric=Number(value); if(!Number.isFinite(numeric))throw new Error('Output requires a finite number or boolean.'); state[key] = numeric === 0 ? 0 : 1; output(); });
        expose('ports.' + name + '_read', function () { return state[key]; });
      }
      for (let n = 1; n <= 4; n++) {
        expose('ports.input' + n + '_read', function () { return sensor('TextInput' + n) === 0 ? 0 : 1; });
        // Original host: kb_specio.inputN_read -> Call_AnalogInputN -> TextAnalogInputN.
        // Keep this separate from Digital INN and the on-board LDR. No unit conversion.
        expose('kb_specio.input' + n + '_read', function () {
          const raw = sensor('TextAnalogInput' + n);
          if (raw < 0 || raw > 4095) throw new Error('Analog IN' + n + ' requires a raw value of 0..4095.');
          return Math.trunc(raw);
        });
      }
      for (let n = 1; n <= 2; n++) {
        expose('button12.sw' + n + '_get', function () { return sensor('TextSwitch' + n) === 0 ? 0 : 1; });
        expose('button12.is_sw' + n + '_pressed', function () { return sensor('TextSwitch' + n) !== 0; });
        expose('button12.is_sw' + n + '_released', function () { return sensor('TextSwitch' + n) === 0; });
      }
      expose('sound.note', function (note) { buzzerTone(core.noteFrequency(note)); });
      expose('sound.tone', buzzerTone); // Local Hz extension for ESP-IDF PWM.
      expose('sound.off', function () { buzzerOff(); });
      expose('sound.get_volume', function () { return buzzer.volume; });
      expose('sound.set_volume', function (volume) {
        if (typeof volume !== 'number' || !Number.isFinite(volume) || volume < 0 || volume > 100) throw new Error('sound.set_volume requires 0..100.');
        buzzer.volume = volume; buzzerOutput();
      });
      expose('sound.rest', function (duration, callback) {
        const ms = core.noteDuration(duration);
        later(function () { buzzerOff(); callback(); }, ms, run);
      }, true);
      expose('ldr.get', function () { return sensor('TextLightSensor'); });
      expose('lm73_1.get', function () { return sensor('TextTempSensor'); });
      expose('ht16k33.show', function (buffer) { state.matrix = core.matrixCsv(buffer); state.displayNumber = null; output(); });
      expose('matrix.show_number', function (value) { state.matrix = core.matrixCsv(core.numberBuffer(value)); state.displayNumber = Math.round(value); output(); });
      expose('ht16k33.idle', function () { return true; });
      expose('ht16k33.wait_idle', function (callback) { later(callback, 0, run); }, true);
    });
    run.interpreter = interpreter;
    cancelStartTimer();
    armed = null;
    active = run;
    lastOutput = ''; // Unity Stop can clear the board even if the next values match.
    document.getElementById('TextRun').textContent = '1';
    send('RUN_STARTED', { sourceRevision: run.revision }, run.id);
    function step() {
      if (active !== run) return;
      const deadline = performance.now() + 6;
      do {
        if (++run.steps > 1000000 || Date.now() - run.started > 300000) throw new Error('Simulation limit reached. Stop or simplify the program.');
        if (!interpreter.step()) { finish('completed'); return; }
        if (interpreter.paused_) break;
      } while (performance.now() < deadline);
      later(step, interpreter.paused_ ? 10 : 0, run);
    }
    later(step, 0, run);
  }
  // Retain the reference callback names used by the compiled Unity build.
  const originalRun = window.Call_RunProgram;
  window.Call_RunProgram = function (value) {
    originalRun(value);
    unityRunning = Number(value) === 1;
    if (unityRunning) stopSceneBootstrap();
    try {
      if (unityRunning && armed && !active) start(armed.id);
      else if (unityRunning && !active) unity('ClickStop', '', '$[UIcontroller]');
      else if (!unityRunning && active) finish('stopped');
    } catch (error) { finish('error', error.message); }
  };
  window.Call_CloseSim = function () { finish('stopped'); send('CLOSE_REQUESTED'); };
  window.addEventListener('message', function (event) {
    if (event.source !== parent || event.origin !== parentOrigin) return;
    const msg = event.data;
    if (!msg || msg.protocolVersion !== version || msg.sessionId !== sessionId || typeof msg.requestId !== 'string') return;
    try {
      if (msg.type === 'HELLO') {
        if (ready) send('READY', { engine: 'zip-unity-prototype-1', apis: core.APIS, scenes: [0] }, null, msg.requestId);
        return;
      }
      if (!ready) throw new Error('Unity is still loading.');
      if (msg.type === 'SET_INPUTS') {
        const temperature = msg.payload && msg.payload.temperature;
        if (temperature !== null && (typeof temperature !== 'number' || !Number.isFinite(temperature) || temperature < -100 || temperature > 150)) throw new Error('Test temperature must be -100 to 150, or null to use Unity.');
        temperatureOverride = temperature;
        send('INPUTS_UPDATED', { temperature, source: temperature === null ? 'unity' : 'local-test' }, null, msg.requestId);
      } else if (msg.type === 'LOAD_PROGRAM') {
        if (active || armed) throw new Error('Stop before loading another program.');
        if (!msg.payload || msg.payload.scene !== 0) throw new Error('This prototype supports only the KidBright board scene.');
        core.validate(msg.payload.source, acorn, { requireOutput: msg.payload.projectCompanion === true });
        loaded = { source: msg.payload.source, revision: String(msg.payload.sourceRevision || '') };
        unity('SceneGame', 'SceneKV', '[SceneManagement]');
        unity('SceneExc', '0', '$[UIcontroller]');
        send('PROGRAM_LOADED', { sourceRevision: loaded.revision }, null, msg.requestId);
      } else if (msg.type === 'RUN') {
        if (!loaded || active || armed || typeof msg.runId !== 'string' || !msg.runId || ended.has(msg.runId)) throw new Error('Load a program and use a new run ID after stopping.');
        armed = { id: msg.runId, revision: loaded.revision, deadline: Date.now() + 15000 };
        send('RUN_ARMED', {}, msg.runId, msg.requestId);
        requestUnityStart(armed);
      }
      else if (msg.type === 'STOP') {
        if (msg.runId && (active || armed) && msg.runId !== (active || armed).id) throw new Error('Stop belongs to an earlier run.');
        finish('stopped'); send('ACK', {}, msg.runId, msg.requestId);
      }
      else if (msg.type === 'RESET') { reset(); send('RESET_DONE', {}, null, msg.requestId); }
      else if (msg.type === 'DISPOSE') { stopSceneBootstrap(); finish('stopped'); loaded = null; send('DISPOSED', {}, null, msg.requestId); }
      else throw new Error('Unsupported command.');
    } catch (error) {
      send('ERROR', { message: error.message }, msg.runId, msg.requestId);
    }
  });
  window.addEventListener('beforeunload', function () { stopSceneBootstrap(); finish('stopped'); });
  window.addEventListener('error', function (event) { send('ERROR', { message: event.message || 'Simulator failed.' }); });
  // No fixed-delay polling, cloud calls or project storage from the reference host.
  const parser = document.createElement('script');
  parser.src = '../JS-Interpreter/acorn.js';
  parser.onload = function () {
    const script = document.createElement('script');
    script.src = '../JS-Interpreter/interpreter.js';
    script.onload = function () {
      try {
        gameInstance = UnityLoader.instantiate('gameContainer', 'Build/KV.json', { onProgress: function (instance, progress) {
          UnityProgress(instance, progress);
          send('LOADING', { percent: Math.round(progress * 100) });
          if (progress === 1 && !ready) {
            ready = true;
            window.KidBrightRuntimeReady = true;
            bootstrapScene();
            send('READY', { engine: 'zip-unity-prototype-1', apis: core.APIS, scenes: [0] });
          }
        } });
      } catch (error) { send('ERROR', { message: error.message }); }
    };
    script.onerror = parser.onerror = function () { send('ERROR', { message: 'Cannot load the simulation interpreter.' }); };
    document.head.appendChild(script);
  };
  document.head.appendChild(parser);
})();
