#include "wm8904.h"
#include "definitions.h" 


#define WM8904_SLAVE_ADDRESS             0x1a
#define delay_ms(x)  SYSTICK_DelayMs(x)

enum {
    I2C_SUCCESS = 1,
    I2C_ERROR=2 ,
    I2C_IDLE=3
};
static volatile uint8_t i2c_state = I2C_IDLE;









void APP_I2CCallback(uintptr_t context )
{
    uint8_t *ret = (uint8_t *)context;
    
    if(SERCOM6_I2C_ErrorGet() == SERCOM_I2C_ERROR_NONE){
        *ret = I2C_SUCCESS;
        
    }
    else{
        *ret = I2C_ERROR;
        printf("i2c error\r\n");
    }
}


int i2c_sync_write(uint8_t* wrData, uint32_t wrLength)
{
    int ret = 0;
    i2c_state = I2C_IDLE;
    SERCOM6_I2C_CallbackRegister( APP_I2CCallback, (uintptr_t)&i2c_state );
    SERCOM6_I2C_Write(WM8904_SLAVE_ADDRESS, wrData, wrLength);
    
    while(i2c_state == I2C_IDLE);
    if(i2c_state != I2C_SUCCESS)
        ret = -1;
    
    return ret;
}

int i2c_sync_write_read(uint8_t* wrData, uint32_t wrLength, uint8_t* rdData, uint32_t rdLength)
{
    int ret = 0;
    i2c_state = I2C_IDLE;
    SERCOM6_I2C_CallbackRegister( APP_I2CCallback, (uintptr_t)&i2c_state );
    SERCOM6_I2C_WriteRead(WM8904_SLAVE_ADDRESS, wrData, wrLength, rdData , rdLength);
    
    while(i2c_state == I2C_IDLE);
    
    if(i2c_state != I2C_SUCCESS)
        ret = -1;
    
    return ret;    
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void wm8904_write_register(uint8_t uc_register_address, uint16_t us_data)
{
	uint8_t uc_temp_data[3];

    uc_temp_data[0] = uc_register_address;
	uc_temp_data[1] = (us_data & 0xff00) >> 8;
	uc_temp_data[2] = us_data & 0xff;
    
    i2c_sync_write(&uc_temp_data[0], 3);
    
}

uint16_t wm8904_read_register(uint8_t uc_register_address)
{
	uint8_t uc_temp_data[2];
	uint16_t us_data;


    i2c_sync_write_read( &uc_register_address, 1,  &uc_temp_data[0], 2);
    
    us_data = (((uint16_t)uc_temp_data[0] << 8) & 0xff00) | uc_temp_data[1];

	return us_data;
}

//
void configure_codec(void)
{
	uint16_t data = 0;
	/* check that WM8904 is present */
	wm8904_write_register(WM8904_SW_RESET_AND_ID, 0xFFFF);
	data = wm8904_read_register(WM8904_SW_RESET_AND_ID);
	if(data != 0x8904) {
		printf("WM8904 not found!\n\r");
		while(1);
	}
    printf("wm8904 access OK\r\n");

	wm8904_write_register(WM8904_BIAS_CONTROL_0, WM8904_ISEL_HP_BIAS);
	wm8904_write_register(WM8904_VMID_CONTROL_0, WM8904_VMID_BUF_ENA |
							WM8904_VMID_RES_FAST | WM8904_VMID_ENA);
	delay_ms(5);
	wm8904_write_register(WM8904_VMID_CONTROL_0, WM8904_VMID_BUF_ENA |
							WM8904_VMID_RES_NORMAL | WM8904_VMID_ENA);
	wm8904_write_register(WM8904_BIAS_CONTROL_0, WM8904_ISEL_HP_BIAS | WM8904_BIAS_ENA);
	wm8904_write_register(WM8904_POWER_MANAGEMENT_0, WM8904_INL_ENA | WM8904_INR_ENA);
	wm8904_write_register(WM8904_POWER_MANAGEMENT_2, WM8904_HPL_PGA_ENA | WM8904_HPR_PGA_ENA);
	wm8904_write_register(WM8904_DAC_DIGITAL_1, WM8904_DEEMPH(0));
	wm8904_write_register(WM8904_ANALOGUE_OUT12_ZC, 0x0000);
	wm8904_write_register(WM8904_CHARGE_PUMP_0, WM8904_CP_ENA);
	wm8904_write_register(WM8904_CLASS_W_0, WM8904_CP_DYN_PWR);
    
    
    // input 32768Hz from PA17, FLL out = 12.288M
	wm8904_write_register(WM8904_FLL_CONTROL_1, 0x0000);
	wm8904_write_register(WM8904_FLL_CONTROL_2, WM8904_FLL_OUTDIV(7)| WM8904_FLL_FRATIO(4));
	wm8904_write_register(WM8904_FLL_CONTROL_3, WM8904_FLL_K(0x8000));
	wm8904_write_register(WM8904_FLL_CONTROL_4, WM8904_FLL_N(0xBB));
	wm8904_write_register(WM8904_FLL_CONTROL_1, WM8904_FLL_FRACN_ENA | WM8904_FLL_ENA);
	delay_ms(5);
    
    // set sysclk/fs rate= 256 : 3-> 256, sample rate: 5 ->48K/44.1K
	wm8904_write_register(WM8904_CLOCK_RATES_1, WM8904_CLK_SYS_RATE(3) | WM8904_SAMPLE_RATE(5));
	wm8904_write_register(WM8904_CLOCK_RATES_0, 0x0000);
    
    // chose FLL as sysclock resource ->WM8904_SYSCLK_SRC=0x40000
	wm8904_write_register(WM8904_CLOCK_RATES_2, WM8904_SYSCLK_SRC | WM8904_CLK_SYS_ENA | WM8904_CLK_DSP_ENA);


#if 1
    // i2s width == 16
   // set BCLK output, bit width=16 bit,BCK=fs*width=sysclk/256 * width = sysclk/8
	wm8904_write_register(WM8904_AUDIO_INTERFACE_1, WM8904_BCLK_DIR | WM8904_AIF_FMT_I2S);
	wm8904_write_register(WM8904_AUDIO_INTERFACE_2, WM8904_BCLK_DIV(8));
    
    // LRCLK is output, LRCLK=BCLK/0x20
	wm8904_write_register(WM8904_AUDIO_INTERFACE_3, WM8904_LRCLK_DIR | WM8904_LRCLK_RATE(0x20));  
    
#else     
    // i2s width == 32
    
    // set BCLK output,bit width = 32
    wm8904_write_register(WM8904_AUDIO_INTERFACE_1, WM8904_BCLK_DIR | WM8904_AIF_FMT_I2S | WM8904_AIF_WL_32BIT);
	wm8904_write_register(WM8904_AUDIO_INTERFACE_2, WM8904_BCLK_DIV(4));
    
    wm8904_write_register(WM8904_AUDIO_INTERFACE_3, WM8904_LRCLK_DIR | WM8904_LRCLK_RATE(0x40));  
#endif   

	wm8904_write_register(WM8904_POWER_MANAGEMENT_6, 
						WM8904_DACL_ENA | WM8904_DACR_ENA | 
						WM8904_ADCL_ENA | WM8904_ADCR_ENA);
	delay_ms(5);
	wm8904_write_register(WM8904_ANALOGUE_LEFT_INPUT_0, WM8904_LIN_VOL(0x10));
	wm8904_write_register(WM8904_ANALOGUE_RIGHT_INPUT_0, WM8904_RIN_VOL(0x10));
	wm8904_write_register(WM8904_ANALOGUE_HP_0, 
						WM8904_HPL_ENA | WM8904_HPR_ENA);
	wm8904_write_register(WM8904_ANALOGUE_HP_0, 
						WM8904_HPL_ENA_DLY | WM8904_HPL_ENA |
						WM8904_HPR_ENA_DLY | WM8904_HPR_ENA);
	wm8904_write_register(WM8904_DC_SERVO_0, 
						WM8904_DCS_ENA_CHAN_3 | WM8904_DCS_ENA_CHAN_2 |
						WM8904_DCS_ENA_CHAN_1 | WM8904_DCS_ENA_CHAN_0);
	wm8904_write_register(WM8904_DC_SERVO_1, 
						WM8904_DCS_TRIG_STARTUP_3 | WM8904_DCS_TRIG_STARTUP_2 |
						WM8904_DCS_TRIG_STARTUP_1 | WM8904_DCS_TRIG_STARTUP_0);
	delay_ms(100);
	wm8904_write_register(WM8904_ANALOGUE_HP_0, 
						WM8904_HPL_ENA_OUTP | WM8904_HPL_ENA_DLY | WM8904_HPL_ENA | 
						WM8904_HPR_ENA_OUTP | WM8904_HPR_ENA_DLY | WM8904_HPR_ENA);
	wm8904_write_register(WM8904_ANALOGUE_HP_0, 
						WM8904_HPL_RMV_SHORT | WM8904_HPL_ENA_OUTP | WM8904_HPL_ENA_DLY | WM8904_HPL_ENA | 
						WM8904_HPR_RMV_SHORT | WM8904_HPR_ENA_OUTP | WM8904_HPR_ENA_DLY | WM8904_HPR_ENA);
	wm8904_write_register(WM8904_ANALOGUE_OUT1_LEFT, WM8904_HPOUT_VU | WM8904_HPOUTL_VOL(0x39));
	wm8904_write_register(WM8904_ANALOGUE_OUT1_RIGHT, WM8904_HPOUT_VU | WM8904_HPOUTR_VOL(0x39));
	delay_ms(100);
}
