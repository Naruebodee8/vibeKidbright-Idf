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

  const project=process.env.VIBE_PROBE_FAN_PROJECT;
  const source=project ? fs.readFileSync(path.join(project,'simulation/main.sim.js'),'utf8').split('\n').slice(1).join('\n') : JSON.parse(fs.readFileSync('simulator-tests/fan-recipes.json','utf8')).usb.source;
  assert(source.includes('ports.usbsw_write('));assert(!source.includes('ports.output1_write('));
  await page.evaluate(source=>{window.__simProbe.companionSource=source;window.__simProbe.events=[];},source);
  await page.getByLabel('Run project simulator').click();
  await page.waitForFunction(()=>{const b=[...document.querySelectorAll('button')].find(b=>b.textContent.trim()==='Run Simulator');return b&&!b.disabled;},null,{timeout:180000});
  const child=page.frames().find(frame=>frame.url().startsWith(unityOrigin));assert(child);
  await child.evaluate(()=>{
    window.__fanInputs=[];
    const sw1=window.Call_SW1;
    window.Call_SW1=function(value){window.__fanInputs.push(Number(value));return sw1(value);};
  });
  // Wait for Unity to enter the board scene, then pause through its own UI.
  await page.getByRole('button',{name:'Run Simulator',exact:true}).click();
  await page.waitForFunction(()=>window.__simProbe.events.some(e=>e.type==='RUN_STARTED'));
  await page.waitForTimeout(1000);
  await page.mouse.click(1515,806);
  await page.waitForFunction(()=>window.__simProbe.events.some(e=>e.type==='RUN_ENDED'));
  await page.waitForTimeout(700);
  // Add and connect the reference USB fan through Unity's own menu/mouse input.
  await page.mouse.click(489,253);await page.waitForTimeout(300);
  await page.screenshot({path:'src-tauri/target/fan-device-menu.png'});
  await page.mouse.click(480,446);await page.waitForTimeout(500);
  await page.mouse.move(1355,549);await page.mouse.down();
  await page.mouse.move(1163,581,{steps:20});await page.mouse.up();await page.waitForTimeout(500);
  await page.screenshot({path:'src-tauri/target/fan-device-attached.png'});
  await page.evaluate(()=>window.__simProbe.events=[]);
  await page.getByRole('button',{name:'Run Simulator',exact:true}).click();
  await page.waitForFunction(()=>window.__simProbe.events.some(e=>e.type==='RUN_STARTED'));
  await page.waitForFunction(()=>window.__simProbe.events.some(e=>e.type==='OUTPUT'&&e.payload.usb===0));
  await page.screenshot({path:'src-tauri/target/fan-usb-off.png'});
  async function press(){await page.mouse.move(922,583);await page.mouse.down();await page.waitForTimeout(650);await page.mouse.up();await page.waitForTimeout(650);}
  await press();
  await page.waitForFunction(()=>window.__simProbe.events.filter(e=>e.type==='OUTPUT').at(-1)?.payload.usb===1);
  await page.screenshot({path:'src-tauri/target/fan-usb-on.png'});
  const onFrames=[];
  for(let i=0;i<3;i++){await page.waitForTimeout(120);onFrames.push((await page.screenshot({clip:{x:1090,y:535,width:155,height:90}})).toString('base64'));}
  assert(new Set(onFrames).size>1,'USB fan must visibly animate while the code enables USB');
  await press();
  await page.waitForFunction(()=>window.__simProbe.events.filter(e=>e.type==='OUTPUT').at(-1)?.payload.usb===0);
  const actual=await page.evaluate(()=>window.__simProbe.events.filter(e=>e.type==='OUTPUT').map(e=>e.payload));
  assert.deepEqual(actual.map(p=>p.usb),[0,1,0]);
  assert(actual.every(p=>p.out1===0&&p.out2===0));
  assert.deepEqual(await child.evaluate(()=>window.__fanInputs),[1,0,1,0]);
  await page.getByRole('button',{name:'Stop',exact:true}).click();
  await page.getByLabel('Close Simulator').click();
  assert.equal(pageErrors.length,0,JSON.stringify(pageErrors));assert.equal(external.length,0,JSON.stringify(external));
  const report={real_compiled_ui:true,real_unity:true,actual_project_companion:!!project,mouse_sw1_two_presses:true,usb_sequence:[0,1,0],held_press_single_toggle:true,other_outputs_stay_off:true,visible_usb_fan_animation:true,external_requests:external,native_ipc_fixture:true,ai_called:false,hardware_flashed:false};
  fs.writeFileSync('src-tauri/target/fan-ui-verification.json',JSON.stringify(report,null,2)+'\n');console.log(JSON.stringify(report));
} finally {await browser?.close();await Promise.all([appServer,unityServer].map(server=>new Promise(resolve=>server.close(resolve))));}
