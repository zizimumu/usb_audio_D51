/*******************************************************************************
  MPLAB Harmony Application Source File
  
  Company:
    Microchip Technology Inc.
  
  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It 
    implements the logic of the application's state machine and it may call 
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files 
// *****************************************************************************
// *****************************************************************************
#include "configuration.h"
#include "definitions.h"
#include "app.h"


#define debug_log(...) printf(__VA_ARGS__) 


#define APP_ID_FEATURE_UNIT    0x05
#define APP_QUEUING_DEPTH  2  
#define APP_PERIOD_SIZE (96*2*2)  //must be multi size of max endpoint

// the I2S TX data register must +2 at 16 bit width mode
#define I2S_DEST 0x43002832
#define DMA_BUF_LEN (32*1024)


typedef struct
{
    /* Application's current state*/
    volatile APP_STATES state;
     /* device layer handle returned by device layer open function */
    USB_DEVICE_HANDLE   usbDevHandle;
    /* Instance number of Audio Function driver */
    USB_DEVICE_AUDIO_INDEX audioInstance;
    /* device configured state */
    bool isConfigured;
    uint32_t usbInterface;
    // uint32_t activeMicInterfaceAlternateSetting;

     uint32_t sampleFreq;
    int volume;
    volatile unsigned char rxReady;
	volatile unsigned char dmaBuffState;
    uint32_t peroidSize;
    uint8_t dmaBeatSize;
	uint32_t sof;

    bool volMuteMode;
	unsigned int playWritePt;
	unsigned char *usb_buf[APP_QUEUING_DEPTH];


} APP_DATA;

typedef struct
{
    uint8_t  isUsed;               //Next Buffer for Codec TX
    uint8_t  isDataReady;                  //Next Buffer for USB RX 
    
    USB_DEVICE_AUDIO_TRANSFER_HANDLE usbReadHandle;
} APP_PLAYBACK_BUFFER_QUEUE;

//Application Class Data
APP_DATA appData = {
	.state = APP_STATE_INIT,
	.playWritePt = 0,
	.dmaBuffState = PLAY_DMA_BUF_NORMAL,

};





unsigned char dma_buff[DMA_BUF_LEN] __ALIGNED(4);  // 8K frames for 16bit stero 
unsigned char usb_buf1[APP_PERIOD_SIZE] __ALIGNED(32);
unsigned char usb_buf2[APP_PERIOD_SIZE] __ALIGNED(32);


APP_PLAYBACK_BUFFER_QUEUE  buf_queue[APP_QUEUING_DEPTH];










void send_read_request(void)
{
    unsigned int i;
    USB_DEVICE_AUDIO_RESULT ret;
    USB_DEVICE_AUDIO_TRANSFER_HANDLE usbReadHandle;
    
    for(i=0;i<APP_QUEUING_DEPTH;i++){
        if(!buf_queue[i].isUsed){

			buf_queue[i].isDataReady = 0;
		
            ret = USB_DEVICE_AUDIO_Read(USB_DEVICE_INDEX_0, 
                          &buf_queue[i].usbReadHandle, 
                          1, appData.usb_buf[i], 
                          appData.peroidSize);

            if(ret != USB_DEVICE_AUDIO_RESULT_OK){
                buf_queue[i].isUsed = 0;
                debug_log("usb urp %d error %d,SCHAR_MIN %d\r\n",i,(int)ret,SCHAR_MIN);
                break;
            }          
            else{
                buf_queue[i].isUsed = 1;
            }
			
			
        }
    }
}

int isAllBuffQueueReady(void)
{
    unsigned int i;
    
    for(i=0;i<APP_QUEUING_DEPTH;i++){
        if(!buf_queue[i].isDataReady){
            return 0;
        }
    }
    
    return 1;
}
 void ClearBuffQueue(void)
 {
	 unsigned int i;
	 
	 for(i=0;i<APP_QUEUING_DEPTH;i++){
		 buf_queue[i].isDataReady = 0;
		 buf_queue[i].isUsed = 0;

	 }

 }


 void stop_i2s_tx(void )
{
	unsigned int reg = I2S_REGS->I2S_CTRLA;

	reg &= ~(1 << 4);

	I2S_REGS->I2S_CTRLA = reg;
	
}

 void start_i2s_tx(void )
{
	unsigned int reg = I2S_REGS->I2S_CTRLA;

	reg |= (1 << 4);

	I2S_REGS->I2S_CTRLA = reg;
	
}


