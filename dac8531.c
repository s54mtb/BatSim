#include <LPC214X.H>
#include "dac8531.h"

volatile static float voltage;


/**
 * Init SPI0 na P0.4 (SCK0 = DAC_SCLK), P0.6 (DAC_DIN) in P0.7 (SYNC)
 */
int DAC_init(void)	 
{

// Najprej init IO pinov


	// P0.4	= SCK0
	PINSEL0 &= ~0x00000300;
	PINSEL0 |= 0x00000100;

	// P0.6 = MOSI
	PINSEL0 &= ~0x00003000;
	PINSEL0 |= 0x00001000;

	// P0.7 = SYNC (CS)
	IO0DIR |= 0x00000080;



/**
1. Set the SPI clock counter register to the desired clock rate.
   The SPI0 rate may be calculated as: PCLK / SPCCR0 value.	The value of the 
   register must also always be greater than or equal to 8.
**/
   	  S0SPCCR = 0x3C;	// 1MHz
/**
2. Set the SPI control register to the CC2420 required settings.
Bit		Desc.		Value
1:0		Reserved	00
2 		BitEnable 	0  	The SPI controller sends and receives 8 bits of data per
						transfer.
3 		CPHA		1 	Data is sampled on the first clock edge of SCK.
4 		CPOL		0 	SCK is active high.
5 		MSTR		1 	The SPI operates in Master mode.
6 		LSBF		0	SPI data is transferred MSB (bit 7) first.
7 		SPIE		0	SPI interrupts are inhibited.
11:8 	BITS 			When bit 2 of this register is 1, this field controls the
						number of bits per transfer... NOT USED
**/
	  S0SPCR = /*0B 0000 0000 0010 1000 =*/ 0x0820;

  return (S0SPSR & 0x08);  /* Return MODF flag in SPI0 status register */

}

unsigned char spi0_transfer(unsigned char dat) {
  S0SPDR  = dat;                	// send data
  while (!(S0SPSR & 0x80)) ;     	// wait for transfer completed
  return S0SPDR;
}

void DAC_Set(unsigned short dat)
{

	IO0CLR = 0x00000080;	// SYNC low
	spi0_transfer(0);
    spi0_transfer((unsigned char)(dat>>8)&0xff);
    spi0_transfer((unsigned char)(dat)&0xff);
	IO0SET = 0x00000080;	// SYNC hi

}


int DAC_Voltage(float v)
{
	float dv;
	unsigned short dac;

	if (v<VOUT_MIN) return -1;
	if (v>VOUT_MAX) return 1;

	dv = DAC_MIN + (DAC_MAX - DAC_MIN) * (v-VOUT_MIN) / (VOUT_MAX - VOUT_MIN);
	dac = (unsigned short)dv;
	DAC_Set(dac);
	voltage = v;

	return 0;
}


float DAC_get_voltage(void)
{
	return voltage;
}


