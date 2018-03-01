
/* ADC calibration data:

	ADC output (signed short) =  ADC_I_K * Ioutput + ADC_I_N

*/

#define ADC_I_K		24475
#define ADC_I_N		-142

#if (ADC_I_K == 0)
	#warning Napaka v definiciji ADC kalibracijskih parametrov!
#endif


// Pins:
	// P0.17 = CLK	(out)
	// P0.18 = Data (in)
	// P0.20 = CS	(out)


#define ADC_CLK   			(unsigned int)(1<<17)
#define ADC_DATA  			(unsigned int)(1<<18)
#define ADC_CS    			(unsigned int)(1<<20)
#define ADC_PORT_DIR 		IO0DIR
#define ADC_PORT_SET 		IO0SET
#define ADC_PORT_CLR 		IO0CLR
#define ADC_PORT_PIN  		IO0PIN

int ADC_init(void)	 ;
signed short ADC_get(void);
signed short ADC_filtered_get(void);
float ADC_Current(void);