#define UNDERRUN_LEVEL (APP_PERIOD_SIZE/2)
void copy2dma_buff(unsigned char *buf,unsigned int len)
{
    unsigned int writePt;
    unsigned int readPt;
    unsigned int cnt,free_size,gap;
	//static unsigned char dmaBuffState = PLAY_DMA_BUF_NORMAL;
    
    readPt = appData.dmaBeatSize * DMAC_ChannelGetTransferredCount(DMAC_CHANNEL_1);


    /*
		all the operation below make write point in front of read point 
	*/
	writePt = appData.playWritePt;

    if(writePt > readPt){
        free_size = DMA_BUF_LEN - writePt + readPt;
        gap = writePt - readPt;
    }
	else if(writePt == readPt){
		free_size = DMA_BUF_LEN;
		gap = DMA_BUF_LEN;
	}
    else{
        free_size = readPt - writePt;
        gap = DMA_BUF_LEN - writePt + readPt;
    }

    if(free_size <= len){  
        debug_log("over run, readPt 0x%x, writePt 0x%x\r\n",readPt,writePt);
        return;
    }

    else{// buff is enough
        
        if(gap < UNDERRUN_LEVEL){
          // at ideal condition, gap = APP_PERIOD_SIZE* APP_QUEUING_DEPTH
          // treat it as under run when gap < APP_PERIOD_SIZE/2   
          	if(appData.dmaBuffState != PLAY_DMA_BUF_UNDER_RUN){
	            
	            stop_i2s_tx();
				appData.dmaBuffState = PLAY_DMA_BUF_UNDER_RUN;
				debug_log("under run\r\n");
			}
        }
        else if(gap >= APP_PERIOD_SIZE && appData.dmaBuffState == PLAY_DMA_BUF_UNDER_RUN ){
            start_i2s_tx();
			appData.dmaBuffState = PLAY_DMA_BUF_NORMAL;
		
            debug_log("leave runder run\r\n");
        }
        else{
        }
        
        if(writePt >= readPt){

            if(DMA_BUF_LEN - writePt >= len ){
                memcpy(writePt+dma_buff,buf,len);
                writePt += len;
            }
            else{
                gap = DMA_BUF_LEN - writePt;
                memcpy(writePt+dma_buff,buf,gap);
                memcpy(dma_buff,buf + gap ,len - gap);
                writePt = len - gap;
            }
        }
        else{
            memcpy(writePt+dma_buff,buf,len); 
            writePt += len;
        }    
        if(writePt >= DMA_BUF_LEN)
            writePt = 0; 


		appData.playWritePt = writePt;

    }

        
}


int isUnderRun(void )
{

    unsigned int writePt = appData.playWritePt;
    unsigned int readPt;
    unsigned int cnt,free_size,gap;
	//static unsigned char dmaBuffState = PLAY_DMA_BUF_NORMAL;
    
    readPt = appData.dmaBeatSize * DMAC_ChannelGetTransferredCount(DMAC_CHANNEL_1);
    

    if(writePt > readPt){
        free_size = DMA_BUF_LEN - writePt + readPt;
        gap = writePt - readPt;
    }
	else if(writePt == readPt){
		free_size = DMA_BUF_LEN;
		gap = DMA_BUF_LEN;
	}
    else{
        free_size = readPt - writePt;
        gap = DMA_BUF_LEN - writePt + readPt;
    }

	if(gap < UNDERRUN_LEVEL){
		return 1;
	}

	return 0;


}
void process_read_data(void )
{
    int i;
    USB_DEVICE_AUDIO_RESULT ret;
	static unsigned int rx_cnt = 0;


	// printf("dma pt 0x%x\r\n",DMAC_ChannelGetTransferredCount(DMAC_CHANNEL_1));
    
     for(i=0;i<APP_QUEUING_DEPTH;i++){
        if(buf_queue[i].isDataReady){

            
            copy2dma_buff(appData.usb_buf[i],APP_PERIOD_SIZE);
            
            ret = USB_DEVICE_AUDIO_Read(USB_DEVICE_INDEX_0, 
                          &buf_queue[i].usbReadHandle, 
                          1, appData.usb_buf[i], 
                          appData.peroidSize); //48 Samples

            if(ret != USB_DEVICE_AUDIO_RESULT_OK){
                buf_queue[i].isUsed = 0;
                break;
            }          
            else{
                buf_queue[i].isUsed = 1;
                buf_queue[i].isDataReady = 0;
            }
			
			if((rx_cnt % 250) == 0)
				printf("rx cnt %d\r\n",rx_cnt);
			rx_cnt++;
        }
    }   
}

