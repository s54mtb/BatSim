/*----------------------------------------------------------------------------
 * Name:    serial.c
 * Purpose: serial port handling for MCB2140
 * Version: V1.20
 *----------------------------------------------------------------------------
 * This file is part of the uVision/ARM development tools.
 * This software may only be used under the terms of a valid, current,
 * end user licence from KEIL for a compatible version of KEIL software
 * development tools. Nothing else gives you the right to use this software.
 *
 * This software is supplied "AS IS" without warranties of any kind.
 *
 * Copyright (c) 2008 Keil - An ARM Company. All rights reserved.
 *----------------------------------------------------------------------------*/
 
#include <LPC214x.H>                                   /* LPC214x definitions */
#include "serial.h"

#define UART_CLK               (60000000UL)            /* UART Clock is 60.0 MHz */


/*----------------------------------------------------------------------------
  Defines for ring buffers
 *---------------------------------------------------------------------------*/
#define SER_BUF_SIZE               (1024)               /* serial buffer in bytes (power 2) */
#define SER_BUF_MASK               (SER_BUF_SIZE-1ul)  /* buffer size mask */

/* Buffer read / write macros */
#define SER_BUF_RESET(serBuf)      (serBuf.rdIdx = serBuf.wrIdx = 0)
#define SER_BUF_WR(serBuf, dataIn) (serBuf.data[SER_BUF_MASK & serBuf.wrIdx++] = (dataIn))
#define SER_BUF_RD(serBuf)         (serBuf.data[SER_BUF_MASK & serBuf.rdIdx++])   
#define SER_BUF_EMPTY(serBuf)      (serBuf.rdIdx == serBuf.wrIdx)
#define SER_BUF_FULL(serBuf)       (serBuf.rdIdx == serBuf.wrIdx+1)
#define SER_BUF_COUNT(serBuf)      (SER_BUF_MASK & (serBuf.wrIdx - serBuf.rdIdx))

// buffer type
typedef struct __SER_BUF_T {
  unsigned char data[SER_BUF_SIZE];
  unsigned int wrIdx;
  unsigned int rdIdx;
} SER_BUF_T;

unsigned long          ser_txRestart;                  /* NZ if TX restart is required */
unsigned short         ser_lineState;                  /* ((msr << 8) | (lsr)) */
SER_BUF_T              ser_out;                        /* Serial data buffers */
SER_BUF_T              ser_in;


/*----------------------------------------------------------------------------
  Function Prototypes
 *---------------------------------------------------------------------------*/
static __irq void ser_irq_1 (void);
static __irq void def_irq   (void);


/*----------------------------------------------------------------------------
  open the serial port
 *---------------------------------------------------------------------------*/
void ser_OpenPort (void) {

    PINSEL0 &= ~0x000F0000;              /* Enable TxD1 and RxD1             */
    PINSEL0 |=  0x00050000;
}

/*----------------------------------------------------------------------------
  close the serial port
 *---------------------------------------------------------------------------*/
void ser_ClosePort (void) {
    PINSEL0 &= ~0x000F0000;              /* disable TxD1 and RxD1            */

  /* Disable the interrupt in the VIC and UART controllers */
  U1IER       = 0x00;                                  /* Disable UART interrupts */
  VICIntEnClr = (1 << 7);                              /* Disable VIC interrupt */
}

/*----------------------------------------------------------------------------
  initialize the serial port
 *---------------------------------------------------------------------------*/
