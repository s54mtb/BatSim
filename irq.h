/**
 * @file irq.h
 *
 * Interrupt related Header file

  interrupt related Header file for NXP LPC23xx/24xx. 
 *
 * <pre>
 * (C) COPYRIGHT HYB d.o.o.
 * Levicnikova 34, 8310 Sentjernej, Slovenia 
 * PROJECT VC (RR00001)
 * SYSTEM: NXP LPC2378, uVision RealView MDK ARM
 *</pre>
 *
 * $URL: https://193.77.211.20/svn/RR0001/trunk/5_DO/2_SW/6_PCAK/11_BatSim/irq.h $
 * $Date: 2009-11-27 15:17:44 +0100 (pet, 27 nov 2009) $
 * $Author: marko.pavlin $
 * $Revision: 1244 $
 */

#ifndef __IRQ_H__ 
#define __IRQ_H__

#define I_Bit			0x80
#define F_Bit			0x40

#define SYS32Mode		0x1F
#define IRQ32Mode		0x12
#define FIQ32Mode		0x11

#define IRQ_HIGHEST_PRIORITY	0x01
#define IRQ_LOWEST_PRIORITY		0x0F

#define	IRQ_WDT_INT			0
#define IRQ_SWI_INT			1
#define IRQ_ARM_CORE0_INT	2
#define	IRQ_ARM_CORE1_INT	3
#define	IRQ_TIMER0_INT		4
#define IRQ_TIMER1_INT		5
#define IRQ_UART0_INT		6
#define	IRQ_UART1_INT		7
#define	IRQ_PWM0_1_INT		8
#define IRQ_I2C0_INT		9
#define IRQ_SPI0_INT		10			/* SPI and SSP0 share VIC slot */
#define IRQ_SSP0_INT		10
#define	IRQ_SSP1_INT		11
#define	IRQ_PLL_INT			12
#define IRQ_RTC_INT			13
#define IRQ_EINT0_INT		14
#define IRQ_EINT1_INT		15
#define IRQ_EINT2_INT		16
#define IRQ_EINT3_INT		17
#define	IRQ_ADC0_INT		18
#define IRQ_I2C1_INT		19
#define IRQ_BOD_INT			20
#define IRQ_EMAC_INT		21
#define IRQ_USB_INT			22
#define IRQ_CAN_INT			23
#define IRQ_MCI_INT			24
#define IRQ_GPDMA_INT		25
#define IRQ_TIMER2_INT		26
#define IRQ_TIMER3_INT		27
#define IRQ_UART2_INT		28
#define IRQ_UART3_INT		29
#define IRQ_I2C2_INT		30
#define IRQ_I2S_INT			31

#define IRQ_VIC_SIZE		16

#define IRQ_VECT_ADDR_INDEX	0x100
#define IRQ_VECT_PRIO_INDEX 0x200


/**
 * Vic Init 
 */
void irq_init_vic(void);


/**
 * Install ISR (IRQ) 
 */
unsigned int irq_install( unsigned char IntNumber, void *HandlerAddr, unsigned int Priority );


/**
 * irq_enable 
 */
void irq_enable( unsigned char IntNumber );


/**
 * irq_disable 
 */
void irq_disable( unsigned char IntNumber );


 
static unsigned int sysreg;		/* used as LR register */
#define IENABLE __asm { MRS sysreg, SPSR; MSR CPSR_c, #SYS32Mode }
#define IDISABLE __asm { MSR CPSR_c, #(IRQ32Mode|I_Bit); MSR SPSR_cxsf, sysreg }



/**
 * External interrupts 
 *
 */

#define IRQ_EINT0		0x00000001
#define IRQ_EINT1		0x00000002
#define IRQ_EINT2		0x00000004
#define IRQ_EINT3		0x00000008

#define IRQ_EINT0_EDGE	0x00000001
#define IRQ_EINT1_EDGE	0x00000002
#define IRQ_EINT2_EDGE	0x00000004
#define IRQ_EINT3_EDGE	0x00000008

#define IRQ_EINT0_RISING	0x00000001
#define IRQ_EINT1_RISING	0x00000002
#define IRQ_EINT2_RISING	0x00000004
#define IRQ_EINT3_RISING	0x00000008

extern void IRQ_EINT0_Handler(void) __irq;
extern int IRQ_EINT0Init( void );
extern void IRQ_EINT1_Handler(void) __irq;
extern int IRQ_EINT1Init( void );
extern void IRQ_EINT2_Handler(void) __irq;
extern int IRQ_EINT2Init( void );
extern void IRQ_EINT3_Handler(void) __irq;
extern int IRQ_EINT3Init( void );

extern void IRQ_Timer0_Handler(void) __irq;
extern int IRQ_Timer0Init( void );
extern void IRQ_Timer1_Handler(void) __irq;
extern int IRQ_Timer1Init( void );
extern void IRQ_Timer2_Handler(void) __irq;
extern int IRQ_Timer2Init( void );
extern void IRQ_Timer3_Handler(void) __irq;
extern int IRQ_Timer3Init( void );

extern void IRQ_UART0_Handler(void) __irq;
extern int IRQ_UART0Init( void );
extern void IRQ_UART1_Handler(void) __irq;
extern int IRQ_UART1Init( void );
extern void IRQ_UART2_Handler(void) __irq;
extern int IRQ_UART2Init( void );


#endif /* end __IRQ_H */

/******************************************************************************
**                            End Of File
******************************************************************************/
