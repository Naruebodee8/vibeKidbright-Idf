/* Portable semantics shared by generated firmware and host regression tests.
 * Columns use the reference simulator's bit-0-at-bottom convention. */
typedef struct { double number; bool boolean; } kb_value;
static inline kb_value kb_num(double n) { return (kb_value){n,false}; }
static inline kb_value kb_bool(bool b) { return (kb_value){b?1.0:0.0,true}; }
static inline bool kb_truth(kb_value v) { return v.number!=0 && !isnan(v.number); }
static double kb_volume=50, kb_frequency=0;
static inline uint32_t kb_u32(double x) {
    if(!isfinite(x)||x==0)return 0;
    double n=fmod(trunc(x),4294967296.0);
    return (uint32_t)(n<0?n+4294967296.0:n);
}
static inline double kb_signed(uint32_t n) { return n>=2147483648u?(double)n-4294967296.0:(double)n; }
static inline kb_value kb_binary(unsigned op,kb_value a,kb_value b) {
    double x=a.number,y=b.number; uint32_t u=kb_u32(x),v=kb_u32(y),shift=v&31;
    switch(op) {
        case 0:return kb_num(x+y);case 1:return kb_num(x-y);case 2:return kb_num(x*y);
        case 3:return kb_num(x/y);case 4:return kb_num(fmod(x,y));
        case 5:return kb_bool(x<y);case 6:return kb_bool(x<=y);case 7:return kb_bool(x>y);case 8:return kb_bool(x>=y);
        case 9:return kb_bool(x==y);case 10:return kb_bool(x!=y);
        case 11:return kb_bool(a.boolean==b.boolean && x==y);case 12:return kb_bool(a.boolean!=b.boolean || x!=y);
        case 13:return kb_num(kb_signed(u&v));case 14:return kb_num(kb_signed(u|v));case 15:return kb_num(kb_signed(u^v));
        case 16:return kb_num(kb_signed(u<<shift));
        case 17:{uint32_t n=u>>shift;if((u&0x80000000u)&&shift)n|=UINT32_MAX<<(32-shift);return kb_num(kb_signed(n));}
        case 18:return kb_num((double)(u>>shift));
    }
    return kb_num(NAN);
}
/* GNU C statement expressions preserve JS left-to-right and short circuit rules. */
#define KB_BINARY(op,left,right) ({kb_value __kb_l=(left);kb_value __kb_r=(right);kb_binary(op,__kb_l,__kb_r);})
#define KB_AND(left,right) ({kb_value __kb_l=(left);kb_truth(__kb_l)?(right):__kb_l;})
#define KB_OR(left,right) ({kb_value __kb_l=(left);kb_truth(__kb_l)?__kb_l:(right);})
static inline kb_value kb_unary(unsigned op,kb_value v) {
    switch(op){case 0:return kb_bool(!kb_truth(v));case 1:return kb_num(kb_signed(~kb_u32(v.number)));case 2:return kb_num(v.number);default:return kb_num(-v.number);}
}
static inline void kb_fail(const char *message) { kb_ok=false;kb_hw_error(message); }
static inline void kb_output(unsigned port,kb_value value) {
    if(!kb_ok)return;
    if(!isfinite(value.number)){kb_fail("Output requires a finite number or boolean");return;}
    kb_hw_output(port,value.number!=0);
}
static inline void kb_tone(kb_value hz) {
    if(!kb_ok)return;
    if(hz.boolean||!isfinite(hz.number)||hz.number<20||hz.number>20000){kb_fail("Tone requires 20..20000 Hz");return;}
    kb_frequency=hz.number;kb_hw_tone(kb_frequency,kb_volume);
}
static inline void kb_off(void) { kb_frequency=0;kb_hw_silence(); }
static inline void kb_set_volume(kb_value value) {
    if(!kb_ok)return;
    if(value.boolean||!isfinite(value.number)||value.number<0||value.number>100){kb_fail("Volume requires 0..100");return;}
    kb_volume=value.number;if(kb_frequency)kb_hw_tone(kb_frequency,kb_volume);
}
static inline void kb_char(kb_value value) {
    if(!kb_ok)return;
    if(!isfinite(value.number)||value.number!=floor(value.number)||value.number<32||value.number>126){kb_fail("Character requires printable ASCII code 32..126");return;}
    uint8_t columns[16]={0};memcpy(columns+5,kb_font[(unsigned)value.number-32],6);kb_hw_matrix(columns);
}
static inline void kb_number(kb_value value) {
    if(!kb_ok)return;
    /* Same Math.round rule, compact 3x5 glyphs and centering as runtime-core.js. */
    static const char *digits[11]={"111101101101111","010110010010111","111001111100111","111001111001111","101101111001001","111100111001111","111100111101111","111001010010010","111101111101111","111101111001111","000000111000000"};
    double rounded=floor(value.number+0.5);
    if(value.boolean||!isfinite(rounded)||rounded< -999||rounded>9999){kb_fail("Number display requires -999..9999");return;}
    char text[8];snprintf(text,sizeof(text),"%d",(int)rounded);size_t length=strlen(text);
    uint8_t columns[16]={0};unsigned left=(16-(length*4-1))/2;
    for(size_t n=0;n<length;n++)for(unsigned y=0;y<5;y++)for(unsigned x=0;x<3;x++){
        unsigned d=text[n]=='-'?10:(unsigned)(text[n]-'0');
        if(digits[d][y*3+x]=='1')columns[left+n*4+x]|=(uint8_t)(1u<<(6-y));
    }
    kb_hw_matrix(columns);
}
