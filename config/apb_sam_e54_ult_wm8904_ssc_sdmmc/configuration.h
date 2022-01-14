/*******************************************************************************
  System Configuration Header

  File Name:
    configuration.h

  Summary:
    Build-time configuration header for the system defined by this project.

  Description:
    An MPLAB Project may have multiple configurations.  This file defines the
    build-time options for a single configuration.

  Remarks:
    This configuration header must not define any prototypes or data
    definitions (or include any files that do).  It only provides macro
    definitions for build-time configuration options

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

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
/*  This section Includes other configuration headers necessary to completely
    define this configuration.
*/

#include "user.h"
#include "toolchain_specifics.h"
#include <stdio.h>
// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

extern "C" {

#endif
// DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: System Configuration
// *****************************************************************************
// *****************************************************************************



// *****************************************************************************
// *****************************************************************************
// Section: System Service Configuration
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Driver Configuration
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Middleware & Other Library Configuration
// *****************************************************************************
// *****************************************************************************

#define USB_AUDIO_FEEDUP_ENABLE 1
#define AUDIO_IN_ENABLE 1
// #define FEED_DEBUG 1
// #define RECORD_DEBUG 1

#define debug_log(...) printf(__VA_ARGS__) 



#define FEED_RATE 2

#define APP_QUEUING_DEPTH  2  

// APP_OUT_IRP_QUEUING_DEPTH should > APP_QUEUING_DEPTH
#define APP_OUT_IRP_QUEUING_DEPTH  (APP_QUEUING_DEPTH + 1)

#define APP_REC_IRP_QUEUING_DEPTH  5


#define SPEAKER_INPUT_ID 0x1
#define SPEAKER_FEATURE_ID 0x5
#define SPEAKER_OUTPUT_ID 0x3


#define MIC_IN_TERMINAL_ID   0x2
#define MIC_FU_ID            0x6
#define MIC_OUT_TERMINAL_ID  0x4


#define USBD_AUDIO_MAX_FREQ 48000
#define RECORD_FREQUENCE 48000
#define RECORD_PERIOD_SIZE (RECORD_FREQUENCE/1000*2*2)
#define RECORD_PACKET_SISE (RECORD_PERIOD_SIZE)


#define USB_MAX_RX_SIZE                ( ( (USBD_AUDIO_MAX_FREQ * 2 * 2)/1000) *2 )


/* Number of Endpoints used */
#if USB_AUDIO_FEEDUP_ENABLE && AUDIO_IN_ENABLE
#define DRV_USBFSV1_ENDPOINTS_NUMBER                        4
#elif USB_AUDIO_FEEDUP_ENABLE || AUDIO_IN_ENABLE
#define DRV_USBFSV1_ENDPOINTS_NUMBER                        3
#else
#define DRV_USBFSV1_ENDPOINTS_NUMBER                        2

#endif

/* The USB Device Layer will not initialize the USB Driver */
#define USB_DEVICE_DRIVER_INITIALIZE_EXPLICIT

/* Maximum device layer instances */
#define USB_DEVICE_INSTANCES_NUMBER                         1

/* EP0 size in bytes */
#define USB_DEVICE_EP0_BUFFER_SIZE                          64

/* Enable SOF Events */
#define USB_DEVICE_SOF_EVENT_ENABLE






/* Maximum instances of Audio function driver */
#define USB_DEVICE_AUDIO_INSTANCES_NUMBER    1 


/* Audio Transfer Queue Size for both read and
   write. Applicable to all instances of the
   function driver */
   // the last queue is reserverd for feed transfer
#define USB_DEVICE_AUDIO_QUEUE_DEPTH_COMBINED (APP_OUT_IRP_QUEUING_DEPTH + APP_REC_IRP_QUEUING_DEPTH+1)

#ifdef AUDIO_IN_ENABLE
/* No of Audio streaming interfaces */
#define USB_DEVICE_AUDIO_MAX_STREAMING_INTERFACES   2

#else
#define USB_DEVICE_AUDIO_MAX_STREAMING_INTERFACES   1

#endif

/* No of alternate settings */
#define USB_DEVICE_AUDIO_MAX_ALTERNATE_SETTING      2


/*** USB Driver Configuration ***/

/* Maximum USB driver instances */
#define DRV_USBFSV1_INSTANCES_NUMBER                        1


/* Enables Device Support */
#define DRV_USBFSV1_DEVICE_SUPPORT                          true
	
/* Disable Host Support */
#define DRV_USBFSV1_HOST_SUPPORT                            false

/* Enable usage of Dual Bank */
#define DRV_USBFSV1_DUAL_BANK_ENABLE                        false

/* Alignment for buffers that are submitted to USB Driver*/ 
#define USB_ALIGN  __ALIGNED(CACHE_LINE_SIZE)



// *****************************************************************************
// *****************************************************************************
// Section: Application Configuration
// *****************************************************************************
// *****************************************************************************


//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

#endif // CONFIGURATION_H
/*******************************************************************************
 End of File
*/
