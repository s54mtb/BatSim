/*----------------------------------------------------------------------------
 * Name:    main.c
 * Purpose: Battery simulator main program
 * Version: V1.20
 * Note(s): RTX Version
 *----------------------------------------------------------------------------*/

#include <LPC214X.H>                                  /* LPC214x definitions */

#include <RTL.h>
#include <stdio.h>
#include <string.h>

#include "usb.h"
#include "usbcfg.h"
#include "usbhw.h"
#include "usbcore.h"
#include "cdc.h"
#include "cdcuser.h"

#include "serial.h"
#include "main.h"
#include "cmd_interpreter.h"

#include "dac8531.h"
#include "ads8321.h"


#include "bq27000_sim.h"

// Line editor
#define CNTLQ      0x11
#define CNTLS      0x13
#define DEL        0x7F
#define BACKSPACE  0x08
#define CR         0x0D
#define LF         0x0A



OS_TID vcom_taskid;
extern OS_TID hdq_taskid;

extern unsigned char rx_data;

/*----------------------------------------------------------------------------
  Initialises the VCOM port.
  Call this function before using VCOM_putchar or VCOM_getchar
 *---------------------------------------------------------------------------*/
void VCOM_Init(void) {

  CDC_Init ();
}



/*----------------------------------------------------------------------------
  checks the serial state and initiates notification
 *---------------------------------------------------------------------------*/
void VCOM_CheckSerialState (void) {
         unsigned short temp;
  static unsigned short serialState;

  temp = CDC_GetSerialState();
  if (serialState != temp) {
     serialState = temp;
     CDC_NotificationIn();                  /* send SERIAL_STATE notification */
  }
}

/*----------------------------------------------------------------------------
  Writes up to 64 chars to USB buffer 
 *---------------------------------------------------------------------------*/
void _VCOM_Write2Usb(char *buf, int len) {

    if (CDC_DepInEmpty) {
      CDC_DepInEmpty = 0;
	  USB_WriteEP (CDC_DEP_IN, (unsigned char *)buf, len);
    }
  	while(CDC_DepInEmpty == 0);

}



/*----------------------------------------------------------------------------
  Writes to USB buffer
 *---------------------------------------------------------------------------*/
void VCOM_Write2Usb(void *buf, int len) {

char *b = buf;
int l = len;

	while (l > 64)
	{
		_VCOM_Write2Usb(b, 64);
		b = b + 64;
		l = l-64;
	}

	_VCOM_Write2Usb(buf, l);

}


void LED_on(U8 led)
{
	if (led < 8)
		IO1SET = 1<<(led+16);
}

void LED_off(U8 led)
{
	if (led < 8)
		IO1CLR = 1<<(led+16);
}


__task void batsim_init (void) {
  int numAvailByte;

  os_dly_wait(10);							/* Wait for hardware */

  VCOM_Init();                              /* VCOM Initialization */

  USB_Init();                               /* USB Initialization */
  USB_Connect(__TRUE);                      /* USB Connect */
  ser_AvailChar (&numAvailByte);
  while (!USB_Configuration) ;              /* wait until USB is configured */
  vcom_taskid = os_tsk_create(vcom, 1);
//  ???_taskid = os_tsk_create( ???, 1);

//  IO1DIR |= 1<<16;

  os_tsk_delete_self ();                    /* stop init task (done)       */

}

/*----------------------------------------------------------------------------
  initialize battery simulator
 *---------------------------------------------------------------------------*/
__task void batsim_initx (void) {

  char welcome[] = "Battery simulator V1.0\n";
  int numAvailByte;

  IO1DIR |= 0x00FF0000;		// LEDs
  
  DAC_init();								/* DAC Initialization */
  DAC_Voltage(VOUT_MIN);					/* set DAC to minimum output */
//  DAC_Voltage(4.000);					

  ADC_init();								/* init ADC */
  ADC_filtered_get();

  // HDQ_init();

  VCOM_Init();                              /* VCOM Initialization */

  USB_Init();                               /* USB Initialization */
  USB_Connect(__TRUE);                      /* USB Connect */
  ser_AvailChar (&numAvailByte);
  while (!USB_Configuration) ;              /* wait until USB is configured */
  VCOM_Write2Usb(welcome, strlen(welcome));
  vcom_taskid = os_tsk_create(vcom, 1);
  hdq_taskid = os_tsk_create(hdq, 1);
  os_tsk_delete_self ();                    /* stop init task (done)       */

} 


/*----------------------------------------------------------------------------
  virtual com port task
 *---------------------------------------------------------------------------*/
__task void vcom (void) {

  static char serBuf [32];
  static char cmd_line [256];
         int  numBytesToRead, numBytesRead, numAvailByte, i, idx;

  idx = 0;
  while(1)
  {
	  VCOM_CheckSerialState();
	  CDC_OutBufAvailChar (&numAvailByte);
	  if (numAvailByte > 0) {
	      numBytesToRead = numAvailByte > 32 ? 32 : numAvailByte; 
	      numBytesRead = CDC_RdOutBuf (&serBuf[0], &numBytesToRead);
          for (i=0; i<numBytesRead; i++)
		  {
		    switch (serBuf[i])
			{
			  case BACKSPACE:  // just increment counter
			  case DEL:
			    i++;
			  break;

			  case CR:		  // process CR
			    cmd_line[idx] = 0;
			    cmd_proc (cmd_line);
				idx = 0;
			  break;

			  default: 		  // process other characters
			    if (serBuf[i]>=32) 
				{
				  cmd_line[idx] = serBuf[i];
				  if (idx<255) 
				  {
				    idx++;  
				  }
				  else	// send command line to interpreter
				  {
				    cmd_line[idx] = 0;
			        cmd_proc (cmd_line);
				    idx = 0;
				  }
				} // if (serBuf[i]
			} // switch (serBuf[i])
		  
		  } // for (i=0; i<numBytesRead   
	  }	 // if (numAvailByte > 0)
	  os_tsk_pass();
  } // while (1)    

} 



/*----------------------------------------------------------------------------
  Main Program
 *---------------------------------------------------------------------------*/
int main (void) {
  os_sys_init_prio (batsim_init, 10);                    /* Init RTX and start 'demo' */
  while(1);
}
