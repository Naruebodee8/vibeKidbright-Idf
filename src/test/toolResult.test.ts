import {describe,it,expect} from 'vitest';
import {toolResultFailed,toolResultErrorMessage} from '../toolResult';
describe('AI tool failure indicators',()=>{
    it('does not mark provider errors as successful tools',()=>{
        expect(toolResultFailed({error:'wrong fan port'})).toBe(true);
        expect(toolResultFailed('{"error":"wrong GPIO"}')).toBe(true);
        expect(toolResultFailed({result:'The word error in a success message'})).toBe(false);
        expect(toolResultFailed('Saved file')).toBe(false);
        expect(toolResultFailed(null)).toBe(false);
    });
});

it('extracts readable tool errors without JSON wrappers and preserves plain failure text',()=>{
    const text='มีโฟลเดอร์ชื่อ moisture_sim อยู่แล้ว กรุณาเลือกชื่อใหม่';
    expect(toolResultErrorMessage({error:text})).toBe(text);
    expect(toolResultErrorMessage(JSON.stringify({error:text}))).toBe(text);
    expect(toolResultErrorMessage(text)).toBe(text);
    expect(toolResultErrorMessage(JSON.stringify({error:{code:'EXISTS'}}))).toBe('{"code":"EXISTS"}');
    expect(toolResultErrorMessage({result:'Saved'})).toBe('');
    expect(toolResultErrorMessage(null)).toBe('');
});