void dma_callback(DMAC_TRANSFER_EVENT status, uintptr_t context) {

	static unsigned int dma_cnt = 0;
    DMAC_ChannelTransfer(DMAC_CHANNEL_1,(void *) &dma_buff[0],(void *)I2S_DEST ,sizeof(dma_buff));
	dma_cnt++;
	//if((dma_cnt % 6) == 0)
	//	debug_log("dma triggle %d\r\n",dma_cnt);
}


int wait_dma_buff_sync()
{

    unsigned int writePt = appData.playWritePt;
    unsigned int readPt,start,timer;
    unsigned int cnt,free_size,gap;
	int delay;
	//static unsigned char dmaBuffState = PLAY_DMA_BUF_NORMAL;


	start = SYSTICK_msCounter();
	delay = 0;

	do{
	    readPt = appData.dmaBeatSize * DMAC_ChannelGetTransferredCount(DMAC_CHANNEL_1);
	    

	    if(writePt > readPt){
	        gap = writePt - readPt;
	    }
		else if(writePt == readPt){
			gap = DMA_BUF_LEN;
		}
	    else{
	        gap = DMA_BUF_LEN - writePt + readPt;
	    }

		timer = SYSTICK_msCounter();
		delay = SYSTICK_msPeriodGet(start);// delay must <=10ms

	}while(gap >= UNDERRUN_LEVEL/2 && delay < 5);



}



void start_player(void)
{

	DMAC_ChannelCallbackRegister(DMAC_CHANNEL_1, dma_callback, 0);
	DMAC_ChannelTransfer(DMAC_CHANNEL_1,(void *) &dma_buff[0],(void *)I2S_DEST ,sizeof(dma_buff));
	start_i2s_tx();
	
	
	debug_log("playing\r\n");

}

void stop_play()
{
	
	wait_dma_buff_sync();
	stop_i2s_tx();
	DMAC_ChannelDisable(DMAC_CHANNEL_1);
	ClearBuffQueue();

	appData.playWritePt = 0;
	appData.dmaBuffState = PLAY_DMA_BUF_NORMAL;

	debug_log("stoping play\r\n");
}


void APP_Initialize ( void )
{

}
void APP_Tasks ( void )
{  
    USB_DEVICE_AUDIO_TRANSFER_HANDLE usbReadHandle;
    USB_DEVICE_AUDIO_RESULT ret;
    unsigned int i;
            
    switch(appData.state){
        case APP_STATE_INIT :
			usb_open();
			appData.state = APP_STATE_USB_CONFIGURED;
			appData.usb_buf[0] = usb_buf1;
			appData.usb_buf[1] = usb_buf2;
            appData.peroidSize = APP_PERIOD_SIZE;
			appData.dmaBeatSize = 2;			
			
		break;
		case APP_STATE_USB_CONFIGURED :
            if(appData.isConfigured == 1 && appData.usbInterface == USB_AUDIO_INTERFACE_PLAYING){
                appData.state = APP_STATE_USB_INIT_READ;

                send_read_request();
            }
            break;
        case APP_STATE_USB_INIT_READ :
            // wait for all buff queue data ready, then start player
            if(isAllBuffQueueReady()){
                appData.state = APP_STATE_PLAYING;
				appData.playWritePt = APP_QUEUING_DEPTH*APP_PERIOD_SIZE;

				send_read_request();
                start_player();
            }
            break;
        case APP_STATE_PLAYING :

			if(appData.usbInterface == USB_AUDIO_INTERFACE_NON){

				stop_play();
				appData.state = APP_STATE_USB_CONFIGURED;
				//wait_dma_buff_sync();
				//stop_i2s_tx();
				//debug_log("pausing play\r\n");
			}
			else if(appData.usbInterface == USB_AUDIO_INTERFACE_PLAYING)
            	process_read_data();
			else{}
			break;
			
		default :
			break;
    
    };

}

