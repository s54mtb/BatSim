#include <LPC214X.H>
#include "ADS8321.h"

void delayus(int us)
{

}


/**
 * Init adc pins and SSP
 */
int ADC_init(void)	 
{

	ADC_PORT_DIR |= ADC_CS;
	ADC_PORT_SET = ADC_CS;
	/*
	Init SPI pins
		P0.17	SCK    PINSEL1(3:2) = 10	   
		P0.18	MISO   PINSEL1(5:4) = 10
		P0.19	MOSI1  PINSEL1(7:6) = 10
			
	 */
	PINSEL1 &= ~((unsigned long int)(1<<2) | 
				(unsigned long int)(1<<4) | 
				(unsigned long int)(1<<6));
	
	PINSEL1 |=  (unsigned long int)(1<<3) | 
				(unsigned long int)(1<<5) | 
				(unsigned long int)(1<<7);
	
		
	
	/*
	SSP0 Setup
		SPH = 1
		SPO = 1
		Mode = SPI
		DSS = 8
		Clock prescaler = 0x00E0 (1MHz)
		==== MM ====
		ADS8341 SPI port:
					
		SCR = PCLK / (CPSDVSR × [SCR+1]) = PCLK / ( 224 * [ 14 + 1]) = 0
		CPSDVSR -- see below
		CPOL = 0 -- clock polarity low between frames
		CPHA = 0 -- data sampled on the first edge --rising edge
		FRF = 00 -- SPI mode
		DSS =  7 -- 8 bit data transfer
	*/
	
	SSPCR0 = 	(unsigned int)(1 << 2)|
				(unsigned int)(1 << 1)|
				(unsigned int)(1 << 0); //0x00C7;
	/*
		SOD = 0 -- slave output disable - irrelevant since in master mode
		MS = 0	-- device is master
		SSE = 1	-- SSP0 on
		LBM = 0 -- no loop back mode
	*/			
	SSPCR1 = 	(unsigned int)(1 << 1); //0x0002; //enable master mode
	
//	SSPCPSR = 0x000A;	// 1.8MHz
	SSPCPSR = 0x00f0;	
	SSPIMSC= 0x0000;	


	return 0;

}

/**
 * Transfer byte via SSP0 
 *
 * Function transfers single byte via SSP0 
   and return byte read within same transfer.
 *
 * @param 
 *   dat byte sent from master to slave
 * @return 
 *   byte sent from slave to master
 */
unsigned char  SSP0_transfer(unsigned char dat) {
  SSPDR  = dat;                	// send data
  while ((SSPSR & 0x10)) ;     	// wait for transfer completed
return SSPDR;
}


/**
 * Get adc readout
 */
signed short ADC_get(void)
{
	unsigned char transfer[3], adcHI, adcLO;
	signed short adc;

	ADC_PORT_CLR = ADC_CS;

	transfer[0] = SSP0_transfer(0);
	transfer[1] = SSP0_transfer(0);
	transfer[2] = SSP0_transfer(0);

	ADC_PORT_SET = ADC_CS;


	adcHI  =  transfer[0]<<6 | transfer[1]>>2;
	adcLO =  transfer[1]<<6 | transfer[2]>>2;
	
	adc = (signed short)(adcHI<<8 | adcLO);

	return adc;

}

/**
 * Get filtered adc readout
 */
signed short ADC_filtered_get(void)
{
	float sum = 0.0;
	int i;
	signed short adc;

	for	(i=0; i<100; i++)
	  sum += ADC_get();

	sum = sum / 100.0;

	adc = (signed short)sum;

	return adc;
}




/**
 * Get current
 */
float ADC_Current(void)
{
	float i;

	i = (float)ADC_filtered_get();
	i = (i - ADC_I_N) / ADC_I_K;

	return i;
}


