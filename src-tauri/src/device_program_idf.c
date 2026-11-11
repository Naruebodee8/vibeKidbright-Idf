/* ESP-IDF 5.5 classic reference. Initialize only resources used by the program.
 * SDA21/SCL22 display; SDA4/SCL5 LM73; digital IN1..4 (no analog claim).
 * Physical revision, LM73 address and light calibration require verification. */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#if KB_MATRIX || KB_TEMPERATURE
#include "driver/i2c_master.h"
#endif
#if KB_LIGHT
#include "esp_adc/adc_oneshot.h"
#endif
#if KB_BUZZER
#include "driver/ledc.h"
#endif
static bool kb_output_ready[3]={false,false,false};
#if KB_BUZZER
static bool kb_buzzer_ready=false;
static double kb_buzzer_hz=0,kb_buzzer_volume=-1;
#endif
static int kb_output_state[3]={0,0,0};
static inline void kb_hw_error(const char *message){ESP_LOGE("KB_PROGRAM","%s",message);}
static inline bool kb_check(esp_err_t result,const char *operation){
    if(result==ESP_OK)return true;
    ESP_LOGE("KB_PROGRAM","%s failed: %s",operation,esp_err_to_name(result));kb_ok=false;return false;
}
static inline void kb_hw_delay(uint32_t ms){
    TickType_t ticks=pdMS_TO_TICKS(ms);vTaskDelay(ticks?ticks:1);
}
static inline void kb_hw_output(unsigned port,bool on){
    esp_err_t result=ESP_ERR_INVALID_ARG;
    /* Logical ON maps to each circuit, never alias OUT1 to the USB load. */
    if(port==0)result=gpio_set_level(GPIO_NUM_26,on?1:0);
    if(port==1)result=gpio_set_level(GPIO_NUM_27,on?1:0);
    if(port==2)result=gpio_set_level(GPIO_NUM_25,on?0:1);
    if(kb_check(result,"write output")){kb_output_state[port]=on;ESP_LOGI("KB_PROGRAM","%s %s",port==2?"USB":port==0?"OUT1":"OUT2",on?"ON":"OFF");}
}
static inline bool kb_input_init(gpio_num_t pin,bool pullup){
    gpio_config_t config={.pin_bit_mask=1ULL<<pin,.mode=GPIO_MODE_INPUT,
        .pull_up_en=pullup?GPIO_PULLUP_ENABLE:GPIO_PULLUP_DISABLE,
        .pull_down_en=GPIO_PULLDOWN_DISABLE,.intr_type=GPIO_INTR_DISABLE};
    return kb_check(gpio_config(&config),"input initialization");
}
#if KB_MATRIX || KB_TEMPERATURE
static inline bool kb_i2c_open(int port,int sda,int scl,int address,i2c_master_dev_handle_t *device){
    i2c_master_bus_config_t config={.i2c_port=port,.sda_io_num=sda,.scl_io_num=scl,
        .clk_source=I2C_CLK_SRC_DEFAULT,.glitch_ignore_cnt=7,.flags.enable_internal_pullup=true};
    i2c_master_bus_handle_t bus=NULL;
    if(!kb_check(i2c_new_master_bus(&config,&bus),"I2C bus initialization"))return false;
    i2c_device_config_t cfg={.dev_addr_length=I2C_ADDR_BIT_LEN_7,.device_address=address,.scl_speed_hz=100000};
    return kb_check(i2c_master_bus_add_device(bus,&cfg,device),"I2C device initialization");
}
#endif
#if KB_MATRIX
static i2c_master_dev_handle_t kb_display=NULL;
#endif
static inline void kb_hw_matrix(const uint8_t columns[16]){
#if KB_MATRIX
    uint8_t packet[17]={0};
    /* Columns already have bit 0 at the bottom: no second bit reversal. */
    for(unsigned x=0;x<16;x++)packet[1+2*(x%8)+x/8]=columns[x];
    kb_check(i2c_master_transmit(kb_display,packet,sizeof(packet),100),"matrix transfer");
#else
    (void)columns;
#endif
}
#if KB_TEMPERATURE
static i2c_master_dev_handle_t kb_temperature=NULL;
#endif
#if KB_LIGHT
static adc_oneshot_unit_handle_t kb_adc=NULL;
#endif
static inline double kb_hw_read(unsigned sensor){
    switch(sensor){
#if KB_TEMPERATURE
        case 0:{uint8_t reg=0,bytes[2];
            if(!kb_check(i2c_master_transmit_receive(kb_temperature,&reg,1,bytes,2,100),"LM73 read (check configured address/bus)"))return NAN;
            int32_t raw=((int32_t)bytes[0]<<8)|bytes[1];if(raw&0x8000)raw-=65536;return raw/128.0;}
#endif
#if KB_LIGHT
        case 1:{int raw=0;if(!kb_check(adc_oneshot_read(kb_adc,ADC_CHANNEL_0,&raw),"LDR read"))return NAN;
            double brightness=(KB_LIGHT_DARK-raw)*100.0/(KB_LIGHT_DARK-KB_LIGHT_BRIGHT);
            return brightness<0?0:brightness>100?100:brightness;}
#endif
        case 2:return gpio_get_level(GPIO_NUM_16)==0;
        case 3:return gpio_get_level(GPIO_NUM_14)==0;
        case 4:return gpio_get_level(GPIO_NUM_32)!=0;
        case 5:return gpio_get_level(GPIO_NUM_33)!=0;
        case 6:return gpio_get_level(GPIO_NUM_34)!=0;
        case 7:return gpio_get_level(GPIO_NUM_35)!=0;
        case 8:case 9:case 10:return kb_output_state[sensor-8];
    }
    kb_ok=false;kb_hw_error("Unsupported sensor");return NAN;
}
static inline void kb_hw_tone(double hz,double volume){
#if KB_BUZZER
    /* Adaptive resolution covers 20..20000 Hz on ESP32 LEDC. Volume is a
     * PWM approximation, not calibrated acoustic amplitude or WebAudio gain. */
    if(hz==kb_buzzer_hz && volume==kb_buzzer_volume)return;
    unsigned bits=hz<2000?14:10;
    if(hz!=kb_buzzer_hz){
    if(!kb_check(ledc_set_duty(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,0),"buzzer quiet before frequency change"))return;
    if(!kb_check(ledc_update_duty(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0),"buzzer duty"))return;
    ledc_timer_config_t timer={.speed_mode=LEDC_LOW_SPEED_MODE,.duty_resolution=bits,
        .timer_num=LEDC_TIMER_0,.freq_hz=(uint32_t)floor(hz+0.5),.clk_cfg=LEDC_AUTO_CLK};
    if(!kb_check(ledc_timer_config(&timer),"buzzer frequency"))return;
    }
    uint32_t duty=(uint32_t)(((1u<<bits)/2)*volume/100.0);
    if(!kb_check(ledc_set_duty(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,duty),"buzzer volume"))return;
    if(kb_check(ledc_update_duty(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0),"buzzer update")){kb_buzzer_hz=hz;kb_buzzer_volume=volume;}
#else
    (void)hz;(void)volume;
#endif
}
static inline void kb_hw_silence(void){
#if KB_BUZZER
    if(kb_buzzer_ready)kb_check(ledc_stop(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,0),"buzzer stop");
    kb_buzzer_hz=0;kb_buzzer_volume=-1;
#endif
}
static inline void kb_hw_init(void){
    const int output_used[3]={KB_OUT1,KB_OUT2,KB_USB};const gpio_num_t pins[3]={GPIO_NUM_26,GPIO_NUM_27,GPIO_NUM_25};
    for(unsigned p=0;p<3;p++)if(output_used[p]){
        if(!kb_check(gpio_set_level(pins[p],p==2?1:0),"preload output OFF"))return;
        gpio_config_t config={.pin_bit_mask=1ULL<<pins[p],.mode=GPIO_MODE_OUTPUT};
        if(!kb_check(gpio_config(&config),"output initialization"))return;
        kb_output_ready[p]=true;kb_hw_output(p,false);if(!kb_ok)return;
    }
    const int input_used[6]={KB_S1,KB_S2,KB_INPUT1,KB_INPUT2,KB_INPUT3,KB_INPUT4};
    const gpio_num_t inputs[6]={GPIO_NUM_16,GPIO_NUM_14,GPIO_NUM_32,GPIO_NUM_33,GPIO_NUM_34,GPIO_NUM_35};
    for(unsigned p=0;p<6;p++)if(input_used[p]&&!kb_input_init(inputs[p],p<2))return;
#if KB_MATRIX
    if(!kb_i2c_open(0,21,22,0x70,&kb_display))return;
    const uint8_t commands[3]={0x21,0x81,0xef};
    for(unsigned n=0;n<3;n++)if(!kb_check(i2c_master_transmit(kb_display,&commands[n],1,100),"matrix initialization"))return;
#endif
#if KB_TEMPERATURE
    if(!kb_i2c_open(1,4,5,KB_LM73_ADDRESS,&kb_temperature))return;
#endif
#if KB_LIGHT
    adc_oneshot_unit_init_cfg_t unit={.unit_id=ADC_UNIT_1};
    if(!kb_check(adc_oneshot_new_unit(&unit,&kb_adc),"ADC initialization"))return;
    adc_oneshot_chan_cfg_t channel={.atten=ADC_ATTEN_DB_12,.bitwidth=ADC_BITWIDTH_12};
    if(!kb_check(adc_oneshot_config_channel(kb_adc,ADC_CHANNEL_0,&channel),"ADC configuration"))return;
#endif
#if KB_BUZZER
    ledc_timer_config_t timer={.speed_mode=LEDC_LOW_SPEED_MODE,.duty_resolution=LEDC_TIMER_14_BIT,
        .timer_num=LEDC_TIMER_0,.freq_hz=440,.clk_cfg=LEDC_AUTO_CLK};
    if(!kb_check(ledc_timer_config(&timer),"buzzer timer initialization"))return;
    ledc_channel_config_t buzzer={.gpio_num=13,.speed_mode=LEDC_LOW_SPEED_MODE,.channel=LEDC_CHANNEL_0,.timer_sel=LEDC_TIMER_0,.duty=0};
    if(!kb_check(ledc_channel_config(&buzzer),"buzzer channel initialization"))return;
    kb_buzzer_ready=true;
#endif
}
static inline void kb_hw_finish(bool fault){
    if(fault)for(unsigned p=0;p<3;p++)if(kb_output_ready[p])kb_hw_output(p,false);
    kb_hw_silence();
}
