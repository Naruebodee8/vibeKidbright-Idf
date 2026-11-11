import {act,cleanup,fireEvent,render,screen} from '@testing-library/react';
import {afterEach,beforeEach,describe,expect,it,vi} from 'vitest';
const f=vi.hoisted(()=>({calls:[] as {name:string,args:any}[],listeners:new Map<string,(event:any)=>void>()}));
vi.mock('@tauri-apps/api/core',()=>({invoke:vi.fn(async(name:string,args:any)=>{
    f.calls.push({name,args});
    if(name==='get_provider')return 'local';
    if(name==='get_base_url')return 'http://127.0.0.1:1234/v1';
    if(name==='get_knowledge_base_files')return [];
    return '';
})}));
vi.mock('@tauri-apps/api/event',()=>({listen:vi.fn(async(name:string,callback:(event:any)=>void)=>{f.listeners.set(name,callback);return ()=>f.listeners.delete(name);})}));
import AiChat from '../AiChat';
beforeEach(()=>{f.calls=[];f.listeners.clear();localStorage.clear();});
afterEach(cleanup);
async function emit(name:string,payload:unknown){await act(async()=>{f.listeners.get(name)?.({payload:JSON.stringify(payload)});});}
describe('Explicit hardware context sent to the AI',()=>{
    it('automatically requests a companion without either checkbox or fan selector',async()=>{
        const send={current:null as ((text:string)=>void)|null};
        render(<AiChat projectDir="/project" onInjectCode={()=>{}} sendApiRef={send}/>);
        await screen.findByText('Local',{exact:true});
        expect(screen.queryByLabelText('Fan connection for AI')).not.toBeInTheDocument();
        expect(screen.queryByLabelText('Generate simulation companion')).not.toBeInTheDocument();
        expect(screen.getByText('สร้างไฟล์ Simulator คู่กับเฟิร์มแวร์อัตโนมัติ')).toBeInTheDocument();
        expect(screen.queryByText('ช่องพัดลมที่ส่งให้ AI')).not.toBeInTheDocument();
        const prompt=screen.getByPlaceholderText('Ask about your code...');
        fireEvent.change(prompt,{target:{value:'SW1 toggle USB fan'}});
        fireEvent.keyDown(prompt,{key:'Enter',code:'Enter'});
        const args=f.calls.find(c=>c.name==='send_ai_message')?.args;
        expect(args).toMatchObject({hardwareTarget:null,simulationEnabled:true,userRequest:'SW1 toggle USB fan',projectDir:'/project',selectedBoard:'kidbright32'});
        await emit('ai-chat-tool-result',{name:'generate_simulation',result:{error:'Wrong port'}});
        expect(screen.getByLabelText('Tool failed')).toBeInTheDocument();
        expect(screen.getByRole('alert')).toHaveTextContent('Wrong port');
        await emit('ai-generation-check',{messageId:args.messageId,fan:'usb',buildChecked:false});
        expect(screen.getByRole('status')).toHaveTextContent('USB');
        expect(screen.getByRole('status')).toHaveTextContent('ยังไม่ได้ทดสอบ Build');
    });
    it('shows checked shared behavior separately from revision-only custom firmware',async()=>{
        const send={current:null as ((text:string)=>void)|null};
        render(<AiChat projectDir="/project" onInjectCode={()=>{}} sendApiRef={send} selectedBoard="miuaiplus"/>);
        await screen.findByText('Local',{exact:true});
        await act(async()=>{
        fireEvent.change(screen.getByPlaceholderText('Ask about your code...'),{target:{value:'Show temperature'}});
        fireEvent.keyDown(screen.getByPlaceholderText('Ask about your code...'),{key:'Enter',code:'Enter'});
        });
        const args=f.calls.find(c=>c.name==='send_ai_message')?.args;
        expect(args.selectedBoard).toBe('miuaiplus');
        await emit('ai-generation-check',{messageId:args.messageId,validation:'shared_behavior',features:['matrix','temperature','buzzer'],build_checked:false});
        expect(screen.getByRole('status')).toHaveTextContent('LED Matrix, อุณหภูมิ, Buzzer');
        expect(screen.getByRole('status')).toHaveTextContent('ยังไม่ได้ทดสอบ Build');
        await emit('ai-generation-check',{messageId:args.messageId,validation:'revision_only',buildChecked:false});
        expect(screen.getByRole('status')).toHaveTextContent('ยังไม่ได้ยืนยันพฤติกรรมหรือ Build');
    });
    it('shows firmware-only unsupported completion, keeps its explanation and permits another request',async()=>{
        render(<AiChat projectDir="/servo" onInjectCode={()=>{}}/>);
        await screen.findByText('Local',{exact:true});
        const prompt=screen.getByPlaceholderText('Ask about your code...');
        fireEvent.change(prompt,{target:{value:'SW1 servo 0 SW2 servo 90'}});
        fireEvent.keyDown(prompt,{key:'Enter',code:'Enter'});
        const args=f.calls.find(c=>c.name==='send_ai_message')!.args;
        await emit('ai-generation-check',{messageId:args.messageId,validation:'unsupported',capabilities:['servo','pwm'],simulationReady:false,buildChecked:false});
        const receipt=screen.getByRole('status');
        expect(receipt).toHaveTextContent('ซิมยังไม่รองรับ: เซอร์โว, PWM');
        expect(receipt).toHaveTextContent('ยังรันซิมงานนี้ไม่ได้');
        expect(receipt).toHaveTextContent('ยังไม่ได้ทดสอบ Build');
        await act(async()=>{f.listeners.get('ai-chat-delta')?.({payload:'สร้างไฟล์ C แล้ว แต่ซิมยังไม่รองรับเซอร์โว กรุณาตรวจรุ่นบอร์ดและขา GPIO'});});
        await emit('ai-chat-done',{history:[]});
        expect(screen.getByText(/สร้างไฟล์ C แล้ว แต่ซิมยังไม่รองรับเซอร์โว/)).toBeInTheDocument();
        expect(screen.queryByText(/❌ Error/)).not.toBeInTheDocument();
        fireEvent.change(prompt,{target:{value:'อธิบายวิธีตรวจขาบอร์ด'}});
        fireEvent.keyDown(prompt,{key:'Enter',code:'Enter'});
        expect(f.calls.filter(c=>c.name==='send_ai_message')).toHaveLength(2);
    });
    it('preserves the streamed explanation when a real companion validation error occurs',async()=>{
        render(<AiChat projectDir="/project" onInjectCode={()=>{}}/>);
        await screen.findByText('Local',{exact:true});
        const prompt=screen.getByPlaceholderText('Ask about your code...');
        fireEvent.change(prompt,{target:{value:'Show temperature'}});
        fireEvent.keyDown(prompt,{key:'Enter',code:'Enter'});
        // Both native events can arrive before React has rendered the final chunk.
        await act(async()=>{
            f.listeners.get('ai-chat-delta')?.({payload:'เขียนเฟิร์มแวร์แล้ว แต่ยังต้องสร้างคู่ไฟล์ซิม'});
            f.listeners.get('ai-chat-error')?.({payload:'ยังไม่มีไฟล์ simulation/main.sim.js'});
        });
        expect(screen.getByText('เขียนเฟิร์มแวร์แล้ว แต่ยังต้องสร้างคู่ไฟล์ซิม')).toBeInTheDocument();
        expect(screen.getByText(/❌ Error: ยังไม่มีไฟล์ simulation\/main.sim.js/)).toBeInTheDocument();
        fireEvent.change(prompt,{target:{value:'สร้างคู่ไฟล์ที่ขาด'}});
        fireEvent.keyDown(prompt,{key:'Enter',code:'Enter'});
        expect(f.calls.filter(c=>c.name==='send_ai_message')).toHaveLength(2);
    });
    it('shows a readable creation conflict, keeps the explanation and permits a fresh request',async()=>{
        render(<AiChat projectDir="/old" onInjectCode={()=>{}}/>);
        await screen.findByText('Local',{exact:true});
        const prompt=screen.getByPlaceholderText('Ask about your code...');
        await act(async()=>{fireEvent.change(prompt,{target:{value:'สร้างโปรเจกต์ใหม่ moisture_sim'}});fireEvent.keyDown(prompt,{key:'Enter',code:'Enter'});});
        const error='มีไฟล์หรือโฟลเดอร์ชื่อ moisture_sim อยู่แล้ว กรุณาเลือกชื่อใหม่';
        await emit('ai-chat-tool-result',{name:'create_project_workspace',result:{error}});
        expect(screen.getByRole('alert')).toHaveTextContent(error);
        expect(screen.getByRole('alert').textContent).not.toContain('{"error"');
        expect(screen.getByLabelText('Tool failed')).toBeInTheDocument();
        await act(async()=>{
            f.listeners.get('ai-chat-delta')?.({payload:'กรุณาเลือกชื่อโปรเจกต์ใหม่ หรือสั่งทำต่อในโครงเดิม'});
            f.listeners.get('ai-chat-error')?.({payload:error});
        });
        expect(screen.queryByText(/Hello boilerplate/)).not.toBeInTheDocument();
        expect(screen.getByText('กรุณาเลือกชื่อโปรเจกต์ใหม่ หรือสั่งทำต่อในโครงเดิม')).toBeInTheDocument();
        await act(async()=>{fireEvent.change(prompt,{target:{value:'สร้างโปรเจกต์ใหม่ moisture_sim_2'}});fireEvent.keyDown(prompt,{key:'Enter',code:'Enter'});});
        expect(f.calls.filter(c=>c.name==='send_ai_message')).toHaveLength(2);
    });
    it('labels a workspace scaffold as incomplete while waiting for hardware information',async()=>{
        render(<AiChat projectDir="/old" onInjectCode={()=>{}}/>);
        await screen.findByText('Local',{exact:true});
        const prompt=screen.getByPlaceholderText('Ask about your code...');
        await act(async()=>{fireEvent.change(prompt,{target:{value:'สร้างโปรเจกต์ใหม่'}});fireEvent.keyDown(prompt,{key:'Enter',code:'Enter'});});
        const args=f.calls.find(c=>c.name==='send_ai_message')!.args;
        await emit('ai-generation-check',{messageId:args.messageId,validation:'scaffold_only',firmwareAvailable:false,simulationReady:false});
        await act(async()=>{f.listeners.get('ai-chat-delta')?.({payload:'ขอทราบรุ่นย่อยบอร์ดก่อนสร้าง driver Analog'});});
        await emit('ai-chat-done',{history:[]});
        expect(screen.getByRole('status')).toHaveTextContent('สร้างเฉพาะโครงโปรเจกต์');
        expect(screen.getByRole('status')).toHaveTextContent('ยังไม่มีโปรแกรมหรือไฟล์ซิมพร้อมใช้งาน');
        expect(screen.queryByText(/Hello boilerplate/)).not.toBeInTheDocument();
        expect(screen.getByText('ขอทราบรุ่นย่อยบอร์ดก่อนสร้าง driver Analog')).toBeInTheDocument();
    });
    it('does not keep a hidden port override when switching projects',async()=>{
        const send={current:null as ((text:string)=>void)|null};
        const view=render(<AiChat projectDir="/old" onInjectCode={()=>{}} sendApiRef={send}/>);
        await screen.findByText('Local',{exact:true});
        await act(async()=>{
        fireEvent.change(screen.getByPlaceholderText('Ask about your code...'),{target:{value:'SW1 toggle USB fan'}});
        fireEvent.keyDown(screen.getByPlaceholderText('Ask about your code...'),{key:'Enter',code:'Enter'});
        });
        await emit('ai-chat-done',{history:[]});
        view.rerender(<AiChat projectDir="/new" onInjectCode={()=>{}} sendApiRef={send}/>);
        await act(async()=>{
        fireEvent.change(screen.getByPlaceholderText('Ask about your code...'),{target:{value:'SW1 toggle OUT2 fan'}});
        fireEvent.keyDown(screen.getByPlaceholderText('Ask about your code...'),{key:'Enter',code:'Enter'});
        });
        const calls=f.calls.filter(call=>call.name==='send_ai_message');
        expect(calls).toHaveLength(2);
        expect(calls.map(call=>call.args.hardwareTarget)).toEqual([null,null]);
        expect(calls.map(call=>call.args.simulationEnabled)).toEqual([true,true]);
        expect(calls.map(call=>call.args.userRequest)).toEqual(['SW1 toggle USB fan','SW1 toggle OUT2 fan']);
        expect(f.calls.some(call=>call.name==='get_hardware_target')).toBe(false);
    });
});

