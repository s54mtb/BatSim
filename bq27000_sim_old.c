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
static int HDQ_TX_State;		// curent state in state machine
static U8  HDQ_bitcount;	// curent bit when sending or receiving byte to/from onewire 
static U8  HDQ_data;		// OWI data register
//OS_TID HDQ_taskID;	// Currently running task
static 	char buf[40];

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


#define HDQ_OUT		PINSEL0 |= (1<<19); PINSEL0 &= ~(1<<18);    //P0.9 = PWM6
#define HDQ_IN		PINSEL0 &= ~(1<<18 | 1<<19);	IODIR0 &= ~(1<<9);  // P0.9 = GPIO, INPUT


/**
 * irq_enable 
 *
 * Inable IRQ
 *
 * @param 
 *   IntNumber 			Interrupt number
 */
void irq_enable( unsigned char IntNumber )
{
	VICIntEnable = 1 << IntNumber;	/* Enable Interrupt */
}



/**
 * irq_disable 
 *
 * Disable interrupt
 *
 * @param 
 *   IntNumber 			Interrupt number
 */
void irq_disable( unsigned char IntNumber )
{
    VICIntEnClr = 1 << IntNumber;	/* Disable Interrupt */
}




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
	PCONP |= 1<<5;			// enable PWM power

	// Init pins:

	// Init capture pin - for timer1 irq
	// P0[10] -> PCAP1[0]
	PINSEL0 |= 	(unsigned int)(1<<21);	 
	PINSEL0 &= ~(unsigned int)(1<<20);

	HDQ_IN;


	/****************** PWM Init ****************************/
	// Prescale Register. The TC is incremented every PR+1 cycles.
	// Set resolution to 1us
	PWMPR = 15-1;	  // 1MHz = 1us

	// set PWM Count Control Register
	// [1:0] = 00: Timer Mode: the TC is incremented when the Prescale Counter matches the Prescale register.
	// 		   01: Counter Mode: the TC is incremented on rising PCAP input
	//         10: Counter Mode: the TC is incremented on falling PCAP input
	//         11: Counter Mode: the TC is incremented on both edges PCAP input
	// [2:1] = 00 => PCAP1.0
	PWMTCR  = 0;	  // Timer Mode: the TC is incremented when Prescale Counter matches the Prescale register.

	// PWM Match Control Register 
	// bit 18=1: an interrupt is generated when PWMMR6 matches the value in the PWMTC
	// bit 19=1: the PWMTC will be reset if PWMMR6 matches it
	// bit 20=0: Stop on PWMMR6
	// PWMMR6 is used as register which defines primary PWM period: 
	//		200us for writing bit from host to BQ27000
	PWMMCR |= (1<<18) | (1<<19); 

	// Enable PWM channel output
	PWMPCR |= (1<<14);  // PWMENA6
	PWMTCR = 3;  // Timer enabled, Timer RESET

	// Set IRQ state machine to Idle
	HDQ_TX_State = HDQ_ST_IDLE;		// set state to idle


	/****************** Timer1 Init ****************************/
	T1TCR = 0;
	T1PR = 15; 	// 1us
	T1CCR = 0x00000006;	// capture falling edge with interupt
	T1TCR = 1;  // Counter enable


// Install PWM interrupt 

  	VICVectAddr8   = (unsigned long)IRQ_PWM_Handler;      /* Set interrupt function */
  	VICVectCntl8   = (0x20 | 8);                          /* PWM Interrupt -> IRQ Slot 8 */
  	VICIntEnable  |= (1 << 8);                            /* Enable interrupt */ 

// Install Timer1 interrupt 

  	VICVectAddr5   = (unsigned long)IRQ_Timer1_Handler;      /* Set interrupt function */
  	VICVectCntl5   = (0x20 | 5);                          /* Timer1 Interrupt -> IRQ Slot 5 */
  	VICIntEnable  |= (1 << 5);                            /* Enable interrupt */ 

    return 0;

}



/**
 * PWM interrupt handler
 * The PWM interrupt handler is main state machine for sending
 * bits via one-wire HDQ
 *
 * IRQ is triggered with MR2 events
 */
void IRQ_PWM_Handler(void) __irq
{

	// Check if IRQ was triggered by timeout in MR2 (main PWM period passed)
	if (PWMIR & 0x00000400) 			// MR2 interrupt - counter reached end of period
	{
		switch (HDQ_TX_State)				// Check the states
		{
			case HDQ_ST_IDLE :		
			break;						// Do nothing, just acknowledge the IRQ

			case HDQ_ST_RESPONSE :		// Simulate response time 200us
				// When timer value reaches MR0 register PWM output is set to 1
				PWMMR5 = 0;		// Reset
				PWMMR6 = 200;	// 200us = period

				if (HDQ_data & 0x01) 		// LSB first
				{
					PWMMR0 = 20;		// send "1": Set at 20us
				}
				else
				{
					PWMMR0 = 120;		// send "0": Set at 120us		
				}
				HDQ_TX_State = HDQ_ST_SENDING;
			break;						
			
			case HDQ_ST_SENDING:		// The device is sending - this is normal state, when PWM reached 200us period
				HDQ_bitcount++;			// Increment the bit count in output byte
				HDQ_data >>= 1;			// shift to next bit

				// Setup periods for next bit
				if (HDQ_data & 0x01)
				{						
					PWMMR0 = 20;		// Send "1": PWM output will Set to 1 at 20us
				}
				else
				{
					PWMMR0 = 120;		// Send "0": PWM output will Set to 1 at 120us		
				}
				
				// Last bit ?	- finish
				if (HDQ_bitcount >= 8) 
				{
					HDQ_TX_State = HDQ_ST_IDLE;	// Return to idle mode
					HDQ_IN;						// Switch to Input mode
					PWMTCR = 3;					// Reset Timer
					HDQ_bitcount = 0;			// reset the bit counter for receive
					HDQ_data = 0;				// reset data byte
					isr_evt_set(HDQ_BYTE_SENT,hdq_taskid);
				}
			break;

		}

		PWMIR |= 0x00000010;	// clear interupt flag
	}
   	VICVectAddr = 0;					// Acknowledge Interrupt
}




