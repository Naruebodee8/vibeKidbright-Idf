import {act,cleanup,fireEvent,render,screen,waitFor} from "@testing-library/react";
import {afterEach,beforeEach,describe,expect,it,vi} from "vitest";
const f=vi.hoisted(()=>({pending:true,calls:[] as string[],handlers:new Map<string,(event:{payload:{path:string}})=>void>()}));
vi.mock("@tauri-apps/api/core",()=>({invoke:vi.fn(async(name:string)=>{
  f.calls.push(name);
  if(name==="load_project_simulation"){
    if(f.pending)throw Error("มี Diff รอตรวจรับ: main/main.c");
    return {path:"simulation/main.sim.js",source:"ports.output2_write(1);",revision:"test",firmware_files:["main/main.c"]};
  }
  if(name==="list_pending_diffs")return f.pending?[{path:"D:/project/main/main.c",relative_path:"main/main.c"}]:[];
  if(name==="start_simulator_host")return {url:"http://127.0.0.1:5555/session/UnityBuild/index.html",origin:"http://127.0.0.1:5555",session_id:"test"};
  return null;
})}));
vi.mock("@tauri-apps/api/event",()=>({listen:vi.fn(async(name:string,handler:any)=>{
  f.handlers.set(name,handler);return()=>{f.handlers.delete(name);};
})}));
vi.mock("../simulator/SimulatorAdapter",()=>({SimulatorAdapter:class{
  constructor(_window:Window,_origin:string,_session:string,private event:(event:any)=>void){}
  async request(type:string){if(type==="HELLO")this.event({type:"READY",payload:{}});return {type:"ACK"};}
  close(){}
}}));
import SimulatorPanel from "../SimulatorPanel";
beforeEach(()=>{f.pending=true;f.calls=[];f.handlers.clear();});
afterEach(cleanup);
function open(onReviewFile=vi.fn()){
  render(<SimulatorPanel projectDir="D:/project" onReviewFile={onReviewFile} onClose={vi.fn()} onDraftChange={vi.fn()}/>);
  return onReviewFile;
}
describe("Simulator pending review navigation",()=>{
  it("shows the exact pending filename and opens review without accepting anything",async()=>{
    const review=open();
    const button=await screen.findByRole("button",{name:"ตรวจ Diff main/main.c"});
    expect(screen.getByRole("button",{name:"Run Simulator"})).toBeDisabled();
    fireEvent.click(button);
    expect(review).toHaveBeenCalledWith("D:/project/main/main.c");
    expect(f.calls).not.toContain("accept_diff");expect(f.calls).not.toContain("reject_diff");
  });
  it("refreshes after a resolved review and enables Run once project and Unity are ready",async()=>{
    open();await screen.findByRole("button",{name:"ตรวจ Diff main/main.c"});
    fireEvent.load(screen.getByTitle("Unity KidBright board"));
    f.pending=false;
    await act(async()=>{f.handlers.get("ai-diff-resolved")?.({payload:{path:"D:/project/main/main.c"}});});
    await waitFor(()=>expect(screen.queryByLabelText("Project simulation error")).not.toBeInTheDocument());
    await waitFor(()=>expect(screen.getByRole("button",{name:"Run Simulator"})).toBeEnabled());
  });
  it("does not treat a review in another project as resolved here",async()=>{
    open();await screen.findByRole("button",{name:"ตรวจ Diff main/main.c"});
    const before=f.calls.filter(name=>name==="load_project_simulation").length;
    await act(async()=>{f.handlers.get("ai-diff-resolved")?.({payload:{path:"D:/different/main/main.c"}});});
    expect(f.calls.filter(name=>name==="load_project_simulation")).toHaveLength(before);
    expect(screen.getByLabelText("Project simulation error")).toBeInTheDocument();
  });
});
