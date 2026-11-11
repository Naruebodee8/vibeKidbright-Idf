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
        if(name==='send_ai_message'){
          setTimeout(()=>{
            if(args.userRequest==='duplicate-project'){
              const error='มีไฟล์หรือโฟลเดอร์ชื่อ moisture_sim อยู่แล้ว กรุณาเลือกชื่อโปรเจกต์ใหม่ หรือสั่งทำต่อในโครงเดิมอย่างชัดเจน ระบบจะไม่เขียนทับไฟล์เดิม';
              emit('ai-chat-tool-result',JSON.stringify({name:'create_project_workspace',result:{error}}));
              emit('ai-chat-delta','เลือกชื่อใหม่ หรือสั่งทำต่อในโปรเจกต์เดิมได้ครับ');
              emit('ai-chat-error',error);
              state.aiCompletions=(state.aiCompletions||0)+1;return;
            }
            if(args.userRequest==='generation-retry'){
              emit('ai-chat-tool-start',JSON.stringify({name:'generate_device_program'}));
              emit('ai-chat-tool-result',JSON.stringify({name:'generate_device_program',result:{error:'Invalid simulation program JSON: missing field `op`. At $.steps[0].body[0]'}}));
              emit('ai-chat-tool-start',JSON.stringify({name:'generate_device_program'}));
              emit('ai-chat-tool-result',JSON.stringify({name:'generate_device_program',result:{result:'Generated C and JS'}}));
              emit('ai-generation-check',JSON.stringify({messageId:args.messageId,validation:'shared_behavior',features:['matrix','analog1'],buildChecked:false}));
              emit('ai-chat-delta','สร้างคู่ C/JS แล้ว ยังไม่ได้ Build หรือทดสอบบอร์ดจริง');
            }
            if(args.userRequest==='scaffold-only'){
              emit('ai-generation-check',JSON.stringify({messageId:args.messageId,validation:'scaffold_only',firmwareAvailable:false,simulationReady:false,buildChecked:false}));
              emit('ai-chat-delta','ขอข้อมูลบอร์ดก่อนสร้าง driver Analog');
            }
            if(args.userRequest.includes('servo')){
              emit('ai-generation-check',JSON.stringify({messageId:args.messageId,validation:'unsupported',capabilities:['servo','pwm'],firmwareAvailable:true,simulationReady:false,buildChecked:false}));
              emit('ai-chat-delta','มีเฟิร์มแวร์เซอร์โวให้ตรวจแล้ว แต่ซิมยังไม่รองรับ กรุณาตรวจรุ่นบอร์ดและขา GPIO');
            }
            emit('ai-chat-done',JSON.stringify({history:[]}));
            state.aiCompletions=(state.aiCompletions||0)+1;
          },args.userRequest==='Create firmware'?250:50);return;
        }
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
  await page.evaluate(()=>window.__simProbe.pendingReview=true);
  await page.getByLabel('Run project simulator').click();
  await page.getByRole('button',{name:'ตรวจ Diff main/main.c'}).waitFor();
  assert(await page.getByRole('button',{name:'Run Simulator',exact:true}).isDisabled());
  assert.equal(await page.evaluate(()=>window.__simProbe.calls.filter(c=>c.name==='accept_diff').length),0);
  await page.getByRole('button',{name:'ตรวจ Diff main/main.c'}).click();
  await page.getByTitle('Accept AI proposed changes').click();
  await page.waitForFunction(()=>!window.__simProbe.pendingReview);
  assert.equal(await page.evaluate(()=>window.__simProbe.calls.filter(c=>c.name==='accept_diff').length),1);
  await page.getByLabel('Run project simulator').click();
  await page.waitForFunction(()=>{const b=[...document.querySelectorAll('button')].find(b=>b.textContent.trim()==='Run Simulator');return b&&!b.disabled;},null,{timeout:180000});
  await page.getByLabel('Close Simulator').click();
  await page.getByText('main.c',{exact:true}).last().click();
  const editor=page.locator('.monaco-editor .view-lines').first();await editor.waitFor();
  await editor.click({position:{x:45,y:10}});await page.keyboard.press('ControlOrMeta+A');await page.keyboard.insertText('void app_main(void) { /* changed */ }');
  await page.getByTitle('Unsaved changes').waitFor();
  await page.evaluate(()=>window.__simProbe.failSave=true);
  const flash=page.getByRole('button',{name:/Build & Flash/});
  await flash.click();
  await page.getByText(/Build failed:.*fixture save failed/).waitFor();
  assert.equal(await page.evaluate(()=>window.__simProbe.calls.filter(c=>c.name==='run_shell_command').length),0);
  await page.getByTitle('Unsaved changes').waitFor();
  await page.evaluate(()=>window.__simProbe.failSave=false);
  await flash.click();
  await page.waitForFunction(()=>window.__simProbe.calls.some(c=>c.name==='run_shell_command'));
  const original=await page.evaluate(()=>window.__simProbe.calls.find(c=>c.name==='run_shell_command').args);
  assert.deepEqual(original,{cmd:'idf.py',args:['build','flash','-p','COM_GUI_FIXTURE'],cwd:'/fixture/project'});
  const before=await page.evaluate(()=>window.__simProbe.calls.filter(c=>c.name==='run_shell_command').length);
  await page.getByLabel('Run project simulator').click();
  await page.waitForFunction(()=>window.__simProbe.events.some(e=>e.type==='READY'),{timeout:180000});
  const child=page.frames().find(frame=>frame.url().startsWith(unityOrigin));
  assert(child);assert.equal(await child.evaluate(()=>typeof window.__TAURI_INTERNALS__),'undefined','The Unity frame must not receive the IDE IPC fixture.');
  const run=page.getByRole('button',{name:'Run Simulator',exact:true});await run.click();
  await page.waitForFunction(()=>window.__simProbe.events.some(e=>e.type==='OUTPUT'&&e.payload.displayNumber===25),{timeout:30000});
  await page.getByLabel('Test temperature').fill('42');
  await page.waitForFunction(()=>window.__simProbe.events.some(e=>e.type==='OUTPUT'&&e.payload.displayNumber===42));
  await page.getByRole('button',{name:'Stop',exact:true}).click();
  await page.waitForFunction(()=>window.__simProbe.events.some(e=>e.type==='RUN_ENDED'));
  const expectedFrames=[...alphabetSource.matchAll(/ht16k33[.]show[(]"((?:[^"])+)"[)]/g)].map(match=>{
    const bytes=[...match[1].matchAll(/\\x([0-9a-f]{2})/g)].map(value=>parseInt(value[1],16));
    assert.equal(bytes.length,16);
    return bytes.flatMap(value=>Array.from({length:8},(_,y)=>(value>>y)&1)).join(',');
  });
  assert.equal(expectedFrames.length,26);
  await page.evaluate(source=>{window.__simProbe.companionSource=source;window.__simProbe.events=[];},alphabetSource);
  await page.getByLabel('Load project simulation').click();
  await run.click();
  await page.waitForFunction(first=>window.__simProbe.events.some(e=>e.type==='OUTPUT'&&e.payload.matrix===first),expectedFrames[0]);
  await page.screenshot({path:'src-tauri/target/alphabet-A.png'});
  await page.waitForFunction(last=>window.__simProbe.events.some(e=>e.type==='OUTPUT'&&e.payload.matrix===last),expectedFrames[25],{timeout:30000});
  await page.screenshot({path:'src-tauri/target/alphabet-Z.png'});
  await page.waitForFunction(first=>window.__simProbe.events.filter(e=>e.type==='OUTPUT'&&e.payload.matrix===first).length>=2,expectedFrames[0],{timeout:5000});
  const actual=await page.evaluate(()=>window.__simProbe.events.filter(e=>e.type==='OUTPUT').map(e=>e.payload.matrix));
  assert.deepEqual(actual.slice(0,27),[...expectedFrames,expectedFrames[0]]);
  await page.getByRole('button',{name:'Stop',exact:true}).click();
  await page.getByLabel('Close Simulator').click();
  assert.equal(await page.evaluate(()=>window.__simProbe.calls.filter(c=>c.name==='run_shell_command').length),before,'Sim must not call Build/Flash');
  await flash.click();
  await page.waitForFunction(before=>window.__simProbe.calls.filter(c=>c.name==='run_shell_command').length===before+1,before);
  const typescriptWorker=page.waitForEvent('worker',{predicate:worker=>worker.url().includes('ts.worker-')});
  await page.getByText('main.sim.js',{exact:true}).click();
  const worker=await typescriptWorker;
  assert.equal(await worker.evaluate(()=>typeof self.onmessage),'function','The JavaScript editor worker must initialize.');
  assert(workers.some(url=>url.includes('editor.worker-')),'The firmware editor must also use a bundled worker.');
  assert.equal(await page.getByLabel('Generate simulation companion').count(),0);
  await page.getByText('สร้างไฟล์ Simulator คู่กับเฟิร์มแวร์อัตโนมัติ',{exact:true}).waitFor();
  const input=page.locator('textarea').last();await input.fill('Create firmware');await input.press('Enter');
  await page.waitForFunction(()=>window.__simProbe.calls.filter(c=>c.name==='send_ai_message').length===1);
  await page.waitForFunction(()=>window.__simProbe.aiCompletions===1);
  await page.getByTitle('⏹️ หยุดการทำงาน',{exact:true}).waitFor({state:'detached'});
  await input.fill('Create firmware and simulation');await input.press('Enter');
  await page.waitForFunction(()=>window.__simProbe.calls.filter(c=>c.name==='send_ai_message').length===2);
  assert.deepEqual(await page.evaluate(()=>window.__simProbe.calls.filter(c=>c.name==='send_ai_message').map(c=>c.args.simulationEnabled)),[true,true]);
  assert.deepEqual(await page.evaluate(()=>window.__simProbe.calls.filter(c=>c.name==='send_ai_message').map(c=>c.args.userRequest)),['Create firmware','Create firmware and simulation']);
  await page.waitForFunction(()=>window.__simProbe.aiCompletions===2);
  await page.getByTitle('⏹️ หยุดการทำงาน',{exact:true}).waitFor({state:'detached'});
  await input.fill('SW1 servo 0 SW2 servo 90');await input.press('Enter');
  await page.waitForFunction(()=>window.__simProbe.aiCompletions===3);
  await page.getByTitle('⏹️ หยุดการทำงาน',{exact:true}).waitFor({state:'detached'});
  await page.getByRole('status').filter({hasText:'ซิมยังไม่รองรับ: เซอร์โว, PWM'}).waitFor();
  await page.getByText('มีเฟิร์มแวร์เซอร์โวให้ตรวจแล้ว แต่ซิมยังไม่รองรับ กรุณาตรวจรุ่นบอร์ดและขา GPIO',{exact:true}).waitFor();
  assert.equal(await page.getByText(/❌ Error/).count(),0);
  await input.fill('Explain the board pins');await input.press('Enter');
  await page.waitForFunction(()=>window.__simProbe.aiCompletions===4);
  await page.getByTitle('⏹️ หยุดการทำงาน',{exact:true}).waitFor({state:'detached'});
  await input.fill('duplicate-project');await input.press('Enter');
  await page.waitForFunction(()=>window.__simProbe.aiCompletions===5);
  await page.getByTitle('⏹️ หยุดการทำงาน',{exact:true}).waitFor({state:'detached'});
  const conflict=page.getByRole('alert').filter({hasText:'มีไฟล์หรือโฟลเดอร์ชื่อ moisture_sim อยู่แล้ว'});await conflict.waitFor();
  assert(!(await conflict.textContent()).includes('{"error"'));
  const errorBox=await conflict.boundingBox(),cardBox=await conflict.locator('..').boundingBox();
  assert(errorBox.width>=cardBox.width*0.8,'Error must span the tool card, not a narrow column beside its name');
  const markerBox=await page.getByLabel('Tool failed',{exact:true}).boundingBox();
  assert(errorBox.y>=markerBox.y+markerBox.height,'Error belongs below its heading');
  assert.equal(await page.getByText(/Hello boilerplate/).count(),0);
  await page.screenshot({path:'src-tauri/target/project-create-error-readable.png'});
  await input.fill('scaffold-only');await input.press('Enter');
  await page.waitForFunction(()=>window.__simProbe.aiCompletions===6);
  await page.getByTitle('⏹️ หยุดการทำงาน',{exact:true}).waitFor({state:'detached'});
  await page.getByRole('status').filter({hasText:'สร้างเฉพาะโครงโปรเจกต์'}).waitFor();
  await page.getByText('ขอข้อมูลบอร์ดก่อนสร้าง driver Analog',{exact:true}).waitFor();
  await input.fill('generation-retry');await input.press('Enter');
  await page.waitForFunction(()=>window.__simProbe.aiCompletions===7);
  await page.getByTitle('⏹️ หยุดการทำงาน',{exact:true}).waitFor({state:'detached'});
  await page.getByLabel('Tool retry succeeded',{exact:true}).waitFor();
  await page.getByText('ลองใหม่สำเร็จแล้ว · ข้อความด้านล่างคือข้อผิดพลาดครั้งก่อน',{exact:true}).waitFor();
  assert.equal(await page.getByRole('alert').filter({hasText:'missing field `op`'}).count(),0);
  await page.getByText('Invalid simulation program JSON: missing field `op`. At $.steps[0].body[0]',{exact:true}).waitFor();
  await page.screenshot({path:'src-tauri/target/generation-retry-resolved.png'});
  await input.fill('Explain the result');await input.press('Enter');
  await page.waitForFunction(()=>window.__simProbe.aiCompletions===8);
  await page.getByTitle('⏹️ หยุดการทำงาน',{exact:true}).waitFor({state:'detached'});
  assert.equal(external.length,0,JSON.stringify(external));assert.equal(pageErrors.length,0,JSON.stringify(pageErrors));
  const report={compiled_ui:true,simulator_without_sdk_download:true,real_unity:true,csp_enforced:true,save_failure_blocks_build:true,pending_review_list_to_explicit_keep:true,original_flash_arguments_preserved:true,simulator_does_not_build_or_flash:true,temperature_25_42:true,compiled_alphabet_A_to_Z_and_wrap:true,stop_close_build_again:true,ai_companion_automatic:true,generation_retry_retains_error_and_marks_success:true,chat_after_generation_retry:true,duplicate_creation_error_readable:true,no_secondary_hello_error:true,scaffold_only_receipt:true,retry_chat_after_creation_failure:true,unsupported_firmware_only_receipt_and_followup:true,bundled_editor_and_javascript_workers:true,iframe_ipc_isolated:true,page_errors:pageErrors,external_requests:external,hardware_flashed:false,native_ipc_fixture:true};
  fs.mkdirSync('src-tauri/target',{recursive:true});fs.writeFileSync('src-tauri/target/simulator-ui-verification.json',JSON.stringify(report,null,2)+'\n');
  console.log(JSON.stringify(report));
} finally {await browser?.close();await Promise.all([appServer,unityServer].map(server=>new Promise(resolve=>server.close(resolve))));}
