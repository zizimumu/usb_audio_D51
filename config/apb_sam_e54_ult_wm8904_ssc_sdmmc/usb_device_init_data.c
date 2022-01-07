/*******************************************************************************
  System Initialization File

  File Name:
    usb_device_init_data.c

  Summary:
    This file contains source code necessary to initialize the system.

  Description:
    This file contains source code necessary to initialize the system.  It
    implements the "SYS_Initialize" function, defines the configuration bits,
    and allocates any necessary global system resources,
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2018 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *******************************************************************************/
// DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include "configuration.h"
#include "definitions.h"
/**************************************************
 * USB Device Function Driver Init Data
 **************************************************/
	const USB_DEVICE_AUDIO_INIT audioInit0 =
{
	.queueSizeRead = APP_OUT_QUEUING_DEPTH,
	.queueSizeWrite = APP_REC_QUEUING_DEPTH
};


/**************************************************
 * USB Device Layer Function Driver Registration 
 * Table
 **************************************************/
const USB_DEVICE_FUNCTION_REGISTRATION_TABLE funcRegistrationTable[1] =
{
    
	/* Audio Function 0 */
    { 
        .configurationValue = 1,    /* Configuration value */ 
        .interfaceNumber = 0,       /* First interfaceNumber of this function */ 
        .speed = USB_SPEED_HIGH|USB_SPEED_FULL,    /* Function Speed */ 
#ifdef AUDIO_IN_ENABLE        
        .numberOfInterfaces = 3,    /* Number of interfaces */
 #else
		.numberOfInterfaces = 2,
 #endif
        .funcDriverIndex = 0,  /* Index of Audio Function Driver */
        .driver = (void*)USB_DEVICE_AUDIO_FUNCTION_DRIVER,    /* USB Audio function data exposed to device layer */
        .funcDriverInit = (void*)&audioInit0    /* Function driver init data */
    },


};

/*******************************************
 * USB Device Layer Descriptors
 *******************************************/
/*******************************************
 *  USB Device Descriptor 
 *******************************************/

const USB_DEVICE_DESCRIPTOR deviceDescriptor =
{
    0x12,                                                   // Size of this descriptor in bytes
    USB_DESCRIPTOR_DEVICE,                                  // DEVICE descriptor type
    0x0200,                                                 // USB Spec Release Number in BCD format
    0x00,         // Class Code
    0x00,         // Subclass code
    0x00,         // Protocol code


    USB_DEVICE_EP0_BUFFER_SIZE,                             // Max packet size for EP0, see configuration.h
    0x04D8,                                                 // Vendor ID
    0x00FF,                                                 // Product ID				
    0x0100,                                                 // Device release number in BCD format
    0x01,                                                   // Manufacturer string index
    0x02,                                                   // Product string index
	0x00,                                                   // Device serial number string index
    0x01                                                    // Number of possible configurations
};




#define AUDIO_PACKET_SZE(frq)          (uint8_t)(((frq * 2 * 2)/1000) & 0xFF), \
                                       (uint8_t)((((frq * 2 * 2)/1000) >> 8) & 0xFF)
#define SAMPLE_FREQ(frq)               (uint8_t)(frq), (uint8_t)((frq >> 8)), (uint8_t)((frq >> 16))
#define USB_ENDPOINT_TYPE_ISOCHRONOUS                 0x01
#define USB_ENDPOINT_SYNC_TYPE_ASYNC                0x04
#define AUDIO_FEED_UP_EP                     0x82
#define AUDIO_IN_EP 0x83


#define AUDIO_FORMAT_TYPE_I                           0x01
#define AUDIO_FORMAT_TYPE_III                         0x03
#define AUDIO_CONTROL_MUTE                            0x0001


#define MIC_IN_TERMINAL_ID                            3
#define MIC_FU_ID                                     6
#define MIC_OUT_TERMINAL_ID                           7


#ifdef USB_AUDIO_FEEDUP_ENABLE 
	#define FEEDUP_EP_SIZE 0x9
#else
	#define FEEDUP_EP_SIZE 0
#endif

#ifdef AUDIO_IN_ENABLE
	#define AUDIO_RECORD_IN_SIZE 83
#else
	#define AUDIO_RECORD_IN_SIZE 0
#endif


/*******************************************
 *  USB Full Speed Configuration Descriptor
 *******************************************/
