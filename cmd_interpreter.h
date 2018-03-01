/*----------------------------------------------------------------------------
 * Name:    cmd_interpreter.h
 * Purpose: Battery simulator command interpreter
 * Version: V1.00
 *----------------------------------------------------------------------------*/


void cmd_proc (char *cmd);
void set_voltage(char *str);
void send_current(void);
void print_help(int n);
void cmd_current(void);
void cmd_voltage(char *argstr_buf);
void cmd_power(void);
void cmd_set_id(char *argstr_buf);
void cmd_monitor(char *argstr_buf);
void cmd_get_id(void);
void cmd_set_reg(char *argstr_buf);
void cmd_get_reg(char *argstr_buf);
void cmd_register(int id, char *argstr_buf);
void cmd_CDC_reset(void);



