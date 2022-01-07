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
#include "wm8904.h"


// #define debug_log(...) printf(__VA_ARGS__) 


#define APP_ID_FEATURE_UNIT    0x05




#ifdef USB_AUDIO_FEEDUP_ENABLE
	#define	APP_PERIOD_SIZE USB_MAX_RX_SIZE
#else 
	#define APP_PERIOD_SIZE (48*2*2)  //must be multi size of max endpoint, itherwise submit URB will faild 
#endif



#define APP_DEFAULT_SAMPLE_FREQ 48000
// the I2S TX data register must +2 at 16 bit width mode
#define I2S_DEST 0x43002832
#define I2S_RX_REG 0x43002836

#define DMA_BUF_LEN (32*1024)

#define UNDERRUN_LEVEL (APP_PERIOD_SIZE/2)

#define arraySize(x) (sizeof(x)/sizeof(x[0]))


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
	uint32_t usbInterface_alt;
    // uint32_t activeMicInterfaceAlternateSetting;

     uint32_t sampleFreq;
    int volume;
    volatile unsigned char rxReady;
	volatile unsigned char dmaBuffState;
    uint32_t peroidSize;
    uint8_t dmaBeatSize;
	
	uint8_t feedState;
	uint32_t feedFreq;
	USB_DEVICE_AUDIO_TRANSFER_HANDLE feedHandle;
	
	uint32_t dmaFreeSize;
	uint32_t sof;
	

    bool volMuteMode;
	unsigned int playWritePt;
	//unsigned char *usb_buf[APP_QUEUING_DEPTH];


} APP_DATA;

