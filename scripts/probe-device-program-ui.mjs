import {spawnSync} from 'node:child_process';
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

  const sensor=name=>({kind:'sensor',name});const binary=(operator,left,right)=>({kind:'binary',operator,left,right});
  const program={version:1,steps:[{op:'repeat',body:[
    {op:'number',value:sensor('temperature')},
    {op:'output',port:'out1',value:binary('||',sensor('s1'),sensor('input1'))},
    {op:'output',port:'out2',value:binary('||',sensor('s2'),sensor('input2'))},
    {op:'output',port:'usb',value:binary('&&',binary('>',sensor('light'),50),binary('||',sensor('input3'),sensor('input4')))},
    {op:'if',condition:sensor('s1'),then:[{op:'tone_start',hz:1000}],else:[{op:'buzzer_off'}]},
    {op:'delay',ms:50}
  ]}],unsupported:[]};
  const artifact=spawnSync(path.join(root,'src-tauri/target/debug/examples/generate-device-program.exe'),[],{input:JSON.stringify({program}),encoding:'utf8',windowsHide:true,maxBuffer:1024*1024});assert.equal(artifact.status,0,artifact.stderr);
  const bundle=JSON.parse(artifact.stdout);assert(bundle.firmware.includes('kb_hw_read(0)'));
  await page.evaluate(source=>{window.__simProbe.companionSource=source;window.__simProbe.events=[];},bundle.source);
  await page.getByLabel('Run project simulator').click();
  await page.waitForFunction(()=>{const b=[...document.querySelectorAll('button')].find(b=>b.textContent.trim()==='Run Simulator');return b&&!b.disabled;},null,{timeout:180000});
  const child=page.frames().find(frame=>frame.url().startsWith(unityOrigin));assert(child);
  await child.evaluate(()=>{
    window.__coverageInputs={TextLightSensor:25,TextInput1:0,TextInput2:0,TextInput3:0,TextInput4:0};
    window.__coverageInputTimer=setInterval(()=>{for(const[id,value]of Object.entries(window.__coverageInputs)){const node=document.getElementById(id);if(node)node.textContent=String(value);}},5);
  });
  await page.getByRole('button',{name:'Run Simulator',exact:true}).click();
  await page.waitForFunction(()=>window.__simProbe.events.some(e=>e.type==='OUTPUT'&&e.payload.displayNumber===25));
  await page.waitForTimeout(500);
  async function mouseButton(x,port){
    await page.mouse.move(x,583);await page.mouse.down();
    await page.waitForFunction(port=>window.__simProbe.events.filter(e=>e.type==='OUTPUT').at(-1)?.payload[port]===1,port);
    if(port==='out1')await page.waitForFunction(()=>window.__simProbe.events.filter(e=>e.type==='BUZZER').at(-1)?.payload.frequency===1000);
    await page.mouse.up();
    await page.waitForFunction(port=>window.__simProbe.events.filter(e=>e.type==='OUTPUT').at(-1)?.payload[port]===0,port);
  }
  await mouseButton(922,'out1');await mouseButton(982,'out2');
  await child.evaluate(()=>Object.assign(window.__coverageInputs,{TextLightSensor:75,TextInput1:1,TextInput2:1,TextInput3:1,TextInput4:1}));
  await page.getByLabel('Test temperature').fill('42');
  await page.waitForFunction(()=>{const p=window.__simProbe.events.filter(e=>e.type==='OUTPUT').at(-1)?.payload;return p&&p.displayNumber===42&&p.out1===1&&p.out2===1&&p.usb===1;});
  await page.screenshot({path:'src-tauri/target/device-program-live.png'});
  await page.getByRole('button',{name:'Stop',exact:true}).click();
  await page.waitForFunction(()=>window.__simProbe.events.filter(e=>e.type==='BUZZER').at(-1)?.payload.playing===false);
  await page.getByLabel('Close Simulator').click();
  assert.equal(external.length,0,JSON.stringify(external));assert.equal(pageErrors.length,0,JSON.stringify(pageErrors));
  // Test the user-facing examples through the actual Unity callback functions.
  // Physical sensor stimuli are fixtures; no direct IDE sensor override is added.
  await page.getByLabel('Run project simulator').click();
  await page.waitForFunction(()=>{const b=[...document.querySelectorAll('button')].find(b=>b.textContent.trim()==='Run Simulator');return b&&!b.disabled;},null,{timeout:180000});
  const sensorFrame=page.frames().find(frame=>frame.url().startsWith(unityOrigin));assert(sensorFrame);
  await sensorFrame.evaluate(()=>{
    window.__rawAnalog=2048;
    const callback=window.Call_AnalogInput1;
    window.Call_AnalogInput1=function(){return callback(window.__rawAnalog);};
    window.Call_AnalogInput1();
  });
  const writes=await page.evaluate(()=>window.__simProbe.calls.filter(c=>['write_project_file','accept_diff','reject_diff','run_shell_command'].includes(c.name)).length);
  await page.getByLabel('Simulation example').selectOption({label:'Analog IN1 → ค่าดิบบน LED'});
  await page.evaluate(()=>window.__simProbe.events=[]);
  await page.getByRole('button',{name:'Run Simulator',exact:true}).click();
  await page.waitForFunction(()=>window.__simProbe.events.some(e=>e.type==='OUTPUT'&&e.payload.displayNumber===2048));
  await sensorFrame.evaluate(()=>{window.__rawAnalog=4095;window.Call_AnalogInput1();});
  await page.waitForFunction(()=>window.__simProbe.events.some(e=>e.type==='OUTPUT'&&e.payload.displayNumber===4095));
  await page.getByRole('button',{name:'Stop',exact:true}).click();
  await page.getByLabel('Simulation example').selectOption({label:'Digital IN1 → 0/1 บน LED'});
  await sensorFrame.evaluate(()=>{
    window.__digitalInput=0;
    const callback=window.Call_Input1;
    window.Call_Input1=function(){return callback(window.__digitalInput);};
    window.Call_Input1();
  });
  await page.evaluate(()=>window.__simProbe.events=[]);
  await page.getByRole('button',{name:'Run Simulator',exact:true}).click();
  await page.waitForFunction(()=>window.__simProbe.events.some(e=>e.type==='OUTPUT'&&e.payload.displayNumber===0));
  await sensorFrame.evaluate(()=>{window.__digitalInput=1;window.Call_Input1();});
  await page.waitForFunction(()=>window.__simProbe.events.some(e=>e.type==='OUTPUT'&&e.payload.displayNumber===1));
  await page.getByRole('button',{name:'Stop',exact:true}).click();
  assert.equal(await page.evaluate(()=>window.__simProbe.calls.filter(c=>['write_project_file','accept_diff','reject_diff','run_shell_command'].includes(c.name)).length),writes);
  await page.getByLabel('Close Simulator').click();
  const report={analog_example_2048_4095_via_real_unity_callback_fixture:true,digital_example_0_1:true,examples_preserve_firmware:true,compiled_ui:true,real_unity:true,source_from_shared_device_compiler:true,temperature_25_42:true,actual_mouse_sw1_sw2:true,all_four_digital_inputs_fixture:true,light_threshold_and_distinct_outputs:true,buzzer_with_led_and_sw1:true,stop_silences_buzzer:true,external_requests:external,native_ipc_fixture:true,physical_sensors_fixture:true,ai_called:false,hardware_flashed:false};
  fs.writeFileSync('src-tauri/target/device-program-ui-verification.json',JSON.stringify(report,null,2)+'\n');console.log(JSON.stringify(report));
} finally {await browser?.close();await Promise.all([appServer,unityServer].map(server=>new Promise(resolve=>server.close(resolve))));}