const uint8_t fullSpeedConfigurationDescriptor[]=
{
	/* Configuration Descriptor */

    0x09,                                                   // Size of this descriptor in bytes
    USB_DESCRIPTOR_CONFIGURATION,                           // Descriptor Type
    
    USB_DEVICE_16bitTo8bitArrange((110+FEEDUP_EP_SIZE+AUDIO_RECORD_IN_SIZE)),       //(110 Bytes)Size of the Configuration descriptor
#ifdef AUDIO_IN_ENABLE
	3,
#else
    2,                                                      // Number of interfaces in this configuration
#endif
    0x01,                                                   // Index value of this configuration
    0x00,                                                   // Configuration string index
    USB_ATTRIBUTE_DEFAULT | USB_ATTRIBUTE_SELF_POWERED, // Attributes
    100,
	

    /* Descriptor for Function 1 - Audio     */ 
    
    /* USB Speaker Standard Audio Control Interface Descriptor	*/
    0x09,                            // Size of this descriptor in bytes (bLength)
    USB_DESCRIPTOR_INTERFACE,        // INTERFACE descriptor type (bDescriptorType)
    0,                               // Interface Number  (bInterfaceNumber)
    0x00,                            // Alternate Setting Number (bAlternateSetting)
    0x00,                            // Number of endpoints in this intf (bNumEndpoints)
    USB_AUDIO_CLASS_CODE,            // Class code  (bInterfaceClass)
    USB_AUDIO_AUDIOCONTROL,          // Subclass code (bInterfaceSubclass)
    USB_AUDIO_PR_PROTOCOL_UNDEFINED, // Protocol code  (bInterfaceProtocol)
    0x00,                            // Interface string index (iInterface)



    /* USB Speaker Class-specific AC Interface Descriptor  */
#ifdef AUDIO_IN_ENABLE
	0x0a,
#else
    0x09,                           // Size of this descriptor in bytes (bLength)
#endif
    USB_AUDIO_CS_INTERFACE,         // CS_INTERFACE Descriptor Type (bDescriptorType)
    USB_AUDIO_HEADER,               // HEADER descriptor subtype 	(bDescriptorSubtype)
    0x00,0x01,                      /* Audio Device compliant to the USB Audio
                                     * specification version 1.00 (bcdADC) */
#ifdef AUDIO_IN_ENABLE
	0x28+37,0x00,
#else
    0x28,0x00, 
#endif
    								/* Total number of bytes returned for the
                                     * class-specific AudioControl interface
                                     * descriptor. (wTotalLength). Includes the
                                     * combined length of this descriptor header
                                     * and all Unit and Terminal descriptors. */

#ifdef AUDIO_IN_ENABLE
	0x02,
	0x01,
	0x02,
#else

    1,                           /* The number of AudioStreaming interfaces
                                     * in the Audio Interface Collection to which
                                     * this AudioControl interface belongs
                                     * (bInCollection) */
    1,                           /* AudioStreaming interface 1 belongs to this
                                     * AudioControl interface. (baInterfaceNr(1))*/
#endif

    /* USB Speaker Input Terminal Descriptor */
    0x0C,                           // Size of the descriptor, in bytes (bLength)
    USB_AUDIO_CS_INTERFACE,    		// CS_INTERFACE Descriptor Type (bDescriptorType)
    USB_AUDIO_INPUT_TERMINAL,	    // INPUT_TERMINAL descriptor subtype (bDescriptorSubtype)
    0x01,          				    // ID of this Terminal.(bTerminalID)
    0x01,0x01,                      // (wTerminalType)
    0x00,                           // No association (bAssocTerminal)
    0x02,                           // Two Channels (bNrChannels)
    0x03,0x00,                      // (wChannelConfig)
    0x00,                           // Unused.(iChannelNames)
    0x00,                           // Unused. (iTerminal)


    /* USB Speaker Feature Unit Descriptor */
    0x0A,                           // Size of the descriptor, in bytes (bLength)
    USB_AUDIO_CS_INTERFACE,    		// CS_INTERFACE Descriptor Type (bDescriptorType)
    USB_AUDIO_FEATURE_UNIT,         // FEATURE_UNIT  descriptor subtype (bDescriptorSubtype)
    0x05,            				// ID of this Unit ( bUnitID  ).
    0x01,          					// Input terminal is connected to this unit(bSourceID)
    0x01,                           // (bControlSize) //was 0x03
    0x01,                           // (bmaControls(0)) Controls for Master Channel
    0x00,                           // (bmaControls(1)) Controls for Channel 1
    0x00,                           // (bmaControls(2)) Controls for Channel 2
    0x00,			    //  iFeature


    /* USB Speaker Output Terminal Descriptor */
    0x09,                           // Size of the descriptor, in bytes (bLength)
    USB_AUDIO_CS_INTERFACE,    		// CS_INTERFACE Descriptor Type (bDescriptorType)
    USB_AUDIO_OUTPUT_TERMINAL,      // OUTPUT_TERMINAL  descriptor subtype (bDescriptorSubtype)
    0x02,          					// ID of this Terminal.(bTerminalID)
    0x01,0x03,                      // (wTerminalType)See USB Audio Terminal Types.
    0x00,                           // No association (bAssocTerminal)
    0x05,             				// (bSourceID)
    0x00,                           // Unused. (iTerminal)



	
#ifdef AUDIO_IN_ENABLE   
		/* USB Microphone Input Terminal Descriptor */
		0x0c, 	  /* bLength */
		USB_AUDIO_CS_INTERFACE,	  /* bDescriptorType */
		USB_AUDIO_INPUT_TERMINAL,		  /* bDescriptorSubtype */
		MIC_IN_TERMINAL_ID, 				  /* bTerminal ID */
		0x01,								  /* wTerminalType Generic Microphone	0x0201 */
		0x02,
		0x00,								  /* bAssocTerminal */
		0x01,								  /* bNrChannels */
		0x00,								  /* wChannelConfig 0x0000	Mono */
		0x00,
		0x00,								  /* iChannelNames */
		0x00,								  /* iTerminal */
		/* 12 byte*/
	
		/* USB Microphone Audio Feature Unit Descriptor */
		0x09,								  /* bLength */
		USB_AUDIO_CS_INTERFACE,	  /* bDescriptorType */
		USB_AUDIO_FEATURE_UNIT, 		  /* bDescriptorSubtype */
		MIC_FU_ID,							  /* bUnitID */
		MIC_IN_TERMINAL_ID, 				  /* bSourceID */
		0x01,								  /* bControlSize */
		AUDIO_CONTROL_MUTE, 				  /* bmaControls(0) */
		0x00,								  /* bmaControls(1) */
		0x00,								  /* iTerminal */
		/* 09 byte*/
	
		/*USB Microphone Output Terminal Descriptor */
		0x09,								  /* bLength */
		USB_AUDIO_CS_INTERFACE,	  /* bDescriptorType */
		USB_AUDIO_OUTPUT_TERMINAL,		  /* bDescriptorSubtype */
		MIC_OUT_TERMINAL_ID,				  /* bTerminalID */
		0x01,								  /* wTerminalType USB STREAMING 0x0101*/
		0x01,								  
		0x00,								  /* bAssocTerminal */ 
		MIC_FU_ID,							  /* bSourceID */
		0x00,								  /* iTerminal */
		/* 09 byte*/
#endif /* AUDIO_IN_ENABLED   */





    /* USB Speaker Standard AS Interface Descriptor (Alt. Set. 0) */
    0x09,                            // Size of the descriptor, in bytes (bLength)
    USB_DESCRIPTOR_INTERFACE,        // INTERFACE descriptor type (bDescriptorType)
    1,	 // Interface Number  (bInterfaceNumber)
    0x00,                            // Alternate Setting Number (bAlternateSetting)
    0x00,                            // Number of endpoints in this intf (bNumEndpoints)
    USB_AUDIO_CLASS_CODE,            // Class code  (bInterfaceClass)
    USB_AUDIO_AUDIOSTREAMING,        // Subclass code (bInterfaceSubclass)
    0x00,                            // Protocol code  (bInterfaceProtocol)
    0x00,                            // Interface string index (iInterface)


    /* USB Speaker Standard AS Interface Descriptor (Alt. Set. 1) */
    0x09,                            // Size of the descriptor, in bytes (bLength)
    USB_DESCRIPTOR_INTERFACE,        // INTERFACE descriptor type (bDescriptorType)
    1,	 // Interface Number  (bInterfaceNumber)
    0x01,                            // Alternate Setting Number (bAlternateSetting)

#ifdef USB_AUDIO_FEEDUP_ENABLE
	0x02,
#else
    0x01,                            // Number of endpoints in this intf (bNumEndpoints)
#endif
	
    USB_AUDIO_CLASS_CODE,            // Class code  (bInterfaceClass)
    USB_AUDIO_AUDIOSTREAMING,        // Subclass code (bInterfaceSubclass)
    0x00,                            // Protocol code  (bInterfaceProtocol)
    0x00,                            // Interface string index (iInterface)


    /*  USB Speaker Class-specific AS General Interface Descriptor */
    0x07,                           // Size of the descriptor, in bytes (bLength)
    USB_AUDIO_CS_INTERFACE,     	// CS_INTERFACE Descriptor Type (bDescriptorType)
    USB_AUDIO_AS_GENERAL,    		// GENERAL subtype (bDescriptorSubtype)
    0x01,           				// Unit ID of the Output Terminal.(bTerminalLink)
    0x01,                           // Interface delay. (bDelay)
    0x01,0x00,                      // PCM Format (wFormatTag)


    /*  USB Speaker Type 1 Format Type Descriptor */
    0x0B,                           // Size of the descriptor, in bytes (bLength)
    USB_AUDIO_CS_INTERFACE,     	// CS_INTERFACE Descriptor Type (bDescriptorType)
    USB_AUDIO_FORMAT_TYPE ,         // FORMAT_TYPE subtype. (bDescriptorSubtype)
    0x01,                           // FORMAT_TYPE_1. (bFormatType)
    0x02,                           // two channel.(bNrChannels)
    0x02,                           // 2 byte per audio subframe.(bSubFrameSize)
    0x10,                           // 16 bits per sample.(bBitResolution)
    0x01,                           // One frequency supported. (bSamFreqType)
    0x80,0xBB,0x00,                 // Sampling Frequency = 48000 Hz(tSamFreq)


    /*  USB Speaker Standard Endpoint Descriptor */
    0x09,                            // Size of the descriptor, in bytes (bLength)
    USB_DESCRIPTOR_ENDPOINT,         // ENDPOINT descriptor (bDescriptorType)
    1 | USB_EP_DIRECTION_OUT,                            // Endpoint1:OUT (bEndpointAddress)
#ifdef USB_AUDIO_FEEDUP_ENABLE
  USB_ENDPOINT_TYPE_ISOCHRONOUS  | USB_ENDPOINT_SYNC_TYPE_ASYNC,        /* bmAttributes */
  (uint8_t)(USB_MAX_RX_SIZE & 0xff),(uint8_t)((USB_MAX_RX_SIZE>>8)&0xff) ,	
#else
    0x09,                            /* ?(bmAttributes) Isochronous,Adaptive, data endpoint */
    AUDIO_PACKET_SZE(USBD_AUDIO_MAX_FREQ),   // ?(wMaxPacketSize) //48 * 4
#endif

    0x01,                            // One packet per frame.(bInterval)
    0x00,                            // Unused. (bRefresh)
#ifdef USB_AUDIO_FEEDUP_ENABLE
    AUDIO_FEED_UP_EP,                            // Unused. (bSynchAddress)
#else
	0x00,
#endif


    /* USB Speaker Class-specific Isoc. Audio Data Endpoint Descriptor*/
    0x07,                            // Size of the descriptor, in bytes (bLength)
    USB_AUDIO_CS_ENDPOINT,           // CS_ENDPOINT Descriptor Type (bDescriptorType)
    USB_AUDIO_EP_GENERAL,            // GENERAL subtype. (bDescriptorSubtype)
    0x00,                            /* No sampling frequency control, no pitch
                                        control, no packet padding.(bmAttributes)*/
    0x00,                            // Unused. (bLockDelayUnits)
    0x00,0x00,                       // Unused. (wLockDelay)


#ifdef USB_AUDIO_FEEDUP_ENABLE
	  /* ##Endpoint 2 for feedback - Standard Descriptor */
	  0x09,  							/* bLength */
	  USB_DESCRIPTOR_ENDPOINT, 			  /* bDescriptorType */
	  AUDIO_FEED_UP_EP, 				/* bEndpointAddress 2 in endpoint*/
	  0x11, 							  /* bmAttributes */
	  8,0,								  /* wMaxPacketSize in Bytes 3 */
	  1,								  /* bInterval 1ms*/
	  FEED_RATE,						/* bRefresh 1 ~ 9,host will get feedup evary FEED_RATE power of 2*/
	  0x00, 							  /* bSynchAddress */
	  /* 09 byte*/
#endif

	
#ifdef AUDIO_IN_ENABLE
	  /*----------------------------------------- 
							 USB Microphone Audio Streaming Interface 
												 -------------------------------------------------*/
	  /* USB Microphone Standard AS Interface Descriptor - Audio Streaming Zero Bandwith */
	  /* Interface 2, Alternate Setting 0											  */
	  0x09,			/* bLength */
	  USB_DESCRIPTOR_INTERFACE,		/* bDescriptorType */
	  0x02, 								/* bInterfaceNumber */
	  0x00, 								/* bAlternateSetting */
	  0x00, 								/* bNumEndpoints */
	  USB_AUDIO_CLASS_CODE,				/* bInterfaceClass */
	  USB_AUDIO_AUDIOSTREAMING,		/* bInterfaceSubClass */
	  0x00, 			/* bInterfaceProtocol */
	  0x00, 								/* iInterface */
	  /* 09 byte*/
	  
	  /* USB Microphone Standard AS Interface Descriptor - Audio Streaming Operational */
	  /* Interface 2, Alternate Setting 1											*/
	  0x09,			/* bLength */
	  USB_DESCRIPTOR_INTERFACE,		/* bDescriptorType */
	  0x02, 								/* bInterfaceNumber */
	  0x01, 								/* bAlternateSetting */
	  0x01, 								/* bNumEndpoints */
	  USB_AUDIO_CLASS_CODE,				/* bInterfaceClass */
	  USB_AUDIO_AUDIOSTREAMING,		/* bInterfaceSubClass */
	  0x00, 			/* bInterfaceProtocol */
	  0x00, 								/* iInterface */
	  /* 09 byte*/
	  
	  /* USB Microphone Audio Streaming Class-Specific Interface Descriptor */
	  0x07,	/* bLength */
	  USB_AUDIO_CS_INTERFACE,		/* bDescriptorType */
	  0x01,				/* bDescriptorSubtype */
	  MIC_OUT_TERMINAL_ID,					/* bTerminalLink */
	  0x01, 								/* bDelay */
	  0x01, 								/* wFormatTag AUDIO_FORMAT_PCM	0x0001*/
	  0x00,
	  /* 07 byte*/
	  
	  /* USB Speaker Audio Type I Format Interface Descriptor */
	  (0x08 + (3 /** SUPPORTED_FREQ_NBR*/)),	/* bLength */
	  USB_AUDIO_CS_INTERFACE,		/* bDescriptorType */
	  0x02,			/* bDescriptorSubtype */
	  AUDIO_FORMAT_TYPE_I,					/* bFormatType */	
	  0x02, 								/* bNrChannels */
	  0x02, 								/* bSubFrameSize: 2 bytes */
	  16,			 /* bBitResolution (effective bits per sample) */ 
	  1, //SUPPORTED_FREQ_NBR,					 /* bSamFreqType number of supported frequencies */ 
	  SAMPLE_FREQ(RECORD_FREQUENCE),	/* Audio sampling frequency coded on 3 bytes */
	  /* 8 + 3*N byte*/
	  
	  /* USB Microphone Streaming Standard Endpoint Descriptor */
	  0x09,	/* bLength */
	  USB_DESCRIPTOR_ENDPOINT, 		/* bDescriptorType */
	  AUDIO_IN_EP,							/* bEndpointAddress 1 In endpoint*/
	  USB_ENDPOINT_TYPE_ISOCHRONOUS, //USB_ENDPOINT_TYPE_ISOCHRONOUS,								  /* bmAttributes: Isochrnous | Asynchronous */ 
	  (uint8_t)(RECORD_PERIOD_SIZE & 0xff),(uint8_t)((RECORD_PERIOD_SIZE>>8)&0xff),
	
	  0x01, 								/* bInterval must be 1*/
	  0x00, 								/* bRefresh must be 0 */
	  0x00, 								/* bSynchAddress */
	  /* 09 byte*/
	 

	    /* USB Speaker Class-specific Isoc. Audio Data Endpoint Descriptor*/
	    0x07,                            // Size of the descriptor, in bytes (bLength)
	    USB_AUDIO_CS_ENDPOINT,           // CS_ENDPOINT Descriptor Type (bDescriptorType)
	    USB_AUDIO_EP_GENERAL,            // GENERAL subtype. (bDescriptorSubtype)
	    0x00,                            /* No sampling frequency control, no pitch
	                                        control, no packet padding.(bmAttributes)*/
	    0x00,                            // Unused. (bLockDelayUnits)
	    0x00,0x00,                       // Unused. (wLockDelay)

	  
#endif /* AUDIO_IN_ENABLED */  



};