static void APP_SetUSBReadBufferReady(USB_DEVICE_AUDIO_TRANSFER_HANDLE handle)
{
    int i=0;
    for (i=0; i<APP_QUEUING_DEPTH; i++)
    {
        if (buf_queue[i].usbReadHandle == handle)
        {
            buf_queue[i].isDataReady = 1;
			buf_queue[i].isUsed = 0;
            break;
        }
    }
}


void APP_USBDeviceAudioEventHandler(USB_DEVICE_AUDIO_INDEX iAudio,
                               USB_DEVICE_AUDIO_EVENT event ,
                               void * pData,
                               uintptr_t context)
{
    volatile USB_DEVICE_AUDIO_EVENT_DATA_INTERFACE_SETTING_CHANGED *interfaceInfo;
    volatile USB_DEVICE_AUDIO_EVENT_DATA_READ_COMPLETE *readEventData;

    uint8_t entityID;
	static uint8_t dacMute = 0;
    uint8_t controlSelector;

    if ( iAudio == 0 )
    {
        switch (event)
        {
            case USB_DEVICE_AUDIO_EVENT_INTERFACE_SETTING_CHANGED:
            {
                /* We have received a request from USB host to change the Interface-
                   Alternate setting.*/
                interfaceInfo = (USB_DEVICE_AUDIO_EVENT_DATA_INTERFACE_SETTING_CHANGED *)pData;
                appData.usbInterface = interfaceInfo->interfaceAlternateSetting; 
                        
                debug_log("audio interface setted %d\r\n",interfaceInfo->interfaceAlternateSetting);
            }
            break;

            case USB_DEVICE_AUDIO_EVENT_READ_COMPLETE:
            {
                //Now send this audio frame to Audio Codec for Playback.
                readEventData = (USB_DEVICE_AUDIO_EVENT_DATA_READ_COMPLETE *)pData;
                APP_SetUSBReadBufferReady(readEventData->handle);
               
            }
            break;

            case USB_DEVICE_AUDIO_EVENT_WRITE_COMPLETE:
            {
            }
            break;
            
            case USB_DEVICE_AUDIO_EVENT_CONTROL_SET_CUR:
            {
                //if(((USB_SETUP_PACKET*)pData)->Recipient == 
                //     USB_SETUP_REQUEST_RECIPIENT_INTERFACE)
                //{
                    entityID = 
                       ((USB_AUDIO_CONTROL_INTERFACE_REQUEST*)pData)->entityID;
                    if ((entityID == APP_ID_FEATURE_UNIT))
                    {
                       controlSelector = 
                          ((USB_AUDIO_FEATURE_UNIT_CONTROL_REQUEST*)
                              pData)->controlSelector;
                       if (controlSelector == USB_AUDIO_MUTE_CONTROL)
                       {
                           //A control write transfer received from Host. 
                           //Now receive data from Host.
                           USB_DEVICE_ControlReceive(appData.usbDevHandle, (void *) &(dacMute), 1);

                       }
                       
                    }

            }
            break;

            case USB_DEVICE_AUDIO_EVENT_CONTROL_GET_CUR:
            {
                entityID = ((USB_AUDIO_CONTROL_INTERFACE_REQUEST*)pData)->entityID;
                if (entityID == APP_ID_FEATURE_UNIT)
                {
                   controlSelector = ((USB_AUDIO_FEATURE_UNIT_CONTROL_REQUEST*)pData)->controlSelector;

                   if (controlSelector == USB_AUDIO_MUTE_CONTROL)
                   {
                       /*Handle Get request*/
                       USB_DEVICE_ControlSend(appData.usbDevHandle, (void *)&(dacMute), 1);
                   }
                }
 
            }
            break;

            case USB_DEVICE_AUDIO_EVENT_CONTROL_SET_MIN:
            case USB_DEVICE_AUDIO_EVENT_CONTROL_GET_MIN:
            case USB_DEVICE_AUDIO_EVENT_CONTROL_SET_MAX:
            case USB_DEVICE_AUDIO_EVENT_CONTROL_GET_MAX:
            case USB_DEVICE_AUDIO_EVENT_CONTROL_SET_RES:
            case USB_DEVICE_AUDIO_EVENT_CONTROL_GET_RES:
            case USB_DEVICE_AUDIO_EVENT_ENTITY_GET_MEM:
                /* Stall request */
                USB_DEVICE_ControlStatus (appData.usbDevHandle, USB_DEVICE_CONTROL_STATUS_ERROR);
            break;
            

            case USB_DEVICE_AUDIO_EVENT_CONTROL_TRANSFER_DATA_RECEIVED:
            {
            
                USB_DEVICE_ControlStatus(appData.usbDevHandle, USB_DEVICE_CONTROL_STATUS_OK );

            }
            break;

            case  USB_DEVICE_AUDIO_EVENT_CONTROL_TRANSFER_DATA_SENT:
            break;

            default:
                debug_log( "Invalid callback" );
            break;

        } //End switch (event)
    }//end of if  if ( iAudio == 0 )

}//End APP_AudioEventCallback()


