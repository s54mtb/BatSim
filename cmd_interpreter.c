/*----------------------------------------------------------------------------
 * Name:    cmd_interpreter.c
 * Purpose: Battery simulator command interpreter
 * Version: V1.00
 *----------------------------------------------------------------------------*/
#include <LPC214X.H>
#include "cmd_interpreter.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dac8531.h"
#include "ADS8321.h"
#include "bq27000_sim.h"
#include "main.h"

extern void CDC_Init (void);
extern void VCOM_Write2Usb(void *buf, int len);
static 	char i_buf[40];


/** Command data structures, tables and constants 
 * 
 *
 ********************************************************************/

#define MAX_CMD_LEN 256

static char cmdstr_buf [1 + MAX_CMD_LEN];
static char argstr_buf [1 + MAX_CMD_LEN];


// Command identifiers	- add more if needed
enum
  {
	REGID_TCOMP=0x7F,
	REGID_DCOMP=0x7E,
	REGID_IMLC=0x7D,
	REGID_PKCFG=0x7C,
	REGID_TAPER=0x7B,
	REGID_DMFSD=0x7A,
	REGID_ISLC=0x79,
	REGID_SEDV1=0x78,
	REGID_SEDVF=0x77,
	REGID_ILMD=0x76,
	REGID_EE_EN=0x6E,
	REGID_CSOC=0x2C,
	REGID_CYCT=0x2A,
	REGID_CYCL=0x28,
	REGID_TTECP=0x26,
	REGID_AP=0x24,
	REGID_SAE=0x22,
	REGID_MLTTE=0x20,
	REGID_MLI=0x1E,
	REGID_STTE=0x1C,
	REGID_SI=0x1A,
	REGID_TTF=0x18,
	REGID_TTE=0x16,
	REGID_AI=0x14,
	REGID_LMD=0x12,
	REGID_CACT=0x10,
	REGID_CACD=0x0E,
	REGID_NAC=0x0C,
	REGID_RSOC=0x0B,
	REGID_FLAGS=0x0A,
	REGID_VOLT=0x08,
	REGID_TEMP=0x06,
	REGID_ARTTE=0x04,
	REGID_AR=0x02,
	REGID_MODE=0x01,
	REGID_CTRL=0x00,

    CID_HELP=1000,
    CID_SET_ID,
    CID_GET_ID,
    CID_CURRENT,
	CID_VOLTAGE,
	CID_POWER,
	CID_SET_REG,
	CID_GET_REG,
	CID_MON,

	CID_CDC_RESET,


    CID_LAST
  };


#define BQADDRCOUNT	36		// number of valid addresses

// command table
struct reg_id_st
  {
  	int address;
  	int bytelength;
  };

// Registers
const struct reg_id_st BQ_registers[BQADDRCOUNT] = {
		{	REGID_TCOMP,	1	},
		{	REGID_DCOMP,	1	},
		{	REGID_IMLC,	1	},
		{	REGID_PKCFG,	1	},
		{	REGID_TAPER,	1	},
		{	REGID_DMFSD,	1	},
		{	REGID_ISLC,	1	},
		{	REGID_SEDV1,	1	},
		{	REGID_SEDVF,	1	},
		{	REGID_ILMD,	1	},
		{	REGID_EE_EN,	1	},
		{	REGID_CSOC,	1	},
		{	REGID_CYCT,	2	},
		{	REGID_CYCL,	2	},
		{	REGID_TTECP,	2	},
		{	REGID_AP,	2	},
		{	REGID_SAE,	2	},
		{	REGID_MLTTE,	2	},
		{	REGID_MLI,	2	},
		{	REGID_STTE,	2	},
		{	REGID_SI,	2	},
		{	REGID_TTF,	2	},
		{	REGID_TTE,	2	},
		{	REGID_AI,	2	},
		{	REGID_LMD,	2	},
		{	REGID_CACT,	2	},
		{	REGID_CACD,	2	},
		{	REGID_NAC,	2	},
		{	REGID_RSOC,	1	},
		{	REGID_FLAGS,	1	},
		{	REGID_VOLT,	2	},
		{	REGID_TEMP,	2	},
		{	REGID_ARTTE,	2	},
		{	REGID_AR,	2	},
		{	REGID_MODE,	1	},
		{	REGID_CTRL,	1	},
};