/**
 * HDQ_Send_Byte() - Initialize PWM1 for sending one BYTE
 * LSB bit is sent from here, then timeout period triggers the IRQ
 * which handles rest of the byte
 */
void HDQ_Send_Byte(U8 b)
{
	irq_disable(IRQ_TIMER1_INT);	// disable Rx	  
	HDQ_data = b;	   // Set global data register (for handling from Interrupt service)
	HDQ_bitcount = 0;

	HDQ_OUT;

	PWMMR5 = 100;		// Reset
	PWMMR6 = 100;	// 200us = period
	PWMMR0 = 0;	// keep "1" for 200us (response time)
	// Enable state machine with first response pulse
	HDQ_TX_State = HDQ_ST_RESPONSE;

//	HDQ_TX_State = HDQ_ST_SENDING;
	// start PWM timer
	PWMTCR = 1;

}

unsigned int rx_last_low;	// last low period
unsigned int rx_last_hi;	// last hi period
int HDQ_RX_state = HDQ_WAITING_BREAK_FALLING_EDGE;
unsigned char rx_bit_count=0;
unsigned int rx_periods[8];
unsigned char rx_data = 0;

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
	{
		if (T1CR0 > 10)	// ignore short glitches
		switch (HDQ_RX_state)
		{
			case HDQ_WAITING_BREAK_FALLING_EDGE:  // falling edge received
				T1CCR = 0x00000005;	// capture next rising edge with interupt
				T1TCR |= 0x00000002; 	// reset timer
				T1TCR &= ~0x00000002; 
				HDQ_RX_state = HDQ_WAITING_BREAK_RISING_EDGE;
			break;

			case HDQ_WAITING_BREAK_RISING_EDGE:
				T1CCR = 0x00000006;	// capture next falling edge with interupt
				if (T1CR0 > 190)	// break received
				{
					HDQ_RX_state = HDQ_WAITING_RX_FALING_EDGE;	// continue with normal Rx
					rx_data = 0;
					rx_bit_count = 0;
					isr_evt_set(HDQ_BREAK, hdq_taskid);
				}  
				else 
				{
					HDQ_RX_state = HDQ_WAITING_BREAK_FALLING_EDGE;		// not break, keep waitnig for valid break
				}
			break;

			case HDQ_WAITING_RX_FALING_EDGE:		// normal RX falling edge received
				T1CCR = 0x00000005;	// capture next rising edge with interupt
				T1TCR |= 0x00000002; 	// reset timer
				T1TCR &= ~0x00000002; 
				HDQ_RX_state = HDQ_WAITING_RX_RISING_EDGE;
			break;

			case HDQ_WAITING_RX_RISING_EDGE:   // normal RX rising edge received
				T1CCR = 0x00000006;	// capture next falling edge with interupt
				if (T1CR0 <=190)  //  valid data
				{
					rx_periods[rx_bit_count] = T1CR0;
					if (rx_bit_count < 7)
					{
						rx_bit_count ++ ;
						HDQ_RX_state = HDQ_WAITING_RX_FALING_EDGE;
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
						isr_evt_set(HDQ_BYTE_RXED, hdq_taskid);
						HDQ_RX_state = 	HDQ_WAITING_RX_FALING_EDGE;
					}
				}
				else // pulse period not valid ... wait for next break
				{
					HDQ_RX_state = 	HDQ_WAITING_BREAK_FALLING_EDGE;
				}
			break;


		}

		T1IR = 0x00000010; // clear Interrupt flag

	}
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
	int bytecount = 0;

	HDQ_init();

	while(1)
	{
				
		os_evt_wait_or (0xffff, 0xffff);
		event_flags = os_evt_get ();
		
		if (event_flags & HDQ_BREAK)			// Break was received
		{
			LED_on(7); LED_off(6); LED_off(5);
			bytecount = 0;
			os_evt_clr(HDQ_BREAK, hdq_taskid);
//			if (get_monitor())
//			{	
//				sprintf(buf, "{B}<");
//				VCOM_Write2Usb(buf, strlen(buf));
//			}
		}
		
		if (event_flags & HDQ_BYTE_SENT)	   // Byte was sent
		{
			LED_on(6); LED_off(7);
//			if (get_monitor())
//			{	
//				sprintf(buf, ">\n");
//				VCOM_Write2Usb(buf, strlen(buf));
//			}
			irq_enable(IRQ_TIMER1_INT);		// Enable Rx
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
			else	// host wants to read ... send register 
			{
				data_byte = bq27_read_reg(command_byte & 0x7f);
				HDQ_Send_Byte(data_byte);
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


