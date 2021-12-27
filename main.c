/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This file contains the "main" function for a project.

  Description:
    This file contains the "main" function for a project.  The
    "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include "definitions.h"                // SYS function prototypes
#include <math.h> 

// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************
extern void configure_codec(void);
extern void wm8904_init(void );

#define SAMPLE_FREQ 48000 
#define SAMPLE_WIDTH 16
#define DMA_BUF_DATA_TYPE signed short



//static dmac_descriptor_registers_t pTxLinkedListDesc[2] __ALIGNED(16);
//DMA_BUF_DATA_TYPE wave_buf[1024] __ALIGNED(16);
//static unsigned int dma_cnt = 0;
//static unsigned int samples = 0;


/* in order to make multi dma descriptor work: 
 * 1. block action must be set to DMAC_BTCTRL_BLOCKACT_INT, otherwise the multi DAM descriptors only be executed one 
 * 2. control A must set TRIGACT to 3

*/
#define BUFEER0_TX_BTCTRL  ( DMAC_BTCTRL_STEPSIZE_X1 | DMAC_BTCTRL_STEPSEL(DMAC_BTCTRL_STEPSEL_SRC_Val)|DMAC_BTCTRL_SRCINC(0x01) |  DMAC_BTCTRL_BEATSIZE_HWORD | DMAC_BTCTRL_BLOCKACT_INT | DMAC_BTCTRL_VALID_Msk )


unsigned int generate_sin(DMA_BUF_DATA_TYPE *buf, unsigned int frequency){
    unsigned int i,samples;
    int val;
    
    samples = SAMPLE_FREQ/frequency;
    if(sizeof(DMA_BUF_DATA_TYPE) == 2)
        val = 0x7fff;
    else
        val = 0x7fffffff;
    
    for(i=0;i<samples;i++){
         buf[i*2] = (DMA_BUF_DATA_TYPE)( val*sin( (double)(2*3.14159*i/samples) ) );
         buf[i*2+1] =  buf[i*2];
    }
    
    return samples;
    
}


#define reads(addr) (*(volatile unsigned short *)(addr))
//__IO  uint16_t test_pt;
int main ( void )
{
 //   int i;
    
    //test_pt = (uint16_t)0x41000018;
    /* Initialize all modules */
    SYS_Initialize ( NULL );
    SYSTICK_TimerStart();

    
    printf("system start\r\n");
    configure_codec();
    //samples = generate_sin((DMA_BUF_DATA_TYPE *)wave_buf,1000);
    
    /*
    DMAC_LinkedListDescriptorSetup(&pTxLinkedListDesc[0],BUFEER0_TX_BTCTRL,
        (void *) &wave_buf[0],
        (void *)0x43002830 ,
        SAMPLE_POINTS*2,
        &pTxLinkedListDesc[0]);
     */
    
    //DMAC_ChannelCallbackRegister(DMAC_CHANNEL_1, APP_Callback, 0);
    //DMAC_ChannelLinkedListTransfer(DMAC_CHANNEL_1, &pTxLinkedListDesc[0]);
    
    //DMAC_ChannelTransfer(DMAC_CHANNEL_1,(void *) &wave_buf[0],(void *)I2S_DEST ,samples*2*SAMPLE_WIDTH/8);
        

	
    while ( true )
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( );
        
        //LED1_Toggle();
        //SYSTICK_DelayMs(1000);
       // printf("use irq 0x%x, st 0x%x\r\n",reads(0x41000018),reads(0x4100001c));


    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}


/*******************************************************************************
 End of File
*/