/*******************************************
 * Array of Full speed Configuration 
 * descriptors
 *******************************************/
USB_DEVICE_CONFIGURATION_DESCRIPTORS_TABLE fullSpeedConfigDescSet[1] =
{
    fullSpeedConfigurationDescriptor
};


/**************************************
 *  String descriptors.
 *************************************/
 /*******************************************
 *  Language code string descriptor
 *******************************************/
    const struct
    {
        uint8_t bLength;
        uint8_t bDscType;
        uint16_t string[1];
    }
    sd000 =
    {
        sizeof(sd000),                                      // Size of this descriptor in bytes
        USB_DESCRIPTOR_STRING,                              // STRING descriptor type
        {0x0409}                                            // Language ID
    };
/*******************************************
 *  Manufacturer string descriptor
 *******************************************/
    const struct
    {
        uint8_t bLength;                                    // Size of this descriptor in bytes
        uint8_t bDscType;                                   // STRING descriptor type
        uint16_t string[25];                                // String
    }
    sd001 =
    {
        sizeof(sd001),
        USB_DESCRIPTOR_STRING,
        {'M','i','c','r','o','c','h','i','p',' ','T','e','c','h','n','o','l','o','g','y',' ','I','n','c','.'}
		
    };

/*******************************************
 *  Product string descriptor
 *******************************************/
	const struct
    {
        uint8_t bLength;                                    // Size of this descriptor in bytes
        uint8_t bDscType;                                   // STRING descriptor type
        uint16_t string[27];                                // String
    }
    sd002 =
    {
        sizeof(sd002),
        USB_DESCRIPTOR_STRING,
		{'H','a','r','m','o','n','y',' ','U','S','B',' ','H','e','a','d','s','e','t',' ','E','x','a','m','p','l','e'}
    }; 