// command table
struct cmd_st
  {
  const char *cmdstr;
  int id;
  };


/* Command strings - match command with command ID */
const struct cmd_st cmd_tbl [] =
  {
// General commands
    { "HLP",		CID_HELP },	   	// HLP
    { "MONITOR",	CID_MON },	   	
    { "MON",		CID_MON },	   	

// BQ27000 commands
    { "SID",		CID_SET_ID },
    { "GID",		CID_GET_ID },
	{ "SETREG", 	CID_SET_REG},
	{ "SR", 		CID_SET_REG},
	{ "GETREG",		CID_GET_REG},
	{ "GR",			CID_GET_REG},

// BQ27000 registers
	{"TCOMP",REGID_TCOMP },   	//TCOMP
	{"DCOMP",REGID_DCOMP },   	//DCOMP
	{"IMLC",REGID_IMLC },   	//IMLC
	{"PKCFG",REGID_PKCFG },   	//PKCFG
	{"TAPER",REGID_TAPER },   	//TAPER
	{"DMFSD",REGID_DMFSD },   	//DMFSD
	{"ISLC",REGID_ISLC },   	//ISLC
	{"SEDV1",REGID_SEDV1 },   	//SEDV1
	{"SEDVF",REGID_SEDVF },   	//SEDVF
	{"ILMD",REGID_ILMD },   	//ILMD
	{"EE_EN",REGID_EE_EN },   	//EE_EN
	{"CSOC",REGID_CSOC },   	//CSOC
	{"CYCT",REGID_CYCT },   	//CYCT
	{"CYCL",REGID_CYCL },   	//CYCL
	{"TTECP",REGID_TTECP },   	//TTECP
	{"AP",REGID_AP },   		//AP
	{"SAE",REGID_SAE },   		//SAE
	{"MLTTE",REGID_MLTTE },   	//MLTTE
	{"MLI",REGID_MLI },   		//MLI
	{"STTE",REGID_STTE },   	//STTE
	{"SI",REGID_SI },   		//SI
	{"TTF",REGID_TTF },   		//TTF
	{"TTE",REGID_TTE },   		//TTE
	{"AI",REGID_AI },   		//AI
	{"LMD",REGID_LMD },   		//LMD
	{"CACT",REGID_CACT },   	//CACT
	{"CACD",REGID_CACD },   	//CACD
	{"NAC",REGID_NAC },   		//NAC
	{"RSOC",REGID_RSOC },   	//RSOC
	{"FLAGS",REGID_FLAGS },   	//FLAGS
	{"VOLT",REGID_VOLT },   	//VOLT
	{"TEMP",REGID_TEMP },   	//TEMP
	{"ARTTE",REGID_ARTTE },   	//ARTTE
	{"AR",REGID_AR },   		//AR
	{"MODE",REGID_MODE },   	//MODE
	{"CTRL",REGID_CTRL },   	//CTRL


// DAC commands
    { "VOLTAGE",	CID_VOLTAGE },
    { "V",			CID_VOLTAGE },

// ADC commands
    { "I",			CID_CURRENT },
    { "CURRENT",	CID_CURRENT },
    { "POWER",		CID_POWER },

// General command
	{ "RST",        CID_CDC_RESET }

  };



#define CMD_TBL_LEN (sizeof (cmd_tbl) / sizeof (cmd_tbl [0]))

/*********************************************************************
 * Function:        static unsigned char cmdid_search
 * PreCondition:    -
 * Input:           command string  
 * Output:          command identifier
 * Side Effects:    -
 * Overview:        This function searches the cmd_tbl for a specific 
 *					command and returns the ID associated with that 
 *					command or CID_LAST if there is no matching command.
 * Note:            None
 ********************************************************************/
static int cmdid_search (
  char *cmdstr)
{
const struct cmd_st *ctp;

for (ctp = cmd_tbl; ctp < &cmd_tbl [CMD_TBL_LEN]; ctp++)
  {
  if (strcmp (ctp->cmdstr, cmdstr) == 0)
    return (ctp->id);
  }

return (CID_LAST);
}



/*********************************************************************
 * Function:        char *strupr ( char *src)
 * PreCondition:    -
 * Input:           string  
 * Output:          Uppercase of string
 * Side Effects:    -
 * Overview:        change to uppercase
 * Note:            None
 ********************************************************************/
