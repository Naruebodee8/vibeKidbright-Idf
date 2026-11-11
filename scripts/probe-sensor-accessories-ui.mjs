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

  // Values come exclusively from the actual Unity accessories and mouse input.
  // Native project/AI/hardware IPC remains a fixture; no real Build/Flash is run.
  const cases=[
    {name:'digital_moisture',mode:'digital',menu:397,row:0},
    {name:'digital_force',mode:'digital',menu:397,row:1},
    {name:'digital_pir',mode:'digital',menu:397,row:2},
    {name:'digital_obstacle',mode:'digital',menu:397,row:3},
    {name:'digital_line',mode:'digital',menu:397,row:4},
    {name:'digital_gas_smoke',mode:'digital',menu:397,row:5},
    {name:'analog_moisture',mode:'analog',menu:578,row:1},
    {name:'analog_force',mode:'analog',menu:578,row:2},
    {name:'analog_rain',mode:'analog',menu:578,row:3},
  ];
  if(process.env.VIBE_SENSOR_CASES){const names=process.env.VIBE_SENSOR_CASES.split(',');for(let n=cases.length-1;n>=0;n--)if(!names.includes(cases[n].name))cases.splice(n,1);}
  const results=[];
  // Find the small cyan Unity slider handle in a screenshot. This reads pixels
  // only, without injecting a value or invoking a sensor callback ourselves.
  async function slider(){
    const shot=(await page.screenshot()).toString('base64');
    return page.evaluate(async data=>{
      const image=new Image();image.src='data:image/png;base64,'+data;await image.decode();
      const canvas=document.createElement('canvas');canvas.width=image.width;canvas.height=image.height;
      const ctx=canvas.getContext('2d');ctx.drawImage(image,0,0);const {data:p}=ctx.getImageData(0,0,canvas.width,canvas.height);
      const w=canvas.width,seen=new Uint8Array(w*canvas.height),found=[];
      const blue=i=>p[4*i]<100&&p[4*i+1]>30&&p[4*i+1]<210&&p[4*i+2]>60&&p[4*i+2]>p[4*i+1]*1.05;
      for(let y=280;y<650;y++)for(let x=360;x<800;x++){
        const i=y*w+x;if(seen[i]||!blue(i))continue;
        const todo=[i];seen[i]=1;let minX=x,maxX=x,minY=y,maxY=y,count=0;
        while(todo.length){const index=todo.pop(),px=index%w,py=Math.floor(index/w);count++;minX=Math.min(minX,px);maxX=Math.max(maxX,px);minY=Math.min(minY,py);maxY=Math.max(maxY,py);
          for(const next of [index-1,index+1,index-w,index+w,index-w-1,index-w+1,index+w-1,index+w+1]){const nx=next%w,ny=Math.floor(next/w);if(nx<360||nx>=800||ny<280||ny>=650||seen[next]||!blue(next))continue;seen[next]=1;todo.push(next);}
        }
        const width=maxX-minX+1,height=maxY-minY+1;
        if(width>=7&&width<=20&&height>=7&&height<=20&&count>=12&&width/height>0.6&&width/height<1.6)found.push({x:(minX+maxX)/2,y:(minY+maxY)/2,width,height,count});
      }
      found.sort((a,b)=>a.y-b.y||a.x-b.x);return {handle:found[0],candidates:found};
    },shot);
  }
  try {
    for(const fixture of cases){
      await page.goto(appOrigin);
      await page.getByRole('button',{name:'ใช้ Simulator ก่อน'}).click();
      const api=fixture.mode==='analog'?'kb_specio':'ports';
      const source='while(1){matrix.show_number('+[1,2,3,4].map(n=>api+'.input'+n+'_read()').join('+')+');vTaskDelay(100);}';
      await page.evaluate(source=>{window.__simProbe.companionSource=source;},source);
      await page.getByLabel('Run project simulator').click();
      await page.waitForFunction(()=>window.__simProbe.events.some(e=>e.type==='READY'),null,{timeout:180000});
      const child=page.frames().find(frame=>frame.url().startsWith(unityOrigin));assert(child);
      await child.evaluate(()=>{
        window.__sensorTrace=[];window.__lastSensor={};
        for(const prefix of ['Call_Input','Call_AnalogInput'])for(let n=1;n<=4;n++){
          const name=prefix+n,original=window[name];
          window[name]=function(value){const result=original(value);if(window.__lastSensor[name]!==value){window.__sensorTrace.push({name,value});window.__lastSensor[name]=value;}return result;};
        }
      });
      const run=page.getByRole('button',{name:'Run Simulator',exact:true});
      const stop=page.getByRole('button',{name:'Stop',exact:true});
      await run.click();await page.waitForFunction(()=>window.__simProbe.events.some(e=>e.type==='RUN_STARTED'));
      await child.waitForFunction(()=>window.__sensorTrace.length>=8);
      await stop.click();await page.waitForTimeout(350);
      await page.mouse.click(fixture.menu,253);await page.waitForTimeout(250);
      await page.mouse.click(480,367+75*fixture.row);await page.waitForTimeout(350);
      await page.screenshot({path:'src-tauri/target/'+fixture.name+'-spawn.png'});
      await page.mouse.move(fixture.mode==='analog'?939:882,fixture.mode==='analog'?369:365);await page.mouse.down();
      await page.mouse.move(fixture.mode==='analog'?604:735,383,{steps:20});await page.mouse.up();await page.waitForTimeout(500);
      await page.evaluate(()=>window.__simProbe.events=[]);
      await run.click();await page.waitForFunction(()=>window.__simProbe.events.some(e=>e.type==='RUN_STARTED'));
      await page.waitForTimeout(400);
      await page.screenshot({path:'src-tauri/target/'+fixture.name+'-before.png'});
      const detected=await slider();assert(detected.handle,fixture.name+' slider: '+JSON.stringify(detected));
      const {x,y}=detected.handle;
      await page.mouse.move(x,y);await page.mouse.down();await page.mouse.move(x+(fixture.mode==='analog'?100:40),y,{steps:15});await page.mouse.up();
      await page.waitForTimeout(600);
      const prefix=fixture.mode==='analog'?'TextAnalogInput':'TextInput';
      const actual=await child.evaluate(prefix=>Object.fromEntries([1,2,3,4].map(n=>[prefix+n,Number(document.getElementById(prefix+n).textContent)])),prefix);
      const expected=Object.values(actual).reduce((a,b)=>a+b,0);
      assert(expected>0,fixture.name+' did not change its real Unity input: '+JSON.stringify({actual,detected}));
      await page.waitForFunction(value=>window.__simProbe.events.some(e=>e.type==='OUTPUT'&&e.payload.displayNumber===value),expected);
      await page.screenshot({path:'src-tauri/target/'+fixture.name+'-after.png'});
      const callbacks=await child.evaluate(()=>window.__sensorTrace);
      await stop.click();
      assert.equal(await page.evaluate(()=>window.__simProbe.calls.some(c=>['write_project_file','accept_diff','reject_diff','run_shell_command','send_ai_message'].includes(c.name))),false);
      results.push({name:fixture.name,mode:fixture.mode,values:actual,display:expected,callbacks,actual_mouse_input:true});
      fs.writeFileSync('src-tauri/target/sensor-accessories-verification.json',JSON.stringify({cases:results,real_unity:true,physical_sensor_values_fixture:false,native_ipc_fixture:true,ai_called:false,hardware_flashed:false},null,2));
      console.log(JSON.stringify({passed:fixture.name,values:actual,display:expected}));
    }
    assert.equal(pageErrors.length,0,JSON.stringify(pageErrors));assert.equal(external.length,0,JSON.stringify(external));
    console.log(JSON.stringify({passed:results.length,real_unity_accessories:true,actual_mouse_slider:true,external_requests:external,native_ipc_fixture:true,ai_called:false,hardware_flashed:false}));
  }catch(error){await page.screenshot({path:'src-tauri/target/sensor-accessory-failure.png'});throw error;}
} finally {await browser?.close();await Promise.all([appServer,unityServer].map(server=>new Promise(resolve=>server.close(resolve))));}
