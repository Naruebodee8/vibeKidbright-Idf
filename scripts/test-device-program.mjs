// Execute generated C with a host hardware fixture and generated JS with the actual
// reference JS-Interpreter. Compare device actions, reads, frame bytes and delays.
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
const root=process.cwd();
const compiler=path.join(root,'src-tauri/target/debug/examples/generate-device-program.exe');
assert(fs.existsSync(compiler),'Build the offline generate-device-program example first.');
const cases=JSON.parse(fs.readFileSync('simulator-tests/device-program-cases.json','utf8'));
const directory=path.join(root,'src-tauri/target/device-program-tests');fs.mkdirSync(directory,{recursive:true});
const names=['temperature','light','s1','s2','input1','input2','input3','input4','out1','out2','usb','analog1','analog2','analog3','analog4'];
const sandbox=vm.createContext({});
for(const file of ['JS-Interpreter/acorn.js','JS-Interpreter/interpreter.js','runtime-core.js'])vm.runInContext(fs.readFileSync('simulator-assets/'+file,'utf8'),sandbox);
function executeJs(source,inputs){
  const trace=[],counts={},state=[0,0,0];let volume=50,frequency=0;
  const read=id=>{const name=names[id];const values=inputs[name]||[0];const count=counts[name]||0;counts[name]=count+1;const value=id>=8&&id<=10?state[id-8]:values[Math.min(count,values.length-1)];trace.push({kind:'read',sensor:name,value});return value;};
  const off=()=>{frequency=0;trace.push({kind:'off'});};
  const tone=hz=>{if(typeof hz!=='number'||!Number.isFinite(hz)||hz<20||hz>20000)throw Error('Invalid tone');frequency=hz;trace.push({kind:'tone',hz,volume});};
  const delay=ms=>trace.push({kind:'delay',ms});
  let failed=false,failureReason="";
  try{
    sandbox.KidBrightRuntimeCore.validate(source,sandbox.acorn,{requireOutput:true});
    const interpreter=new sandbox.Interpreter(source,(it,scope)=>{
      const objects={};
      const expose=(name,callback)=>{const parts=name.split('.');let owner=scope;
        if(parts.length===2){if(!objects[parts[0]]){objects[parts[0]]=it.createObject(it.OBJECT);it.setProperty(scope,parts[0],objects[parts[0]]);}owner=objects[parts[0]];}
        it.setProperty(owner,parts.at(-1),it.createNativeFunction((...args)=>it.createPrimitive(callback(...args))));};
      expose('vTaskDelay',delay);
      for(const [id,port] of ['output1','output2','usbsw'].entries()){
        expose('ports.'+port+'_write',value=>{const n=Number(value);if(!Number.isFinite(n))throw Error('Nonfinite output');state[id]=n===0?0:1;trace.push({kind:'output',port:['out1','out2','usb'][id],value:state[id]});});
        expose('ports.'+port+'_read',()=>read(id+8));
      }
      for(let n=1;n<=4;n++){
        expose('ports.input'+n+'_read',()=>read(n+3));
        expose('kb_specio.input'+n+'_read',()=>{const raw=read(n+10);if(!Number.isFinite(raw)||raw<0||raw>4095)throw Error('Invalid analog input');return Math.trunc(raw);});
      }
      expose('lm73_1.get',()=>read(0));expose('ldr.get',()=>read(1));
      expose('button12.sw1_get',()=>read(2));expose('button12.sw2_get',()=>read(3));
      expose('sound.tone',tone);expose('sound.note',note=>tone(sandbox.KidBrightRuntimeCore.noteFrequency(note)));
      expose('sound.off',off);expose('sound.rest',duration=>{delay(sandbox.KidBrightRuntimeCore.noteDuration(duration));off();});
      expose('sound.set_volume',value=>{if(typeof value!=='number'||!Number.isFinite(value)||value<0||value>100)throw Error('Invalid volume');volume=value;if(frequency)tone(frequency);});
      expose('sound.get_volume',()=>volume);
      const show=buffer=>{sandbox.KidBrightRuntimeCore.matrixCsv(buffer);trace.push({kind:'matrix',columns:Array.from(buffer,c=>c.charCodeAt(0))});};
      expose('ht16k33.show',show);expose('ht16k33.idle',()=>true);expose('ht16k33.wait_idle',()=>delay(0));
      expose('matrix.show_number',value=>show(sandbox.KidBrightRuntimeCore.numberBuffer(value)));
    });
    let steps=0;while(interpreter.step()){assert(++steps<1000000,'Bounded fixture failed to terminate');}
  }catch(error){failed=true;failureReason=String(error);trace.push({kind:'error'});}
  if(frequency)off();
  return {trace,failed,failureReason};
}
function header(inputs){
  const values=names.map(name=>inputs[name]||[0]);const width=Math.max(1,...values.map(a=>a.length));
  return `#include <stdlib.h>
static const double kb_test_values[${names.length}][${width}]={${values.map(a=>'{'+a.join(',')+'}').join(',')}};
static const unsigned kb_test_lengths[${names.length}]={${values.map(a=>a.length).join(',')}};
static unsigned kb_test_reads[${names.length}]={0};static int kb_test_outputs[3]={0};static double kb_test_hz=0;
static const char *kb_test_names[${names.length}]={${names.map(JSON.stringify).join(',')}};
static inline void kb_hw_error(const char *text){(void)text;puts("{\\"kind\\":\\"error\\"}");}
static inline void kb_hw_init(void){}
static inline double kb_hw_read(unsigned id){unsigned n=kb_test_reads[id]++;double value=id>=8&&id<=10?kb_test_outputs[id-8]:kb_test_values[id][n<kb_test_lengths[id]?n:kb_test_lengths[id]-1];printf("{\\"kind\\":\\"read\\",\\"sensor\\":\\"%s\\",\\"value\\":%.17g}\\n",kb_test_names[id],value);return value;}
static inline void kb_hw_output(unsigned id,bool on){kb_test_outputs[id]=on;printf("{\\"kind\\":\\"output\\",\\"port\\":\\"%s\\",\\"value\\":%d}\\n",kb_test_names[id+8],on?1:0);}
static inline void kb_hw_delay(uint32_t ms){printf("{\\"kind\\":\\"delay\\",\\"ms\\":%u}\\n",(unsigned)ms);}
static inline void kb_hw_matrix(const uint8_t cols[16]){fputs("{\\"kind\\":\\"matrix\\",\\"columns\\":[",stdout);for(unsigned n=0;n<16;n++)printf("%s%u",n?",":"",cols[n]);puts("]}");}
static inline void kb_hw_tone(double hz,double volume){kb_test_hz=hz;printf("{\\"kind\\":\\"tone\\",\\"hz\\":%.17g,\\"volume\\":%.17g}\\n",hz,volume);}
static inline void kb_hw_silence(void){kb_test_hz=0;puts("{\\"kind\\":\\"off\\"}");}
static inline void kb_hw_finish(bool fault){(void)fault;if(kb_test_hz)kb_hw_silence();}
`;
}
const report=[];
for(const fixture of cases){
  const generated=spawnSync(compiler,[],{input:JSON.stringify(fixture),encoding:'utf8',windowsHide:true,maxBuffer:1024*1024});
  assert.equal(generated.status,0,generated.stderr);const bundle=JSON.parse(generated.stdout);
  const folder=path.join(directory,fixture.name);fs.mkdirSync(folder,{recursive:true});
  fs.writeFileSync(path.join(folder,'main.c'),bundle.firmware+'\nint main(void){app_main();return 0;}\n');
  fs.writeFileSync(path.join(folder,'kb_host_test.h'),header(fixture.inputs));
  const exe=path.join(folder,'program.exe');
  const built=spawnSync('gcc',['-std=gnu11','-O0','-DKB_HOST_TEST','-I','.','main.c','-lm','-o','program.exe'],{cwd:folder,encoding:'utf8',windowsHide:true});
  assert.equal(built.status,0,fixture.name+': '+built.stderr);
  const run=spawnSync(exe,[],{encoding:'utf8',windowsHide:true,timeout:10000,maxBuffer:1024*1024});assert.equal(run.status,0,fixture.name+': '+run.stderr);
  const actual=run.stdout.trim().split(/\r?\n/).filter(Boolean).map(line=>JSON.parse(line));
  const expected=executeJs(bundle.source,fixture.inputs);
  assert.equal(expected.failed,!!fixture.error,fixture.name+' JS outcome: '+expected.failureReason);
  assert.equal(actual.some(e=>e.kind==='error'),!!fixture.error,fixture.name+' C outcome');
  if(!fixture.error)assert.deepEqual(actual,expected.trace,fixture.name+' C vs reference JS trace');
  fs.writeFileSync(path.join(folder,'js-trace.json'),JSON.stringify(expected.trace,null,2));
  fs.writeFileSync(path.join(folder,'c-trace.json'),JSON.stringify(actual,null,2));
  report.push({name:fixture.name,passed:true,actions:actual.length,features:bundle.features});
}
fs.writeFileSync(path.join(directory,'report.json'),JSON.stringify({cases:report,actual_c_execution:true,actual_reference_js_interpreter:true,hardware_fixture:true,board_flashed:false},null,2));
console.log(JSON.stringify({passed:report.length,cases:report.map(r=>r.name),actual_c_execution:true,actual_reference_interpreter:true,physical_board_tested:false}));