void APP_USBDeviceEventHandler(USB_DEVICE_EVENT event, 
                               void * pEventData, 
                               uintptr_t context )
{
    volatile USB_DEVICE_EVENT_DATA_CONFIGURED* configuredEventData;

    switch( event )
    {


		case USB_DEVICE_EVENT_SOF:
			// appData.sof++;
			// if( (appData.sof % 100) == 0)
			//	printf("sof %d\r\n",appData.sof);			
			break;

        case USB_DEVICE_EVENT_RESET:
            break;

        case USB_DEVICE_EVENT_DECONFIGURED:
            // USB device is reset or device is de-configured.
            // This means that USB device layer is about to de-initialize
            // all function drivers. So close handles to previously opened
            // function drivers.

            // Also turn ON LEDs to indicate reset/de-configured state.
            /* Switch on red and orange, switch off green */

            break;

        case USB_DEVICE_EVENT_CONFIGURED:
            /* check the configuration */
            configuredEventData = 
                     (USB_DEVICE_EVENT_DATA_CONFIGURED *)pEventData;
            if(configuredEventData->configurationValue == 1)
            {
                /* the device is in configured state */
                /* Switch on green and switch off red and orange */
                USB_DEVICE_AUDIO_EventHandlerSet(0,
                                                 APP_USBDeviceAudioEventHandler ,
                                                 (uintptr_t)NULL);

                /* mark that set configuration is complete */
                appData.isConfigured = 1;
                
                debug_log("usb deviced configured\r\n");
            }
            break;

        case USB_DEVICE_EVENT_SUSPENDED:

            break;

        case USB_DEVICE_EVENT_RESUMED:
        case USB_DEVICE_EVENT_POWER_DETECTED:
            /* VBUS was detected. Notify USB stack about the event */

			//debug_log("usb poweredr\r\n");
            USB_DEVICE_Attach (appData.usbDevHandle);
            break;

        case USB_DEVICE_EVENT_POWER_REMOVED:
            /* VBUS was removed. Notify USB stack about the event*/
            USB_DEVICE_Detach (appData.usbDevHandle);

			break;

        case USB_DEVICE_EVENT_ERROR:
        default:
            break;
    }
} //



 void timer_callback(uintptr_t context){
 
     // we check dma buff underrun here
     // overrun check will be done in  copy2dma_buff
     static unsigned int led_cnt= 0;



	 if(APP_STATE_PLAYING == appData.state){
		 if(isUnderRun() && appData.dmaBuffState != PLAY_DMA_BUF_UNDER_RUN){

			 
			 stop_i2s_tx();

		 	 // PLAY_DMA_BUF_UNDER_RUN will be cleared in copy2dma_buff
			 appData.dmaBuffState = PLAY_DMA_BUF_UNDER_RUN;
		 	 debug_log("under run\r\n");
		 }
		 if(led_cnt % 1000 == 0 && (appData.dmaBuffState != PLAY_DMA_BUF_UNDER_RUN) )
		 	LED1_Toggle();

		 led_cnt++;

		 
	 }

	 
 }
void usb_open(void )
{
    appData.usbDevHandle = USB_DEVICE_Open(USB_DEVICE_INDEX_0,
                                           DRV_IO_INTENT_READWRITE); 

    if (appData.usbDevHandle != USB_DEVICE_HANDLE_INVALID)
    {
        /* Register a callback with device layer to get 
         * event notification (for end point 0) */
        USB_DEVICE_EventHandlerSet(appData.usbDevHandle, 
                                   APP_USBDeviceEventHandler, 0);
        
        
        SYSTICK_TimerCallbackSet(timer_callback,0);

    }
    else
    {
        /* The Device Layer is not ready to be opened. We should try
         * again later. */
    }
}

/*******************************************************************************
 End of File
 */