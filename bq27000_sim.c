/*----------------------------------------------------------------------------
 * Name:    bq27000_sim.c
 * Purpose: bq27000 simulated hardware
 * Version: V1.00
 *----------------------------------------------------------------------------*/
#include <LPC214X.H>
#include "irq.h"
#include "bq27000_sim.h"
#include <rtl.h>
#include <string.h>
#include <stdio.h>
#include "main.h"


/** Globals */
static int HDQ_State;		// curent state in state machine
static 	char buf[40];
unsigned int rx_last_low;	// last low period
unsigned int rx_last_hi;	// last hi period
unsigned char rx_bit_count=0;
unsigned int rx_periods[8];
unsigned char rx_data = 0;
unsigned char tx_data = 0;
unsigned char tx_bitcount = 0;
int bytecount = 0;


static unsigned char BQ_memory[0x80] = 
{
0x00,0x00,0x01,0x00,0x01,0x00,0x04,0x9c,0x0f,0xa0,0x10,60,0x03,0xe8,0x03,0xe8
,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10,0x11,0x12,0x13,0x14,0x15
,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10,0x11,0x12,0x13,0x14,0x15
,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10,0x11,0x12,0x13,0x14,0x15
,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10,0x11,0x12,0x13,0x14,0x15
,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10,0x11,0x12,0x13,0x14,0x15
,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10,0x11,0x12,0x13,0x14,0x15
,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10,0x11,0x12,0x13,0x14,0x15

};
static int HDQ_monitor = 0;
OS_TID hdq_taskid;
extern void VCOM_Write2Usb(void *buf, int len);


#define HDQ_OUT		IODIR0 |=  (1<<9);    	//P0.9 = OUTPUT
#define HDQ_IN		IODIR0 &= ~(1<<9);  	// P0.9 = INPUT
#define CAPTURE_ENABLE		PINSEL0 |= 	(unsigned int)(1<<21);	 
#define CAPTURE_DISABLE		PINSEL0 &= 	~(unsigned int)(1<<21);	
#define HDQ_OUT1	IOSET0 = (1<<9); 
#define HDQ_OUT0	IOCLR0 = (1<<9); 


/**
 * Write to virtual register 
 */
void bq27_write_reg(unsigned char adr, unsigned char val)
{
	if (adr < 0x80) 
		BQ_memory[adr] = val;
}

/**
 * Read from virtual register 
 */
unsigned char bq27_read_reg(unsigned char adr)
{
    if (adr<0x80)
		return BQ_memory[adr];
	else 
		return 0;
}


/**
 * Function to initialize one wire interface 
 * using PWM controller
 *
 * @return
 *		0 if init finished normally
 *		-1 error installing IRQ handler
 */
int HDQ_init(void)
{

	// Init pins:

	// Init capture pin - for timer1 irq
	// P0[10] -> PCAP1[0]
	PINSEL0 |= 	(unsigned int)(1<<21);	 
	PINSEL0 &= ~(unsigned int)(1<<20);
	PINSEL0 &= ~(1<<18 | 1<<19);

	HDQ_IN;

	// Set IRQ state machine to Idle
	HDQ_State = HDQ_WAITING_BREAK_FALLING_EDGE;		// waiting for break

	/****************** Timer1 Init ****************************/
	T1TCR = 0;
	T1PR = 15; 	// 1us
	T1CCR = 0x00000006;	// capture falling edge with interupt
	T1TCR = 1;  // Counter enable

// Install Timer1 interrupt 

  	VICVectAddr5   = (unsigned long)IRQ_Timer1_Handler;      /* Set interrupt function */
  	VICVectCntl5   = (0x20 | 5);                          /* Timer1 Interrupt -> IRQ Slot 5 */
  	VICIntEnable  |= (1 << 5);                            /* Enable interrupt */ 

    return 0;

}