void ser_InitPort (unsigned long baudrate, unsigned int  databits,
                  unsigned int  parity,   unsigned int  stopbits) {

  unsigned char lcr_p, lcr_s, lcr_d;
  unsigned long dll;

  switch (databits) {
    case 5:                                            /* 5 Data bits */
      lcr_d = 0x00;
    break;
    case 6:                                            /* 6 Data bits */
      lcr_d = 0x01;
    break;
    case 7:                                            /* 7 Data bits */
      lcr_d = 0x02;
    break;
    case 8:                                            /* 8 Data bits */
    default:
      lcr_d = 0x03;
    break;
  }

  switch (stopbits) {
    case 1:                                            /* 1,5 Stop bits */
    case 2:                                            /* 2   Stop bits */
      lcr_s = 0x04;
    break;
    case 0:                                            /* 1   Stop bit */
    default:
      lcr_s = 0x00;
    break;
  }

  switch (parity) {
    case 1:                                            /* Parity Odd */
      lcr_p = 0x08;
    break;
    case 2:                                            /* Parity Even */
      lcr_p = 0x18;
    break;
    case 3:                                            /* Parity Mark */
      lcr_p = 0x28;
    break;
    case 4:                                            /* Parity Space */
      lcr_p = 0x38;
    break;
    case 0:                                            // Parity None */
    default:
      lcr_p = 0x00;
    break;
  }

  SER_BUF_RESET(ser_out);                              /* reset out buffer */
  SER_BUF_RESET(ser_in);                               /* reset in buffer */
  
  /* Note that the pclk is 60,0 MHz.                         */
  /* 60 MHz PCLK generates also rates for 115200, 57600 baud */
  dll = ((UART_CLK / baudrate) / 16UL);

  U1FDR = 0;                                           /* Fractional divider not used */
  U1LCR = 0x80 | lcr_d | lcr_p | lcr_s;                /* Data bits, Parity,   Stop bit */
  U1DLL = dll;                                         /* Baud Rate @ 24 MHZ PCLK */
  U1DLM = (dll >> 8);                                  /* High divisor latch */
  U1LCR = 0x00 | lcr_d | lcr_p | lcr_s;                /* DLAB = 0 */
  U1IER = 0x03;                                        /* Enable TX/RX interrupts */

  ser_txRestart = 1;                                   /* TX fifo is empty */

  /* Set up and enable the UART interrupt in the VIC */
  VICDefVectAddr = (unsigned long)def_irq;              /* Set default interrupt function */
  VICVectAddr7   = (unsigned long)ser_irq_1;            /* Set interrupt function */
  VICVectCntl7   = (0x20 | 7);                          /* UART Interrupt -> IRQ Slot 7 */
  VICIntEnable  |= (1 << 7);                            /* Enable interrupt */ 

}

/*----------------------------------------------------------------------------
  read data from serial port
 *---------------------------------------------------------------------------*/
int ser_Read (char *buffer, const int *length) {
  int bytesToRead, bytesRead;
  
  /* Read *length bytes, block if *bytes are not avaialable	*/
  bytesToRead = *length;
  bytesToRead = (bytesToRead < (*length)) ? bytesToRead : (*length);
  bytesRead = bytesToRead;

  while (bytesToRead--) {
    while (SER_BUF_EMPTY(ser_in));                     /* Block until data is available if none */
    *buffer++ = SER_BUF_RD(ser_in);
  }
  return (bytesRead);  
}

/*----------------------------------------------------------------------------
  write data to the serial port
 *---------------------------------------------------------------------------*/
int ser_Write (const char *buffer, int *length) {
  int  bytesToWrite, bytesWritten;

  /* Write *length bytes */
  bytesToWrite = *length;
  bytesWritten = bytesToWrite;

  while (!SER_BUF_EMPTY(ser_out));                     /* Block until space is available if none */
  while (bytesToWrite) {
      SER_BUF_WR(ser_out, *buffer++);                  /* Read Rx FIFO to buffer */  
      bytesToWrite--;
  }     

  if (ser_txRestart) {
    ser_txRestart = 0;
    U1THR = SER_BUF_RD(ser_out);                       /* Write to the Tx Register */
  }

  return (bytesWritten); 
}

/*----------------------------------------------------------------------------
  check if character(s) are available at the serial interface
 *---------------------------------------------------------------------------*/
void ser_AvailChar (int *availChar) {

  *availChar = SER_BUF_COUNT(ser_in);

}

/*----------------------------------------------------------------------------
  read the line state of the serial port
 *---------------------------------------------------------------------------*/
void ser_LineState (unsigned short *lineState) {

  *lineState = ser_lineState;
  ser_lineState = 0;

}

/*----------------------------------------------------------------------------
  serial port 1 interrupt
 *---------------------------------------------------------------------------*/
static __irq void ser_irq_1 (void) { 
  volatile unsigned long iir;
  
  iir = U1IIR;
   
  if ((iir & 0x4) || (iir & 0xC)) {                    /* RDA or CTI pending */
    while (U1LSR & 0x01) {                             /* Rx FIFO is not empty */
      SER_BUF_WR(ser_in, U1RBR);                       /* Read Rx FIFO to buffer */  
    }
  }
  if ((iir & 0x2)) {                                   /* TXMIS pending */
	if (SER_BUF_COUNT(ser_out) != 0) {
      U1THR = SER_BUF_RD(ser_out);                     /* Write to the Tx FIFO */
      ser_txRestart = 0;
    }
	else {
      ser_txRestart = 1;
	}
  }

  ser_lineState = ((U1MSR << 8) | U1LSR) & 0xE01E;     /* update linestate */

  VICVectAddr = 0;                                     /* acknowledge interrupt */
}


/*----------------------------------------------------------------------------
  LPC214x default interrupt
 *---------------------------------------------------------------------------*/
static __irq void def_irq (void) { 
  VICVectAddr = 0;                                     // acknowledge interrupt
}