describe('Generation tool retries',()=>{
    it('retains the failed payload but marks a successful retry and permits follow-up chat',async()=>{
        render(<AiChat projectDir="/project" onInjectCode={()=>{}}/>);
        await screen.findByText('Local',{exact:true});
        const prompt=screen.getByPlaceholderText('Ask about your code...');
        fireEvent.change(prompt,{target:{value:'Analog IN1 on LED'}});
        fireEvent.keyDown(prompt,{key:'Enter',code:'Enter'});
        const args=f.calls.find(c=>c.name==='send_ai_message')!.args;
        const error='Invalid simulation program JSON: missing field `op` at $.steps[0].body[0]';
        await emit('ai-chat-tool-result',{name:'generate_device_program',result:{error}});
        expect(screen.getByRole('alert')).toHaveTextContent(error);
        await emit('ai-chat-tool-result',{name:'read_knowledge_file',result:{result:'schema'}});
        expect(screen.getByLabelText('Tool failed')).toBeInTheDocument();
        await emit('ai-chat-tool-result',{name:'generate_device_program',result:{error:'Still invalid'}});
        expect(screen.getAllByLabelText('Tool failed')).toHaveLength(2);
        await emit('ai-chat-tool-result',{name:'generate_device_program',result:{result:'Generated C and JS'}});
        expect(screen.getAllByLabelText('Tool retry succeeded')).toHaveLength(2);
        expect(screen.queryByRole('alert')).not.toBeInTheDocument();
        expect(screen.getByText(error)).toBeInTheDocument();
        await emit('ai-generation-check',{messageId:args.messageId,validation:'shared_behavior',features:['matrix','analog1'],buildChecked:false});
        expect(screen.getByRole('status')).toHaveTextContent('ยังไม่ได้ทดสอบ Build');
        await emit('ai-chat-done',{history:[]});
        fireEvent.change(prompt,{target:{value:'Explain the scale'}});
        fireEvent.keyDown(prompt,{key:'Enter',code:'Enter'});
        expect(f.calls.filter(c=>c.name==='send_ai_message')).toHaveLength(2);
    });
    it('does not resolve errors from another tool or an earlier workspace',async()=>{
        render(<AiChat projectDir="/project" onInjectCode={()=>{}}/>);
        await screen.findByText('Local',{exact:true});
        const prompt=screen.getByPlaceholderText('Ask about your code...');
        fireEvent.change(prompt,{target:{value:'Generate a program'}});
        fireEvent.keyDown(prompt,{key:'Enter',code:'Enter'});
        await emit('ai-chat-tool-result',{name:'generate_device_program',result:{error:'Old workspace failure'}});
        await emit('ai-chat-tool-result',{name:'generate_simulation',result:{result:'JS only'}});
        expect(screen.queryByLabelText('Tool retry succeeded')).not.toBeInTheDocument();
        await emit('ai-chat-tool-result',{name:'create_project_workspace',result:{workspace_path:'/new'}});
        await emit('ai-chat-tool-result',{name:'generate_device_program',result:{result:'New workspace pair'}});
        expect(screen.getByRole('alert')).toHaveTextContent('Old workspace failure');
        expect(screen.queryByLabelText('Tool retry succeeded')).not.toBeInTheDocument();
    });
    it('keeps a final retry failure visible and does not resolve it from a later chat message',async()=>{
        render(<AiChat projectDir="/project" onInjectCode={()=>{}}/>);
        await screen.findByText('Local',{exact:true});
        const prompt=screen.getByPlaceholderText('Ask about your code...');
        fireEvent.change(prompt,{target:{value:'Generate a program'}});
        fireEvent.keyDown(prompt,{key:'Enter',code:'Enter'});
        await emit('ai-chat-tool-result',{name:'generate_device_program',result:{error:'Uncorrected program'}});
        await act(async()=>{f.listeners.get('ai-chat-error')?.({payload:'Uncorrected program'});});
        expect(screen.getByRole('alert')).toHaveTextContent('Uncorrected program');
        fireEvent.change(prompt,{target:{value:'Try again'}});
        fireEvent.keyDown(prompt,{key:'Enter',code:'Enter'});
        await emit('ai-chat-tool-result',{name:'generate_device_program',result:{result:'Generated'}});
        expect(screen.getByRole('alert')).toHaveTextContent('Uncorrected program');
        expect(screen.queryByLabelText('Tool retry succeeded')).not.toBeInTheDocument();
    });
});