/**
 * Timer1 interrupt handler
 * The Timer interrupt handler is main state machine for receiving
 * bits via one-wire HDQ
 *
 * IRQ is triggered with capture events via pin CAP1.0 (P0.10)
 */
void IRQ_Timer1_Handler(void) __irq
{

	if (T1IR & 0x00000010) // Capture channel 0 event
	{   //if (T1CR0 > 10) 
		switch (HDQ_State)
		{
			case HDQ_WAITING_BREAK_FALLING_EDGE:  // falling edge received
				T1CCR = 0x00000005;	// capture next rising edge with interupt
				T1TCR = 0x00000003; 	// reset timer
				T1TCR = 0x00000001; 
				HDQ_State = HDQ_WAITING_BREAK_RISING_EDGE;
			break;

			case HDQ_WAITING_BREAK_RISING_EDGE:
				T1CCR = 0x00000006;	// capture next falling edge with interupt
				if (T1CR0 > 190)	// break received
				{
					HDQ_State = HDQ_WAITING_RX_FALING_EDGE;	// continue with normal Rx
					rx_data = 0;
					rx_bit_count = 0;
					isr_evt_set(HDQ_BREAK, hdq_taskid);
				}  
				else 
				{
					HDQ_State = HDQ_WAITING_BREAK_FALLING_EDGE;		// not break, keep waitnig for valid break
				}
			break;

			case HDQ_WAITING_RX_FALING_EDGE:		// normal RX falling edge received
				T1CCR = 0x00000005;	// capture next rising edge with interupt
				T1TCR = 0x00000003; 	// reset timer
				T1TCR = 0x00000001; 
				HDQ_State = HDQ_WAITING_RX_RISING_EDGE;
			break;

			case HDQ_WAITING_RX_RISING_EDGE:   // normal RX rising edge received
				T1CCR = 0x00000006;	// capture next falling edge with interupt
				if (T1CR0 <=190)  //  valid data
				{
					rx_periods[rx_bit_count] = T1CR0;
					if (rx_bit_count < 7)
					{
						rx_bit_count ++ ;
						HDQ_State = HDQ_WAITING_RX_FALING_EDGE;
					}
					else
					{
						rx_data = 0;
					    for (rx_bit_count = 0; rx_bit_count<8; rx_bit_count++)
						{
							if (rx_periods[rx_bit_count]<50) rx_data |= 0x80;  // lsb first shifting
							if (rx_bit_count < 7) rx_data >>= 1; 
						}
						rx_bit_count = 0;
						HDQ_State = HDQ_WAITING_RX_FALING_EDGE;	   // continue reading
						isr_evt_set(HDQ_BYTE_RXED, hdq_taskid);
					}
				}
				else // pulse period not valid ... wait for next break
				{
					HDQ_State = 	HDQ_WAITING_BREAK_FALLING_EDGE;
				}

			break;


		}
	}



	if (T1IR & 0x00000001) // MR0 event
	{
		T1TCR = 0x00000003;	  // reset

		switch (HDQ_State)
		{
			case HDQ_START_WRITING_BYTE:  // start writing byte - set output to 0 here
				HDQ_OUT0;
				tx_bitcount = 0;
				if (tx_data & 0x01) 
					T1MR0 = 20;		// first half of sending bit
				else 
					T1MR0 = 120;
				HDQ_State = HDQ_TX_SECOND_HALF;
			break;

			case HDQ_TX_SECOND_HALF:
				HDQ_OUT1;
				T1MR0 = 200 - T1MR0;	// continue with remainig 200 us 
				tx_bitcount++;
				tx_data >>= 1;
				if (tx_bitcount == 8) 
				{
					isr_evt_set(HDQ_BYTE_SENT, hdq_taskid);
					HDQ_State = HDQ_ST_IDLE;
				}
				else
				{
					HDQ_State = HDQ_TX_FIRST_HALF;
				}
			break;


			case HDQ_TX_FIRST_HALF:
				HDQ_OUT0;
				if (tx_data & 0x01) 
					T1MR0 = 20;		// first half of sending bit
				else 
					T1MR0 = 120;
				HDQ_State = HDQ_TX_SECOND_HALF;
			break;

		}
		T1TCR = 0x00000001;	  // continue
	}

	T1IR = 0x00000011; 			// clear Interrupt flag
   	VICVectAddr = 0;					// Acknowledge Interrupt

}


