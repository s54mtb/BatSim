/*----------------------------------------------------------------------------
 * Name:    bq27000_sim.h
 * Purpose: bq27000 simulated hardware
 * Version: V1.00
 *----------------------------------------------------------------------------*/

#include <rtl.h>

#define BAT_EV_FL_FINISHED	0x0001
#define BAT_EV_FL_ERROR 0x1000


/* BQ registers */
#define EEREG_TCOMP 	0x7F
#define EEREG_DCOMP 	0x7E
#define EEREG_IMLC 		0x7D
#define EEREG_ID1 		0x7F
#define EEREG_ID2 		0x7E
#define EEREG_ID3 		0x7D
#define EEREG_PKCFG 	0x7C
#define EEREG_TAPER 	0x7B
#define EEREG_DMFSD 	0x7A
#define EEREG_ISLC 		0x79
#define EEREG_SEDV1 	0x78
#define EEREG_SEDVF 	0x77
#define EEREG_ILMD 		0x76
#define EEREG_EE_EN 	0x6E

#define RAMREG_CSOC 	0x2C
#define RAMREG_CYCT 	0x2B
#define RAMREG_CYCL 	0x29
#define RAMREG_TTECP 	0x27
#define RAMREG_AP 		0x25
#define RAMREG_SAE 		0x23
#define RAMREG_MLTTE 	0x21
#define RAMREG_MLI 		0x1F
#define RAMREG_STTE 	0x1D
#define RAMREG_SI 		0x1B
#define RAMREG_TTF 		0x19
#define RAMREG_TTE 		0x17
#define RAMREG_AI 		0x15
#define RAMREG_LMD 		0x13
#define RAMREG_CACT 	0x11
#define RAMREG_CACD 	0x0F
#define RAMREG_NAC 		0x0D
#define RAMREG_RSOC 	0x0B
#define RAMREG_FLAGS 	0x0A
#define RAMREG_VOLT 	0x09
#define RAMREG_TEMP 	0x07
#define RAMREG_ARTTE 	0x05
#define RAMREG_AR 		0x03
#define RAMREG_MODE 	0x01
#define RAMREG_CTRL 	0x00



typedef enum 		// state machine states
{
	HDQ_ST_IDLE,
	HDQ_ST_RESPONSE,
	HDQ_ST_SENDING,
	HDQ_ST_RECEIVING_TIMEOUT,
	HDQ_ST_BREAK_RX,				// break received
	HDQ_WAITING_BREAK_FALLING_EDGE,
	HDQ_WAITING_BREAK_RISING_EDGE,
	HDQ_WAITING_RX_FALING_EDGE,
	HDQ_WAITING_RX_RISING_EDGE,
	HDQ_START_WRITING_BYTE,
	HDQ_TX_FIRST_HALF,
	HDQ_TX_SECOND_HALF
} HDQ_STATES; 

#define HDQ_BREAK 			0x0001		// break received  
#define HDQ_BYTE_SENT		0x0002		// Byte was sent
#define HDQ_BYTE_RXED		0x0004		// Byte was sent

// 
extern unsigned char BQ_memory[0x80];
extern OS_TID hdq_taskid;

int HDQ_init(void);
void bq27_write_reg(unsigned char adr, unsigned char val);
unsigned char bq27_read_reg(unsigned char adr);

// PWM1 Interrupt handler
void IRQ_PWM_Handler(void) __irq;

// Timer1 IRQ handler
void IRQ_TIMER1_Handler(void) __irq;

/**
 * HDQ_Send_Byte() - Initialize PWM1 for sending one BYTE
 * LSB bit is sent from here, then timeout period triggers the IRQ
 * which handles rest of the byte
 */
void HDQ_Send_Byte(U8 b);


int set_monitor ( int state );
int get_monitor(void);