char *strupr ( char *src) 
{
char *s;

for (s = src; *s != '\0'; s++)
  *s = toupper (*s);

return (src);
}




/*********************************************************************
 * Function:        void cmd_proc (const char *cmd)
 * PreCondition:    -
 * Input:           command line  
 * Output:         	None
 * Side Effects:    Depends on command
 * Overview:        This function processes the cmd command.
 * Note:            The "big case" is here
 ********************************************************************/
void cmd_proc (char *cmd)
{
char *argsep;
unsigned int id;
	 //VCOM_Write2Usb(cmd,strlen(cmd));	  // echo command  

/*------------------------------------------------
First, copy the command and convert it to all
uppercase.
------------------------------------------------*/
	strncpy (cmdstr_buf, cmd, sizeof (cmdstr_buf) - 1);
	cmdstr_buf [sizeof (cmdstr_buf) - 1] = '\0';
	strupr (cmdstr_buf);

/*------------------------------------------------
Next, find the end of the first thing in the
buffer.  Since the command ends with a space,
we'll look for that.  NULL-Terminate the command
and keep a pointer to the arguments.
------------------------------------------------*/
	argsep = strchr (cmdstr_buf, ' ');
	
	if (argsep == NULL)
	  {
	  argstr_buf [0] = '\0';
	  }
	else
	  {
	  strcpy (argstr_buf, argsep + 1);
	  *argsep = '\0';
	  }

/*------------------------------------------------
Search for a command ID, then switch on it.  Each
function invoked here.
------------------------------------------------*/
	id = cmdid_search (cmdstr_buf);
	
	switch (id)
	{
		case CID_HELP:
			print_help(1);	
		break;

		case CID_MON:
			cmd_monitor(argstr_buf);
		break;

		case CID_SET_ID:
			cmd_set_id(argstr_buf);
		break;

		case CID_GET_ID:
			cmd_get_id();
		break;

		case CID_SET_REG:
			cmd_set_reg(argstr_buf);
		break;

		case CID_GET_REG:
			cmd_get_reg(argstr_buf);
		break;
		
		case REGID_TCOMP:
		case REGID_DCOMP:
		case REGID_IMLC:
		case REGID_PKCFG:
		case REGID_TAPER:
		case REGID_DMFSD:
		case REGID_ISLC:
		case REGID_SEDV1:
		case REGID_SEDVF:
		case REGID_ILMD:
		case REGID_EE_EN:
		case REGID_CSOC:
		case REGID_CYCT:
		case REGID_CYCL:
		case REGID_TTECP:
		case REGID_AP:
		case REGID_SAE:
		case REGID_MLTTE:
		case REGID_MLI:
		case REGID_STTE:
		case REGID_SI:
		case REGID_TTF:
		case REGID_TTE:
		case REGID_AI:
		case REGID_LMD:
		case REGID_CACT:
		case REGID_CACD:
		case REGID_NAC:
		case REGID_RSOC:
		case REGID_FLAGS:
		case REGID_VOLT:
		case REGID_TEMP:
		case REGID_ARTTE:
		case REGID_AR:
		case REGID_MODE:
		case REGID_CTRL:
			cmd_register(id, argstr_buf);
		break;


  		case CID_CURRENT:
			cmd_current();
		break;

  		case CID_VOLTAGE:
			cmd_voltage(argstr_buf);
		break;

  		case CID_POWER:
			cmd_power();
		break;
		
		case CID_CDC_RESET: 
			cmd_CDC_reset();
		break;	
	
		case CID_LAST:
			print_help(0);	
		break;
	}
}


