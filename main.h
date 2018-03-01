/*----------------------------------------------------------------------------
 * Name:    vcomdemo.h
 * Purpose: USB virtual COM port Demo Definitions
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

/* Push Button Definitions */
#define PB_INT0         0x00000400  /* P2.10 */

/* LED Definitions */
#define LED_MSK         0x000000FF  /* P2.0..7 */


__task void vcom (void);
__task void hdq (void);
void VCOM_Write2Usb(void *buf, int len);

void LED_on(U8 led);
void LED_off(U8 led);