typedef struct
{
    uint8_t  isUsed;               //Next Buffer for Codec TX
    uint8_t  isDataReady;                  //Next Buffer for USB RX 
    uint32_t dataLen;
	unsigned char *buf;
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

unsigned char usb_buf_rec1[RECORD_PERIOD_SIZE] __ALIGNED(32);
unsigned char usb_buf_rec2[RECORD_PERIOD_SIZE] __ALIGNED(32);
unsigned char usb_buf_rec3[RECORD_PERIOD_SIZE] __ALIGNED(32);
unsigned char usb_buf_rec4[RECORD_PERIOD_SIZE] __ALIGNED(32);
unsigned char usb_buf_rec5[RECORD_PERIOD_SIZE] __ALIGNED(32);
unsigned char usb_buf_rec6[RECORD_PERIOD_SIZE] __ALIGNED(32);

unsigned char usb_buf_feed[4] __ALIGNED(32);


// the last one buf_queue is reservered for feed transfer
APP_PLAYBACK_BUFFER_QUEUE  buf_queue[APP_QUEUING_DEPTH];
APP_PLAYBACK_BUFFER_QUEUE  feed_buf_queue;

// the last one rec_buf_queue is reservered for overrun situation
APP_PLAYBACK_BUFFER_QUEUE  rec_buf_queue[APP_REC_QUEUING_DEPTH+1];


#ifdef FEED_DEBUG
	unsigned short debug_buf[256];
unsigned int  uart_wr = 0;
unsigned int  uart_rd = 0;

#endif




void send_read_request(void)
{
    unsigned int i;
    USB_DEVICE_AUDIO_RESULT ret;
//    USB_DEVICE_AUDIO_TRANSFER_HANDLE usbReadHandle;
    
    for(i=0;i<APP_QUEUING_DEPTH;i++){
        if(!buf_queue[i].isUsed){

			buf_queue[i].isDataReady = 0;
		
            ret = USB_DEVICE_AUDIO_Read(USB_DEVICE_INDEX_0, 
                          &buf_queue[i].usbReadHandle, 
                          1, buf_queue[i].buf, 
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


								
#define FEED_SLOW_STATE 1
#define FEED_FAST_STATE 2
#define FEED_NORMAL_STATE 0

#define FEED_MIN_VALUE (APP_DEFAULT_SAMPLE_FREQ - APP_DEFAULT_SAMPLE_FREQ/8)
#define FEED_MAX_VALUE (APP_DEFAULT_SAMPLE_FREQ + APP_DEFAULT_SAMPLE_FREQ/4)

#define FAST_FEED_STEP    60 //20000  // (sizeof(IsocOutBuff)/2 /(USBD_AUDIO_FREQ/4) ) 
#define SLOW_FEED_STEP  60 //20000 // (sizeof(IsocOutBuff)/2 /(USBD_AUDIO_FREQ/8) ) 


#define FEED_FREQ_2_BUFF(buf,freq) ( *(uint32_t *)buf = ( ( ((uint32_t) freq /1000) << 14) | ((freq %1000) << 4) )  )

#define FEED_2_FREQ(freq) (  (freq)  << 2 & 0xff00 |   (freq)  & 0xff )




void send_feed(uint32_t freeSize)
{
	// USB_DEVICE_AUDIO_TRANSFER_HANDLE handle;
	USB_DEVICE_AUDIO_RESULT ret;
	APP_PLAYBACK_BUFFER_QUEUE *buf;
	static unsigned int cnt =0 ;


#ifdef	FEED_DEBUG
	debug_buf[uart_wr] = freeSize;
	uart_wr++;
	if(uart_wr >= 256)
		uart_wr = 0;

#endif

	buf = &feed_buf_queue;

	// we only send feed when last transfer is completed
	if(buf->isUsed != 0)
		return;
		

	switch(appData.feedState){

		case FEED_NORMAL_STATE:
			appData.feedFreq = APP_DEFAULT_SAMPLE_FREQ;
			if(freeSize >= DMA_BUF_LEN*3/4){
				printf("fast\r\n");
				appData.feedState = FEED_FAST_STATE;
			}
			else if(freeSize <= DMA_BUF_LEN/4){
				printf("slow\r\n");
				appData.feedState = FEED_SLOW_STATE;
			}
			break;
		case FEED_SLOW_STATE:
			//appData.feedFreq -= SLOW_FEED_STEP; 
			//if(appData.feedFreq <= FEED_MIN_VALUE) {
				appData.feedFreq = FEED_MIN_VALUE;
			//}		

			// the threshold should be < DMA_BUF_LEN*3/4, 
			// otherwise feedState would switched to FEED_FAST_STATE after goto FEED_NORMAL_STATE
			if(freeSize >= DMA_BUF_LEN*3/5)  
				appData.feedState = FEED_NORMAL_STATE;
				
			break;
		case FEED_FAST_STATE:
			//appData.feedFreq += SLOW_FEED_STEP; 
			//if(appData.feedFreq >= FEED_MAX_VALUE) {
				appData.feedFreq = FEED_MAX_VALUE;
			//}
			// to avoid too much data buffered in DMA, the free size should not be too small
			if(freeSize <= DMA_BUF_LEN/2)
				appData.feedState = FEED_NORMAL_STATE;
			
			break;		
		default :
			appData.feedFreq = APP_DEFAULT_SAMPLE_FREQ;
			break;
	};


	
	

	//if(buf->isUsed == 0){
		FEED_FREQ_2_BUFF(buf->buf,appData.feedFreq);

		buf->isUsed = 1;
		ret = USB_DEVICE_AUDIO_Write(USB_DEVICE_INDEX_0, &buf->usbReadHandle, 1, buf->buf, 3);
		
		if(ret != USB_DEVICE_AUDIO_RESULT_OK){
			debug_log("send_feed error %d\r\n",ret);
			buf->isUsed = 0;
		}

		//if(cnt % 250 == 0)
		//	printf("feed cnt %d\r\n",cnt);

		//cnt++;
	//}		   


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
	 
	 for(i=0;i<arraySize(buf_queue);i++){
		 buf_queue[i].isDataReady = 0;
		 buf_queue[i].isUsed = 0;
	 	buf_queue[i].dataLen = 0;

	 }

	 feed_buf_queue.isDataReady = 0;
	 feed_buf_queue.isUsed = 0;
	 feed_buf_queue.dataLen = 0;

	 //memset(buf_queue,0,sizeof(buf_queue));

 }
 void ClearRecBuffQueue(void)
 {
	 unsigned int i;
	 
	 for(i=0;i<APP_REC_QUEUING_DEPTH;i++){
		 rec_buf_queue[i].isDataReady = 0;
		 rec_buf_queue[i].isUsed = 0;

	 }


 }
 void InitRecBuffQueue(void)
 {
	 unsigned int i;
	 
	 for(i=0;i<APP_REC_QUEUING_DEPTH;i++){
		 rec_buf_queue[i].isDataReady = 0;
		 rec_buf_queue[i].isUsed = 0;

	 }
	rec_buf_queue[0].buf = usb_buf_rec1;
	 rec_buf_queue[1].buf = usb_buf_rec2;
	 rec_buf_queue[2].buf = usb_buf_rec3;
	 rec_buf_queue[3].buf = usb_buf_rec4;
	 rec_buf_queue[4].buf = usb_buf_rec5;
	 rec_buf_queue[5].buf = usb_buf_rec5;

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


 void stop_i2s_rx(void )
{
	unsigned int reg = I2S_REGS->I2S_CTRLA;

	reg &= ~(1 << 5);

	I2S_REGS->I2S_CTRLA = reg;
	
}

 void start_i2s_rx(void )
{
	unsigned int reg = I2S_REGS->I2S_CTRLA;

	reg |= (1 << 5);

	I2S_REGS->I2S_CTRLA = reg;
	
}


uint32_t copy2dma_buff(unsigned char *buf,unsigned int len)
{
    unsigned int writePt;
    unsigned int readPt;
    unsigned int free_size,gap;
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
        debug_log("over run, free %d, feed %d\r\n",free_size,(appData.feedFreq));
        return 0;
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

    return free_size; 
}


int isUnderRun(void )
{

    unsigned int writePt = appData.playWritePt;
    unsigned int readPt;
    unsigned int gap;
	//static unsigned char dmaBuffState = PLAY_DMA_BUF_NORMAL;
    
    readPt = appData.dmaBeatSize * DMAC_ChannelGetTransferredCount(DMAC_CHANNEL_1);
    

    if(writePt > readPt){
        ///free_size = DMA_BUF_LEN - writePt + readPt;
        gap = writePt - readPt;
    }
	else if(writePt == readPt){
		//free_size = DMA_BUF_LEN;
		gap = DMA_BUF_LEN;
	}
    else{
        //free_size = readPt - writePt;
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
	static unsigned int timer = 0;


	// printf("dma pt 0x%x\r\n",DMAC_ChannelGetTransferredCount(DMAC_CHANNEL_1));
    
     for(i=0;i<APP_QUEUING_DEPTH;i++){
        if(buf_queue[i].isDataReady){

            
            appData.dmaFreeSize = copy2dma_buff(buf_queue[i].buf,buf_queue[i].dataLen);
            
            ret = USB_DEVICE_AUDIO_Read(USB_DEVICE_INDEX_0, 
                          &buf_queue[i].usbReadHandle, 
                          1, buf_queue[i].buf, 
                          appData.peroidSize); //48 Samples

            if(ret != USB_DEVICE_AUDIO_RESULT_OK){
                buf_queue[i].isUsed = 0;
                break;
            }          

            buf_queue[i].isUsed = 1;
            buf_queue[i].isDataReady = 0;

			
			//if((rx_cnt % 250) == 0)
			//	printf("rx cnt %d\r\n",rx_cnt);
			//rx_cnt++;
        }
    }   

#ifdef USB_AUDIO_FEEDUP_ENABLE



	send_feed(appData.dmaFreeSize);

	//if(SYSTICK_msPeriodGet(timer) >= (1<< FEED_RATE -1) ) { // host will read feed data per 8ms
	//	send_feed(appData.dmaFreeSize);
	//	timer = SYSTICK_msCounter();
	//}

#endif

}

int find_valide_rec_buff()
{
	int i;
	
	for(i=0;i<APP_REC_QUEUING_DEPTH;i++){

		if(!rec_buf_queue[i].isDataReady){
			return i;
		}


	}
	return -1;
}

void process_record()
{

	int i;
	USB_DEVICE_AUDIO_RESULT ret;
	static unsigned int rec_cnt = 0;
	
	for(i=0;i<APP_REC_QUEUING_DEPTH;i++){

		if(rec_buf_queue[i].isDataReady && rec_buf_queue[i].isUsed == 0){

			rec_buf_queue[i].isUsed = 1;
			ret = USB_DEVICE_AUDIO_Write(USB_DEVICE_INDEX_0, 
							  &rec_buf_queue[i].usbReadHandle, 
							  2, rec_buf_queue[i].buf, 
							  RECORD_PERIOD_SIZE);

			if(ret != USB_DEVICE_AUDIO_RESULT_OK){
				debug_log("write error %d, SCHAR_MIN %d\r\n",(int)ret,(int)SCHAR_MIN);
			}				  
			
			if(rec_cnt % 1000 == 0)
				printf("send %d\r\n",rec_cnt);
			rec_cnt++;
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

void dma_callback_record(DMAC_TRANSFER_EVENT status, uintptr_t context)
{
	int index;
	static unsigned int dma_index = 0;


	rec_buf_queue[dma_index].isDataReady = 1;
	//rec_buf_queue[x].isUsed = 0;

	
	index = find_valide_rec_buff();
	if(index == -1){
		index = APP_REC_QUEUING_DEPTH ; //reserver last buff for overrun
		//debug_log("rec over\r\n");

	}

	dma_index = index;

	//rec_buf_queue[index].isDataReady = 0;
    DMAC_ChannelTransfer(DMAC_CHANNEL_0,(void *)I2S_RX_REG, (void *) rec_buf_queue[index].buf,RECORD_PERIOD_SIZE);
	//DMAC_ChannelTransfer(DMAC_CHANNEL_0,(void *)I2S_RX_REG ,(void *) &dma_buff[0],RECORD_PERIOD_SIZE);
	//DMAC_ChannelTransfer(DMAC_CHANNEL_0,(void *)I2S_RX_REG ,(void *) &dma_buff[0],96);

	
	//if( (dma_cnt % 1000) == 0)
	//	printf("dma_cnt %d\r\n",dma_cnt);

	//dma_cnt++;
}



void  wait_dma_buff_sync()
{

    unsigned int writePt = appData.playWritePt;
    unsigned int readPt,start;
    unsigned int gap;
	unsigned int delay,time_out;
	//static unsigned char dmaBuffState = PLAY_DMA_BUF_NORMAL;


	start = SYSTICK_msCounter();
	delay = 0;

	// the audio data buffered less than DMA_BUF_LEN/2
	time_out = 1000 / (APP_DEFAULT_SAMPLE_FREQ / (DMA_BUF_LEN/4) ) / 2;

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

		//timer = SYSTICK_msCounter();
		delay = SYSTICK_msPeriodGet(start);// delay must <=5ms

	}while(gap >= UNDERRUN_LEVEL/2 && delay < time_out);



}



void start_player(void)
{

	DMAC_ChannelCallbackRegister(DMAC_CHANNEL_1, dma_callback, 0);
	DMAC_ChannelTransfer(DMAC_CHANNEL_1,(void *) &dma_buff[0],(void *)I2S_DEST ,sizeof(dma_buff));
	start_i2s_tx();
	wm8904_hpout_mute(CODEC_HPOUT_MUTE_OFF);
	
	
	debug_log("playing\r\n");

}

void stop_play()
{
	
	wait_dma_buff_sync();
	stop_i2s_tx();
	wm8904_hpout_mute(CODEC_HPOUT_MUTE_ON);
	DMAC_ChannelDisable(DMAC_CHANNEL_1);
	ClearBuffQueue();
	// no need to cancle all URB, device layer will cancle all URB when inferface is changed to 0

	appData.playWritePt = 0;
	appData.dmaBuffState = PLAY_DMA_BUF_NORMAL;

	debug_log("stopping play\r\n");
}

void start_record()
{
	ClearRecBuffQueue();

	DMAC_ChannelCallbackRegister(DMAC_CHANNEL_0, dma_callback_record, 0);
	//DMAC_ChannelTransfer(DMAC_CHANNEL_0,(void *)I2S_RX_REG ,(void *) &dma_buff[0],96);
	DMAC_ChannelTransfer(DMAC_CHANNEL_0,(void *)I2S_RX_REG, (void *) rec_buf_queue[0].buf,RECORD_PERIOD_SIZE);
	start_i2s_rx();	
}

void stop_record()
{
	stop_i2s_rx();	
	DMAC_ChannelDisable(DMAC_CHANNEL_0);
}

void APP_RecordTasks()
{
	static int state = APP_RECORD_INIT;
	switch(state){
		case APP_RECORD_INIT:
			InitRecBuffQueue();
			state = APP_RECORD_START_CHECK;
			break;
	
		case APP_RECORD_START_CHECK :
			if(appData.isConfigured == 1 && \
				appData.usbInterface == USB_AUDIO_INTERFACE_CAPTURE && \
				appData.usbInterface_alt == 1){

				state = APP_RECORDING;

				start_record();
				debug_log("recording\r\n");
			}


				
		break;
		case APP_RECORDING:
			if(appData.isConfigured == 1 && \
				appData.usbInterface == USB_AUDIO_INTERFACE_CAPTURE && \
				appData.usbInterface_alt == 0){
				
				state = APP_RECORD_START_CHECK;
				
				stop_record();
				debug_log("record  stopping\r\n");

				break;
			
			
			}
			process_record();


		
		break;
		
		default :
		break;

	};
}

void APP_Initialize ( void )
{

}


void APP_PlayTasks ( void )
{  
	static unsigned int timer = 0;
#ifdef FEED_DEBUG
		if(uart_wr	!= uart_rd){
			printf("%d\r\n",debug_buf[uart_rd]);
			uart_rd++;
			if(uart_rd >= 256)
				uart_rd = 0 ;
		}
#endif

            
    switch(appData.state){
        case APP_STATE_INIT :
			usb_open();
			appData.state = APP_STATE_USB_CONFIGURED;
			buf_queue[0].buf = usb_buf1;
			buf_queue[1].buf = usb_buf2;
			feed_buf_queue.buf = usb_buf_feed; // for feed transfer

			
            appData.peroidSize = APP_PERIOD_SIZE;
			appData.dmaBeatSize = 2;	
			appData.sampleFreq = APP_DEFAULT_SAMPLE_FREQ;
			
		break;
		case APP_STATE_USB_CONFIGURED :
            if(appData.isConfigured == 1 && \
				appData.usbInterface == USB_AUDIO_INTERFACE_PLAYING && \
				appData.usbInterface_alt == 1){
                appData.state = APP_STATE_USB_INIT_READ;
				appData.feedState = FEED_NORMAL_STATE;
				appData.feedFreq = APP_DEFAULT_SAMPLE_FREQ;
				appData.dmaFreeSize = DMA_BUF_LEN/2;

				timer = SYSTICK_msCounter();

			
            }		
            break;
        case APP_STATE_USB_INIT_READ :
            // wait for all buff queue data ready, then start player
            if(appData.usbInterface == USB_AUDIO_INTERFACE_PLAYING && appData.usbInterface_alt == 0){
				// if host change inferce very fast, we change state to wait APP_STATE_USB_CONFIGURED
				appData.state = APP_STATE_USB_CONFIGURED;
				break;
			}
			// to void quick switch from host, we wait 2ms
			if(SYSTICK_msPeriodGet(timer) >= 2){
            //if(isAllBuffQueueReady()){
                appData.state = APP_STATE_PLAYING;
				appData.playWritePt = APP_QUEUING_DEPTH*APP_PERIOD_SIZE;
				memset(dma_buff,0,APP_QUEUING_DEPTH*APP_PERIOD_SIZE);

				
				ClearBuffQueue();
				_USB_DEVICE_AUDIO_clear();
                send_read_request();

				//send_read_request();
                start_player();
            //}
			}
			
            break;
        case APP_STATE_PLAYING :

			if(appData.usbInterface == USB_AUDIO_INTERFACE_PLAYING && appData.usbInterface_alt == 0){

				stop_play();
				appData.state = APP_STATE_USB_CONFIGURED;
				//wait_dma_buff_sync();
				//stop_i2s_tx();
				//debug_log("pausing play\r\n");
			}
			else if(appData.usbInterface == USB_AUDIO_INTERFACE_PLAYING){
            	process_read_data();
			}

			break;
			
		default :
			break;
    
    };

}

void APP_Tasks()
{

	//InitRecBuffQueue();
	//start_record();
	//while(1);
	
	

	APP_PlayTasks();
#ifdef AUDIO_IN_ENABLE

	APP_RecordTasks();

#endif

}



static void APP_SetUSBReadBufferReady(USB_DEVICE_AUDIO_TRANSFER_HANDLE handle,uint32_t len)
{
    int i=0;
    for (i=0; i<arraySize(buf_queue); i++)
    {
        if (buf_queue[i].usbReadHandle == handle)
        {
            buf_queue[i].isDataReady = 1;
			buf_queue[i].isUsed = 0;
			buf_queue[i].dataLen = len;
            break;
        }
    }
}

static void APP_CheckFeedTrans(USB_DEVICE_AUDIO_TRANSFER_HANDLE handle,uint32_t len)
{

    if (feed_buf_queue.usbReadHandle == handle)
    {
        feed_buf_queue.isDataReady = 1;
		feed_buf_queue.isUsed = 0;
		feed_buf_queue.dataLen = len;

    }

}


static void APP_SetUSBWrBufferReady(USB_DEVICE_AUDIO_TRANSFER_HANDLE handle,uint32_t len)
{
    int i=0;
    for (i=0; i<APP_REC_QUEUING_DEPTH; i++)
    {
        if (rec_buf_queue[i].usbReadHandle == handle)
        {
            rec_buf_queue[i].isDataReady = 0;
			rec_buf_queue[i].isUsed = 0;
			rec_buf_queue[i].dataLen = len;
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
                appData.usbInterface = interfaceInfo->interfaceNumber; 
				appData.usbInterface_alt = interfaceInfo->interfaceAlternateSetting;
                        
                debug_log("audio interface %d setted %d\r\n",interfaceInfo->interfaceNumber, interfaceInfo->interfaceAlternateSetting);
            }
            break;

            case USB_DEVICE_AUDIO_EVENT_READ_COMPLETE:
            {
                //Now send this audio frame to Audio Codec for Playback.
                readEventData = (USB_DEVICE_AUDIO_EVENT_DATA_READ_COMPLETE *)pData;
				
                APP_SetUSBReadBufferReady(readEventData->handle,readEventData->length);
               
            }
            break;

            case USB_DEVICE_AUDIO_EVENT_WRITE_COMPLETE:
            {
				readEventData = (USB_DEVICE_AUDIO_EVENT_DATA_READ_COMPLETE *)pData;

				// check feed transfer is completed
				APP_CheckFeedTrans(readEventData->handle,readEventData->length);
				APP_SetUSBWrBufferReady(readEventData->handle,readEventData->length);

			
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
	 //static unsigned int feed_cnt= 0;



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
		 //feed_cnt++;

		 
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
