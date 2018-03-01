/**
 * @file irq.c
 *
 * Interrupt controller functions.

   Functions for installing Interrupt Service Routines, Enabling IRQ and 
   VIC Initialization .
 * <pre>
 * (C) COPYRIGHT HYB d.o.o.
 * Levicnikova 34, 8310 Sentjernej, Slovenia 
 * PROJECT VC (RR00001)
 * SYSTEM: NXP LPC2378, uVision RealView MDK ARM
 *</pre>
 *
 * $URL: https://193.77.211.20/svn/RR0001/trunk/5_DO/2_SW/6_PCAK/11_BatSim/irq.c $
 * $Date: 2009-11-27 15:17:44 +0100 (pet, 27 nov 2009) $
 * $Author: marko.pavlin $
 * $Revision: 1244 $
 */


#include <LPC23xx.H>
#include <rtl.h>
#include "irq.h"


/**
 * Vic Init 
 *
 * Initialize VIC interrupt controller.
 *
 */
void irq_init_vic(void) 
{
    unsigned int i = 0;
    unsigned int *vect_addr, *vect_prio;
   	
    /* initialize VIC*/
    VICIntEnClr = 0xffffffff;
    VICVectAddr = 0;
    VICIntSelect = 0;

    /* set all the vector and vector control register to 0 */
    for ( i = 0; i < IRQ_VIC_SIZE; i++ )
    {
		vect_addr = (unsigned int *)(VIC_BASE_ADDR + IRQ_VECT_ADDR_INDEX + i*4);
		vect_prio = (unsigned int *)(VIC_BASE_ADDR + IRQ_VECT_PRIO_INDEX + i*4);
		*vect_addr = 0x0;	
		*vect_prio = 0xF;
    }
    return;
}



/**
 * Install ISR (IRQ) 
 *
 * Install interrupt handler
 *
 * @param 
 *   IntNumber 			Interrupt number
 *	 HandlerAddr		interrupt handler address
 *	 Priority			interrupt priority
 * @return 
 *   true or false, return false if IntNum is out of range
 */
unsigned int irq_install( unsigned char IntNumber, void *HandlerAddr, unsigned int Priority )
{
    unsigned int *vect_addr;
    unsigned int *vect_prio;
      
    VICIntEnClr = 1 << IntNumber;	/** Disable Interrupt */
    if ( IntNumber >= IRQ_VIC_SIZE )
    {
		return ( __FALSE );
    }
    else
    {
		/** find first un-assigned VIC address for the handler */
		vect_addr = (unsigned int *)(VIC_BASE_ADDR + IRQ_VECT_ADDR_INDEX + IntNumber*4);
		vect_prio = (unsigned int *)(VIC_BASE_ADDR + IRQ_VECT_PRIO_INDEX + IntNumber*4);
		*vect_addr = (unsigned int)HandlerAddr;	/** set interrupt vector */
		*vect_prio = 0x20 | Priority;
		VICIntEnable = 1 << IntNumber;	/** Enable Interrupt */
		return( __TRUE );
    }
}



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







/*----------------------------------------------------------------------------
 * end of file
 *---------------------------------------------------------------------------*/


