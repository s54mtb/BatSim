/*----------------------------------------------------------------------------
 * Name:    serial.h
 * Purpose: serial port handling definitions
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


/*----------------------------------------------------------------------------
  Serial interface related prototypes
 *---------------------------------------------------------------------------*/
extern void  ser_OpenPort  (void);
extern void  ser_ClosePort (void); 
extern void  ser_InitPort  (unsigned long baudrate, unsigned int databits, unsigned int parity, unsigned int stopbits);
extern void  ser_AvailChar (int *availChar);
extern int   ser_Write     (const char *buffer, int *length);
extern int   ser_Read      (char *buffer, const int *length);
extern void  ser_LineState (unsigned short *lineState);

