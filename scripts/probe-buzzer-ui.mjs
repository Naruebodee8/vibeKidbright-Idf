// Real compiled IDE + Unity, isolated browser; native/AI/hardware IPC is a fixture.
import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import { createRequire } from 'node:module';
const tools = process.env.VIBE_PROBE_TOOLS;
if (!tools) throw Error('Set VIBE_PROBE_TOOLS to an isolated directory with playwright-core.');
const { chromium } = createRequire(path.resolve(tools,'package.json'))('playwright-core');
const executable=['C:/Program Files/Google/Chrome/Application/chrome.exe','C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe'].find(fs.existsSync);
assert(executable,'A local Chromium browser is required.');
const compilerCases=JSON.parse(fs.readFileSync('simulator-tests/compiler-cases.json','utf8'));
const alphabetSource=process.env.VIBE_PROBE_COMPANION ? fs.readFileSync(process.env.VIBE_PROBE_COMPANION,'utf8') : compilerCases.find(c=>c.name==='alphabet').source;
const root=process.cwd(), config=JSON.parse(fs.readFileSync('src-tauri/tauri.conf.json','utf8'));
const listen=server=>new Promise(resolve=>server.listen(0,'127.0.0.1',()=>resolve(`http://127.0.0.1:${server.address().port}`)));
const kind=file=>file.endsWith('.wasm.code.unityweb')?'application/wasm':file.endsWith('.js')||file.endsWith('.framework.unityweb')?'text/javascript':file.endsWith('.html')?'text/html':file.endsWith('.json')?'application/json':file.endsWith('.css')?'text/css':file.endsWith('.ttf')?'font/ttf':'application/octet-stream';
let appOrigin, browser;
function serve(directory, policy, stripSession=false) {
  return http.createServer((req,res)=>{
    let pathname=new URL(req.url,'http://localhost').pathname;
    if(stripSession)pathname=pathname.replace(/^\/[a-f0-9-]{36}\//,'/');
    if(pathname==='/')pathname='/index.html';
    const file=path.resolve(directory,'.'+decodeURIComponent(pathname));
    if(!file.startsWith(directory+path.sep)||!fs.existsSync(file)||!fs.statSync(file).isFile()){res.writeHead(404).end();return;}
    res.writeHead(200,{'Content-Type':kind(file),'Content-Security-Policy':policy()});
    fs.createReadStream(file).pipe(res);
  });
}
const appServer=serve(path.join(root,'dist'),()=>config.app.security.csp);
const unityServer=serve(path.join(root,'simulator-assets'),()=>`default-src 'self'; script-src 'self' 'unsafe-inline' 'unsafe-eval' blob:; style-src 'self' 'unsafe-inline'; img-src 'self' data:; worker-src 'self' blob:; connect-src 'self'; frame-ancestors ${appOrigin}`,true);
try {
  appOrigin=await listen(appServer); const unityOrigin=await listen(unityServer);
  browser=await chromium.launch({executablePath:executable,headless:true,args:['--use-angle=swiftshader','--enable-unsafe-swiftshader']});
  const page=await browser.newPage({viewport:{width:1600,height:1000}});
  const pageErrors=[],external=[],workers=[];
  page.on('worker',worker=>workers.push(worker.url()));
  page.on('pageerror',error=>pageErrors.push(String(error)));
  await page.route('**/*',route=>{
    const url=route.request().url();
    if(url.startsWith(appOrigin+'/')||url.startsWith(unityOrigin+'/'))return route.continue();
    external.push(url);return route.abort();
  });
  await page.addInitScript(()=>{
    if(window.top!==window)return;
    const originalGain=AudioContext.prototype.createGain,originalOscillator=AudioContext.prototype.createOscillator;
    const contexts=[],known=new Map();window.__audioProbe={contexts};
    function entry(context){
      if(!known.has(context)){
        const analyser=context.createAnalyser();analyser.fftSize=4096;analyser.smoothingTimeConstant=0;
        const item={context,analyser,active:new Set()};known.set(context,item);contexts.push(item);
      }
      return known.get(context);
    }
    AudioContext.prototype.createGain=function(){
      const node=originalGain.call(this),item=entry(this),connect=node.connect.bind(node);
      node.connect=function(destination,...args){const result=connect(destination,...args);if(destination===item.context.destination)connect(item.analyser);return result;};return node;
    };
    AudioContext.prototype.createOscillator=function(){
      const node=originalOscillator.call(this),item=entry(this),start=node.start.bind(node),stop=node.stop.bind(node);
      node.start=function(...args){const result=start(...args);item.active.add(node);return result;};
      node.stop=function(...args){const result=stop(...args);item.active.delete(node);return result;};return node;
    };
  });
  await page.addInitScript(({unityOrigin})=>{
    if(window.top!==window)return;
    localStorage.clear();localStorage.setItem('vibe-project-dir','/fixture/project');localStorage.setItem('vibe-theme','dark');
    const callbacks=new Map(),listeners=new Map();let sequence=0;
    const state=window.__simProbe={calls:[],failSave:false,events:[],installed:false};
    window.addEventListener('message',event=>{if(event.origin===unityOrigin&&event.data?.protocolVersion===1)state.events.push(event.data);});
    const emit=(name,payload)=>{const id=listeners.get(name);if(id)callbacks.get(id)?.({payload});};
    window.__TAURI_EVENT_PLUGIN_INTERNALS__={unregisterListener:()=>{}};
    window.__TAURI_INTERNALS__={metadata:{currentWindow:{label:'main'},currentWebview:{label:'main'}},
      transformCallback:callback=>{const id=++sequence;callbacks.set(id,callback);return id;},unregisterCallback:id=>callbacks.delete(id),
      invoke:async(name,args={})=>{
        state.calls.push({name,args});
        if(name==='plugin:event|listen'){listeners.set(args.event,args.handler);return args.handler;}
        if(name==='plugin:event|unlisten'){listeners.delete(args.event);return;}
        if(name==='get_toolchain_storage')return {required_bytes:12*1024**3,options:[{install_dir:'C:/fixture/toolchain',available_bytes:40*1024**3,sufficient:true,error:null}],recommended_dir:'C:/fixture/toolchain',warning:null};
        if(name==='check_toolchain')return {status:state.installed?'ready':'not_installed',version:'fixture',path:'fixture'};
        if(name==='download_toolchain')throw Error('A test must never download a real toolchain.');
        if(name==='check_esp_idf'){if(!state.installed)throw Error('fixture SDK not installed');return 'Ready: fixture SDK';}
        if(name==='get_idf_custom_paths')return {idf_path:'',tools_path:''};
        if(name==='list_project_files')return [{name:'main.c',path:'/fixture/project/main/main.c',is_dir:false},{name:'main.sim.js',path:'/fixture/project/simulation/main.sim.js',is_dir:false}];
        if(name==='validate_idf_project')return true;
        if(name==='list_serial_ports')return ['COM_GUI_FIXTURE'];
        if(name==='read_project_file')return args.path.endsWith('.sim.js')?'ports.output2_write(1);':'void app_main(void) {}';
        if(name==='check_pending_diff')return state.pendingReview&&args.path.endsWith('/main/main.c')?'void app_main(void) { /* reviewed */ }':null;
        if(name==='list_pending_diffs')return state.pendingReview?[{path:'/fixture/project/main/main.c',relative_path:'main/main.c'}]:[];
        if(name==='accept_diff'){
          if(args.expectedContent!=='void app_main(void) { /* reviewed */ }')throw Error('Unexpected reviewed version');
          state.pendingReview=false;
          emit('file-modified',JSON.stringify({path:args.path}));
          emit('ai-diff-resolved',{path:args.path,action:'keep'});
          return 'kept';
        }
        if(name==='write_project_file'){if(state.failSave)throw Error('fixture save failed');return;}
        if(name==='run_shell_command')return {success:true,code:0};
        if(name==='start_simulator_host')return {url:`${unityOrigin}/${args.sessionId}/UnityBuild/index.html`,origin:unityOrigin,session_id:args.sessionId};
        if(name==='load_project_simulation'&&state.pendingReview)throw Error('มี Diff รอตรวจรับ: main/main.c');
        if(name==='load_project_simulation')return {path:'simulation/main.sim.js',source:state.companionSource||'while(1){matrix.show_number(lm73_1.get());vTaskDelay(250);}',revision:'fixture',firmware_files:['main/main.c']};
        if(name==='get_knowledge_base_files')return [];
        if(name==='get_provider')return 'local';
        if(name==='get_base_url')return 'http://127.0.0.1:1/v1';
        if(name==='send_ai_message'){setTimeout(()=>emit('ai-chat-done',JSON.stringify({history:[]})),50);return;}
        if(name.startsWith('get_'))return '';
        return null;
      }};
  },{unityOrigin});
  await page.goto(appOrigin);
  await page.getByRole('button',{name:'ใช้ Simulator ก่อน'}).click();
  assert.equal(await page.evaluate(()=>window.__simProbe.calls.some(c=>c.name==='download_toolchain')),false);
  await page.getByLabel('Run project simulator').click();
  await page.waitForFunction(()=>window.__simProbe.events.some(e=>e.type==='READY'),{timeout:180000});
  await page.getByRole('button',{name:'Run Simulator',exact:true}).click();
  await page.waitForFunction(()=>window.__simProbe.events.some(e=>e.type==='OUTPUT'&&e.payload.displayNumber===25));
  await page.getByLabel('Close Simulator').click();
  assert(await page.getByRole('button',{name:/Toolchain Required/}).isDisabled());
  await page.evaluate(()=>{window.__simProbe.installed=true;window.__simProbe.events=[];});
  await page.keyboard.press('F5');
  await page.waitForFunction(()=>[...document.querySelectorAll('button')].some(b=>b.textContent.includes('Build & Flash')&&!b.disabled));
  await page.getByLabel('Run project simulator').click();
  const run=page.getByRole('button',{name:'Run Simulator',exact:true});
  await run.waitFor();
  await page.waitForFunction(()=>{const b=[...document.querySelectorAll('button')].find(b=>b.textContent.trim()==='Run Simulator');return b&&!b.disabled;},null,{timeout:180000});
  const child=page.frames().find(frame=>frame.url().startsWith(unityOrigin));assert(child);
  const soundCase=name=>compilerCases.find(c=>c.name===name).source;
  async function loadSound(name){
    await page.evaluate(source=>{window.__simProbe.companionSource=source;window.__simProbe.events=[];},soundCase(name));
    await page.getByLabel('Load project simulation').click();
  }
  const graph=()=>{
    const entry=window.__audioProbe.contexts.at(-1);
    if(!entry)return {peak:0,frequency:0,active:0,state:'missing'};
    const time=new Float32Array(entry.analyser.fftSize);entry.analyser.getFloatTimeDomainData(time);
    const bins=new Float32Array(entry.analyser.frequencyBinCount);entry.analyser.getFloatFrequencyData(bins);
    let best=1;for(let i=2;i<bins.length;i++)if(bins[i]>bins[best])best=i;
    return {peak:Math.max(...time.map(Math.abs)),frequency:best*entry.context.sampleRate/entry.analyser.fftSize,active:entry.active.size,state:entry.context.state};
  };
  await loadSound('buzzer-tone');await run.click();
  await page.waitForFunction(()=>window.__audioProbe.contexts.at(-1)?.active.size>0);
  await page.waitForTimeout(180);
  const tone=await page.evaluate(graph);
  assert.equal(tone.state,'running');assert(tone.peak>0.005,JSON.stringify(tone));assert(Math.abs(tone.frequency-1000)<30,JSON.stringify(tone));
  await page.getByLabel('Mute simulator',{exact:true}).click();await page.waitForTimeout(180);
  const muted=await page.evaluate(graph);assert.equal(muted.active,0);assert(muted.peak<0.0001,JSON.stringify(muted));
  await page.getByLabel('Unmute simulator',{exact:true}).click();await page.waitForFunction(()=>window.__audioProbe.contexts.at(-1).active.size===1);
  await page.getByLabel('Simulator volume').focus();await page.getByLabel('Simulator volume').press('Home');await page.waitForTimeout(120);
  assert.equal((await page.evaluate(graph)).active,0);
  await page.getByLabel('Simulator volume').press('End');await page.waitForFunction(()=>window.__audioProbe.contexts.at(-1).active.size===1);
  await page.getByRole('button',{name:'Stop',exact:true}).click();await page.waitForTimeout(180);
  const stopped=await page.evaluate(graph);assert.equal(stopped.active,0);assert(stopped.peak<0.0001,JSON.stringify(stopped));
  await page.waitForFunction(()=>window.__simProbe.events.some(e=>e.type==='RUN_ENDED'));
  await loadSound('buzzer-notes');await run.click();
  await page.waitForFunction(()=>window.__simProbe.events.some(e=>e.type==='RUN_ENDED'),null,{timeout:5000});
  const notes=await page.evaluate(()=>window.__simProbe.events.filter(e=>e.type==='BUZZER'&&e.payload.playing).map(e=>e.payload.frequency));
  assert.deepEqual(notes,[261,329,391]);assert.equal((await page.evaluate(graph)).active,0);
  await child.evaluate(()=>document.getElementById('TextSwitch1').textContent='0');
  await loadSound('buzzer-button');await run.click();await page.waitForTimeout(120);
  assert.equal((await page.evaluate(graph)).active,0);
  await child.evaluate(()=>document.getElementById('TextSwitch1').textContent='1');
  await page.waitForFunction(()=>window.__audioProbe.contexts.at(-1).active.size>0);
  await page.waitForTimeout(50);const buttonTone=await page.evaluate(graph);assert(buttonTone.peak>0.005,JSON.stringify(buttonTone));
  await child.evaluate(()=>document.getElementById('TextSwitch1').textContent='0');
  await page.waitForFunction(()=>window.__audioProbe.contexts.at(-1).active.size===0);
  await page.getByRole('button',{name:'Reset',exact:true}).click();
  assert.equal((await page.evaluate(graph)).active,0);
  await loadSound('buzzer-tone');await run.click();await page.waitForFunction(()=>window.__audioProbe.contexts.at(-1).active.size===1);
  await page.getByLabel('Close Simulator').click();
  await page.waitForFunction(()=>window.__audioProbe.contexts.every(e=>e.context.state==='closed'&&e.active.size===0));
  assert.equal(external.length,0,JSON.stringify(external));assert.equal(pageErrors.length,0,JSON.stringify(pageErrors));
  const report={compiled_ui:true,real_unity:true,real_web_audio_pcm:true,autoplay_bypass_used:false,tone_1000_hz:tone,muted_silent:muted,stop_silent:stopped,reference_C_E_G:notes,SW1_signal_to_real_audio:true,volume_controls:true,reset:true,close_releases_contexts:true,page_errors:pageErrors,external_requests:external,native_ai_hardware_fixture:true,hardware_flashed:false};
  fs.writeFileSync('src-tauri/target/buzzer-ui-verification.json',JSON.stringify(report,null,2)+'\n');console.log(JSON.stringify(report));

} finally {await browser?.close();await Promise.all([appServer,unityServer].map(server=>new Promise(resolve=>server.close(resolve))));}