int set_monitor ( int state )
{
	switch (state)
	{
		case 0: 
		case 1:
			HDQ_monitor = state;
			return 0;

		default: 
			return -1;
	}	
}


int get_monitor(void)
{
	return HDQ_monitor;
}




/*----------------------------------------------------------------------------
  HDQ task - main hdq task
 *---------------------------------------------------------------------------*/
__task void hdq (void) 
{

	U16 event_flags;
	
	unsigned char command_byte;
	unsigned char data_byte;

	HDQ_init();

	while(1)
	{
				
		os_evt_wait_or (0xffff, 0xffff);
		event_flags = os_evt_get ();
		CAPTURE_ENABLE;
		if (event_flags & HDQ_BREAK)			// Break was received
		{
			LED_on(7); LED_off(6); LED_off(5);
			bytecount = 0;
			os_evt_clr(HDQ_BREAK, hdq_taskid);
			if (get_monitor())
			{	
				sprintf(buf, "{B}<");
				VCOM_Write2Usb(buf, strlen(buf));
			}
		}
		
		if (event_flags & HDQ_BYTE_SENT)	   // Byte was sent	  - prepare timer for receiving
		{
			LED_on(6); LED_off(7);
			if (get_monitor())
			{	
				sprintf(buf, ">\n");
				VCOM_Write2Usb(buf, strlen(buf));
			}
			//CAPTURE_ENABLE;
			HDQ_IN;
			HDQ_State = HDQ_WAITING_BREAK_FALLING_EDGE;		// waiting for break
			T1TCR = 0;
			T1MCR = 0;
			T1CCR = 0x00000006;	// capture falling edge with interupt
			T1TCR = 0x00000003; 	// reset timer
			T1TCR = 0x00000001; 
		}


		
		if (event_flags & HDQ_BYTE_RXED)		// Byte received
		{
			LED_on(5); LED_off(7);
			if (bytecount == 0)  // was the first byte after break? == command byte
			{
				command_byte = rx_data;
			}
			else				// next bytes received
			{
				data_byte = rx_data;
			}

			if (command_byte & 0x80) // host wants to write to battery	... check if we got data byte
			{
				if (bytecount == 1) 
				{
					bq27_write_reg(command_byte & 0x7f, data_byte);
					if (get_monitor())
					{	
						sprintf(buf, "S:%02x,%02x\n", command_byte & 0x7f, data_byte);
						VCOM_Write2Usb(buf, strlen(buf));
					}
				}
			}
			else	// host wants to read ... send register byte
			{
				T1TCR = 0x00000003; 	// reset timer

				tx_data = bq27_read_reg(command_byte & 0x7f);

				HDQ_State = HDQ_START_WRITING_BYTE;
				HDQ_OUT1;
				HDQ_OUT;
				T1CCR = 0; // disable capture events

				// Reset at MR0 - response time
				T1MR0 = 100;	// tresponse = 190+20 us
				// Interrupt on MR0, Reset on MR0
				T1MCR = 0x00000003;
				T1TCR = 0x00000001; // re-enable timer

				if (get_monitor())
				{	
					sprintf(buf, "T:%02x,%02x\n", command_byte & 0x7f, data_byte);
					VCOM_Write2Usb(buf, strlen(buf));
				}
			}

			if (bytecount == 0) bytecount=1;	  // control byte received, next is data byte

			os_evt_clr(HDQ_BYTE_RXED, hdq_taskid);
		}


	}

}