/***************************************
 * Array of string descriptors
 ***************************************/
USB_DEVICE_STRING_DESCRIPTORS_TABLE stringDescriptors[3]=
{
    (const uint8_t *const)&sd000,
    (const uint8_t *const)&sd001,
    (const uint8_t *const)&sd002,
};

/*******************************************
 * USB Device Layer Master Descriptor Table 
 *******************************************/
 
const USB_DEVICE_MASTER_DESCRIPTOR usbMasterDescriptor =
{
    &deviceDescriptor,                                      // Full speed descriptor
    1,                                                      // Total number of full speed configurations available
    fullSpeedConfigDescSet,                                 // Pointer to array of full speed configurations descriptors
	NULL, 
	0,
	NULL,
	3,  													// Total number of string descriptors available.
    stringDescriptors,                                      // Pointer to array of string descriptors.
	NULL, 
	NULL
};


/****************************************************
 * USB Device Layer Initialization Data
 ****************************************************/

const USB_DEVICE_INIT usbDevInitData =
{
    /* Number of function drivers registered to this instance of the
       USB device layer */
    .registeredFuncCount = 1,
	
    /* Function driver table registered to this instance of the USB device layer*/
    .registeredFunctions = (USB_DEVICE_FUNCTION_REGISTRATION_TABLE*)funcRegistrationTable,

    /* Pointer to USB Descriptor structure */
    .usbMasterDescriptor = (USB_DEVICE_MASTER_DESCRIPTOR*)&usbMasterDescriptor,

    /* USB Device Speed */
	.deviceSpeed =  USB_SPEED_FULL,
	
	/* Index of the USB Driver to be used by this Device Layer Instance */
    .driverIndex = DRV_USBFSV1_INDEX_0,

    /* Pointer to the USB Driver Functions. */
    .usbDriverInterface = DRV_USBFSV1_DEVICE_INTERFACE,
	
};
// </editor-fold>
