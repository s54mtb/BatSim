
/* DAC calibration data */

#define DAC_MIN	   0x03E2
#define VOUT_MIN   2.5300
#define DAC_MAX	   0xFC8C
#define VOUT_MAX   4.9500


#if (DAC_MAX == DAC_MIN)
	#warning Napaka v definicija DAC kalibracijskih parametrov!
#endif


int DAC_init(void)	 ;
void DAC_Set(unsigned short dat);
int DAC_Voltage(float v);
float DAC_get_voltage(void);

