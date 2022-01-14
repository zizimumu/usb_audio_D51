/*******************************************************************************
  MPLAB Harmony Application Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app.h

  Summary:
    This header file provides prototypes and definitions for the application.

  Description:
    This header file provides function prototypes and data type definitions for
    the application.  Some of these are required by the system (such as the
    "APP_Initialize" and "APP_Tasks" prototypes) and some of them are only used
    internally by the application (such as the "APP_STATES" definition).  Both
    are defined here for convenience.
*******************************************************************************/

#ifndef APP_H
#define APP_H

#include "configuration.h"
#include "definitions.h"


// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************


typedef enum
{

    /* Application's state machine's initial state. */
    APP_STATE_INIT=0,
    APP_STATE_USB_CONFIGURED,
    APP_STATE_USB_INIT_READ,
    APP_STATE_PLAYING,
    APP_STATE_PLAY_STOP,
    APP_STATE_PLAY_PAUSE,
    APP_STATE_CODEC_SET_BUFFER_HANDLER,
    APP_STATE_USB_DEVICE_OPEN,
    APP_STATE_WAIT_FOR_CONFIGURATION,
    APP_STATE_WAIT_FOR_USB_AUDIO_STREAM_ENABLE,
    APP_STATE_INITIAL_USB_READ_REQUEST,
    APP_STATE_INITIAL_CODEC_WRITE_REQUEST,
    APP_PROCESS_DATA,
    APP_MUTE_AUDIO_PLAYBACK,
    APP_USB_INTERFACE_ALTERNATE_SETTING_RCVD,
    APP_SAMPLING_FREQUENCY_CHANGE,
    APP_USB_INTERFACE_ALTERNATE_SETTING_RCVD_HP, //headphone
    APP_IDLE,
    APP_STATE_ERROR,

} APP_STATES;

typedef enum{
	APP_RECORD_INIT = 0,
	APP_RECORD_START_CHECK,
	APP_RECORD_WAIT,
	APP_RECORDING,
	APP_RECORD_END

}APP_RECORD_STATES;

typedef enum
{

    /* Application's state machine's initial state. */
    PLAY_DMA_BUF_NORMAL=0,
    PLAY_DMA_BUF_OVER_RUN,
    PLAY_DMA_BUF_UNDER_RUN,

} PLAY_DMA_BUF_STATE;

typedef enum
{

    /* Application's state machine's initial state. */
    USB_AUDIO_INTERFACE_NON=0,
    USB_AUDIO_INTERFACE_PLAYING = 1,
    USB_AUDIO_INTERFACE_CAPTURE = 2,

} USB_AUDIO_INTERFACE_SETTING;







void APP_Initialize ( void );
void APP_Tasks ( void );
void usb_open(void );


#endif /* _APP_H */
/*******************************************************************************
 End of File
 */