const char helptext[]=
"Battery simulator\t\t\t\n"
"\t\t\t\n"
"HLP\t\t\tPrint this help\n"
"SID\t<id>\t\tSet ID (id = 0...16777215)\n"
"GID\t\t\tPrint current ID\n"
"SETREG\t<addr> <val>\t\tSet register at address <addr> to value <val>\n"
"SR\t\t\tSame as SETREG\n"
"GETREG\t<addr>\t\tGet register at address <addr>\n"
"GR\t\t\tSame as GETREG\n"
"----------\t----------\t\tEEPROM Registers\n"
"TCOMP\t[<val>]\t\tTemperature Compensation Constants\n"
"DCOMP\t[<val>]\t\tDischarge Rate Compensation Constants\n"
"IMLC\t[<val>]\t\tInitial Max Load Current\n"
"PKCFG\t[<val>]\t\tPack Configuration Values\n"
"TAPER\t[<val>]\t\tAging Estimate Enable, Charge Termination Taper Current\n"
"DMFSD\t[<val>]\t\tDigital Magnitude Filter and Self-Discharge Rate Constants\n"
"ISLC\t[<val>]\t\tInitial Standby Load Current\n"
"SEDV1\t[<val>]\t\tScaled EDV1 Threshold\n"
"SEDVF\t[<val>]\t\tScaled EDVF Threshold\n"
"ILMD\t[<val>]\t\tInitial Last Measured Discharge High Byte\n"
"EE_EN\t[<val>]\t\tEEPROM Program Enable\n"
"----------\t----------\t\tRAM Registers\n"
"CSOC\t[<val>]\t\tCompensated State-of-Charge\n"
"CYCT\t[<val>]\t\tCycle Count Total High - Low Byte\n"
"CYCL\t[<val>]\t\tCycle Count Since Learning Cycle High - Low Byte\n"
"TTECP\t[<val>]\t\tTime-to-Empty At Constant Power High - Low Byte\n"
"AP\t[<val>]\t\tAverage Power High - Low Byte\n"
"SAE\t[<val>]\t\tAvailable Energy High - Low Byte\n"
"MLTTE\t[<val>]\t\tMax Load Time-to-Empty High - Low Byte\n"
"MLI\t[<val>]\t\tMax Load Current High - Low Byte\n"
"STTE\t[<val>]\t\tStandby Time-to-Empty High - Low Byte\n"
"SI\t[<val>]\t\tStandby Current High - Low Byte\n"
"TTF\t[<val>]\t\tTime-to-Full High - Low Byte\n"
"TTE\t[<val>]\t\tTime-to-Empty High - Low Byte\n"
"AI\t[<val>]\t\tAverage Current High - Low Byte\n"
"LMD\t[<val>]\t\tLast Measured Discharge High - Low Byte\n"
"CACT\t[<val>]\t\tTemperature Compensated CACD High - Low Byte\n"
"CACD\t[<val>]\t\tDischarge Compensated NAC High - Low Byte\n"
"NAC\t[<val>]\t\tNominal Available Capacity High - Low Byte\n"
"RSOC\t[<val>]\t\tRelative State-of-Charge\n"
"FLAGS\t[<val>]\t\tStatus Flags\n"
"VOLT\t[<val>]\t\tReported Voltage High - Low Byte\n"
"TEMP\t[<val>]\t\tReported Temperature High - Low Byte\n"
"ARTTE\t[<val>]\t\tAt-Rate Time-to-Empty High - Low Byte\n"
"AR\t[<val>]\t\tAt-Rate High - Low Byte\n"
"MODE\t[<val>]\t\tDevice Mode Register\n"
"CTRL\t[<val>]\t\tDevice Control Register\n"
"----------\t----------\t\tCell generator commands\n"
"VOLTAGE\t[<val>]\t\toutput voltage\n"
"V\t[<val>]\t\tSame as VOLTAGE\n"
"CURRENT\t\t\tGet output current\n"
"I\t\t\tSame as CURRENT\n"
"POWER\t\t\tGet current power (Vout*Iout)\n"
"MONITOR\t<1/0>\t\tTurn on/off HDQ monitor\n\000"
;

const char syntax_error[]="Syntax error. Type HLP to list available commands.\n\000";

void print_help(int n)
{
	switch (n)
	{
		case 1:
			VCOM_Write2Usb((void *)helptext, strlen(helptext));	
		break;

		default: 
		   	VCOM_Write2Usb((void *)syntax_error, strlen(syntax_error));
	}		
}

/**
 * set ID in emulated B27000
Input:
 	string representng numeric value 
Output:
	None
*/
void cmd_set_id(char *str)
{
	unsigned int val;	
	unsigned char hi, med, lo;
	
	if (sscanf(str, "%u", &val) == 1)
	{
		if (val <= 0x00ffffff)
		{
			hi = (unsigned char)((val >> 16) & 0xff);
			med = (unsigned char)((val >> 8) & 0xff);
			lo = (unsigned char)(val & 0xff);
			bq27_write_reg(EEREG_ID1, hi);
			bq27_write_reg(EEREG_ID2, med);
			bq27_write_reg(EEREG_ID3, lo);
			sprintf(i_buf, "OK\n");
		}
		else
		{
			sprintf(i_buf, "Parameter out of range: %u",val);
		}
	}
	else 
	{
		sprintf(i_buf, "Syntax error!\n");
	}

	VCOM_Write2Usb(i_buf, strlen(i_buf) );
}

/**
 * get ID in virtual registers
Input:
 	None 
Output:
	Current ID from emulated BQ27000
*/
void cmd_get_id(void)
{
	unsigned int val;	
	unsigned char hi, med, lo;
//	char buf[40];
	
	hi = bq27_read_reg(EEREG_ID1);
	med = bq27_read_reg(EEREG_ID2);
	lo = bq27_read_reg(EEREG_ID3);
	val = (unsigned int)(hi);
	val <<= 8;
	val |= (unsigned int)(med);
	val <<= 8;
	val |= (unsigned int)(lo);

	sprintf(i_buf, "%u\n", val);

	VCOM_Write2Usb(i_buf, strlen(i_buf) );
}


/**
 * set register in emulated B27000
Input:
 	<address> <value>
	where <address> is string in decimal format
Output:
	None
*/
void cmd_set_reg(char *str)
{
	unsigned int addr, val;
//	char buf[40];
	int i, valid = 0;
	
	if (sscanf(str, "%u %u", &addr, &val) == 2)
	{
		for (i=0; i<BQADDRCOUNT; i++)
		{
			if (addr == BQ_registers[i].address) 
			{
			 	valid = 1;
				break;
			} 
		}
		if (valid)
		{
			switch (BQ_registers[i].bytelength)
			{
				case 1 : 
					if (val<=0xff) 
					{
						bq27_write_reg(addr, val);
						sprintf(i_buf, "OK\n");
					}
					else
					{
						sprintf(i_buf, "8 bit register out of range: %u",val);
					}
				break;

				case 2 : 
					if (val <= 0xffff)
					{
						bq27_write_reg(addr, val & 0xff);  	// lo part
						bq27_write_reg(addr+1, (val>>8) & 0xff); // hi part
						sprintf(i_buf, "OK\n");
					}
					else
					{
						sprintf(i_buf, "16 bit register out of range: %u",val);
					}
				break;
			}
		}
		else
		{
			sprintf(i_buf, "Invalid address: %u",addr);
		}
	}
	else 
	{
		sprintf(i_buf, "Syntax error!\n");
	}

	VCOM_Write2Usb(i_buf, strlen(i_buf) );

}


/**
 * get register in emulated B27000
Input:
 	<address> string in decimal format
Output:
	None
*/
void cmd_get_reg(char *str)
{
	unsigned int addr, val;
//	char buf[40];
	int i, valid = 0;
	
	if (sscanf(str, "%u", &addr) == 1)
	{
		for (i=0; i<BQADDRCOUNT; i++)
		{
			if (addr == BQ_registers[i].address) 
			{
			 	valid = 1;
				break;
			} 
		}
		if (valid)
		{
			switch (BQ_registers[i].bytelength)
			{
				case 1 : 
					val = bq27_read_reg(addr);
					sprintf(i_buf, "%u\n", val);
				break;

				case 2 : 
					val = bq27_read_reg(addr+1);  	// hi part
					val <<= 8;
					val |= bq27_read_reg(addr);     // lo part
					sprintf(i_buf, "%u\n", val);
				break;
			}
		}
		else
		{
			sprintf(i_buf, "Invalid address: %u",addr);
		}
	}
	else 
	{
		sprintf(i_buf, "Syntax error!\n");
	}

	VCOM_Write2Usb(i_buf, strlen(i_buf) );

}

extern OS_ID  tmr1;

void cmd_register(int addr, char *str)
{
	unsigned int val;
//	char buf[40];
	int i;
	int rv;

	rv = sscanf(str, "%u", &val);	

	if (rv == 1)
	{
		for (i=0; i<BQADDRCOUNT; i++)
		{
			if (addr == BQ_registers[i].address) break;
		}

		switch (BQ_registers[i].bytelength)
		{
			case 1 : 
				if (val<=0xff) 
				{
					bq27_write_reg(addr, val);
					if (addr == REGID_RSOC)
					{

						tmr1 = os_tmr_create (50+val, 0x1234);

						IODIR0 |= 0x00000001;
						IOSET0 = 0x00000001;
						//sprintf(i_buf, "OS_TIMER started: %d\n",50+val);
					} //else
					sprintf(i_buf, "OK\n");
				}
				else
				{
					sprintf(i_buf, "8 bit register out of range: %u",val);
				}
			break;

			case 2 : 
				if (val <= 0xffff)
				{
					bq27_write_reg(addr, val & 0xff);  	// lo part
					bq27_write_reg(addr+1, (val>>8) & 0xff); // hi part
					sprintf(i_buf, "OK\n");
				}
				else
				{
					sprintf(i_buf, "16 bit register out of range: %u",val);
				}
			break;
		}
	}
	else 
		if (rv == EOF) // no parameters - just return the register value
		{
		for (i=0; i<BQADDRCOUNT; i++)
		{
			if (addr == BQ_registers[i].address) break;
		}
			switch (BQ_registers[i].bytelength)
			{
				case 1 : 
					val = bq27_read_reg(addr);
					sprintf(i_buf, "%u\n", val);
				break;

				case 2 : 
					val = bq27_read_reg(addr+1);  	// hi part
					val <<= 8;
					val |= bq27_read_reg(addr);     // lo part
					sprintf(i_buf, "%u\n", val);
				break;
			}

		}

		else 
		{
			sprintf(i_buf, "Syntax error!\n");
		}

	VCOM_Write2Usb(i_buf, strlen(i_buf) );

}



/**
 * set or display voltage
Input:
 	string representng numeric value for output voltage
	or None
Output:
	When no input, current voltage is sent to VCOM
*/
void cmd_voltage(char *str)
{
	float val;	
//	char buf[40];
	int rv;

	rv = sscanf(str, "%f", &val);

	switch (rv)
	{
		case 1 : 	// one parameter
			rv = DAC_Voltage(val);
			switch (rv)
			{
				case 1 : sprintf(i_buf, "%fV is too high.\n", val); break;
				case -1 : sprintf(i_buf, "%fV is too low.\n", val); break;
				case 0 : sprintf(i_buf, "OK.\n"); break;
			}
			
		break;

		case EOF : 	// no parameters - return last value
			val = DAC_get_voltage();
			sprintf(i_buf, "%f\n", val);
		break;

		default: 
			sprintf(i_buf, "Syntax error!\n");

	}

	VCOM_Write2Usb(i_buf, strlen(i_buf) );
}
	
/**
 * Enable or disable monitor
Input:
 	string "1" or "0"
Output:
	none
*/
void cmd_monitor(char *str)
{
	int val;	
//	char buf[40];
	int rv;

	rv = sscanf(str, "%u", &val);

	switch (rv)
	{
		case 1 : 	// one parameter
			if ((val==1) | (val==0))
			{
				set_monitor(val);
				sprintf(i_buf, "OK.\n"); 
			}
			else
			{
				sprintf(i_buf, "Invalid parameter. Use 1 or 0.\n"); 
			}		
		break;

		case EOF : 	// no parameters - return last value
			val = get_monitor();
			sprintf(i_buf, "%u\n", val);
		break;

		default: 
			sprintf(i_buf, "Syntax error!\n");

	}

	VCOM_Write2Usb(i_buf, strlen(i_buf) );
}
	
/**
 * get current and voltage, multiply and send result to VCOM
Input:
 	None 
Output:
	Power sent to VCOM
*/
void cmd_power(void)
{
//	char buf[40];
	float p = DAC_get_voltage() * ADC_Current();
	
	sprintf(i_buf, "%f\n",p);
	VCOM_Write2Usb(i_buf, strlen(i_buf) );

}
	  
/**
 * diplay current
Input:
 	None 
Output:
	current in amps sent to VCOM
*/
void cmd_current(void)
{
//	char buf[40];
	
	sprintf(i_buf, "%f\n",ADC_Current());
	VCOM_Write2Usb(i_buf, strlen(i_buf) );

}


void cmd_CDC_reset(void)
{
	CDC_Init();
	sprintf(i_buf, "OK.\n");
	VCOM_Write2Usb(i_buf, strlen(i_buf) );

}


