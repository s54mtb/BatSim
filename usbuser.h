/*----------------------------------------------------------------------------
 * U S B  -  K e r n e l
 *----------------------------------------------------------------------------
 * Name:    usbuser.h
 * Purpose: USB Custom User Definitions
 * Version: V1.20
 * Note(s): RTX version
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

#ifndef __USBUSER_H__
#define __USBUSER_H__


/* USB Device Events */
#define USB_EVT_POWER_ON    0x0001  /* USB Power On */
#define USB_EVT_POWER_OFF   0x0002  /* USB Power Off */
#define USB_EVT_RESET       0x0004  /* USB Bus Reset */
#define USB_EVT_WAKEUP      0x0008  /* USB Remote Wakeup */
#define USB_EVT_SUSPEND     0x0010  /* USB Suspend */
#define USB_EVT_RESUME      0x0020  /* USB Resume */
#define USB_EVT_SOF         0x0040  /* USB Start of Frame */
#define USB_EVT_ERROR       0x0080  /* USB Error */

/* USB Endpoint Events */
#define USB_EVT_SETUP       0x0002  /* Setup Packet */
#define USB_EVT_OUT         0x0004  /* OUT Packet */
#define USB_EVT_IN          0x0008  /*  IN Packet */
#define USB_EVT_OUT_NAK     0x0010  /* OUT Packet - Not Acknowledged */
#define USB_EVT_IN_NAK      0x0020  /*  IN Packet - Not Acknowledged */
#define USB_EVT_OUT_STALL   0x0040  /* OUT Packet - Stalled */
#define USB_EVT_IN_STALL    0x0080  /*  IN Packet - Stalled */
#define USB_EVT_OUT_DMA_EOT 0x0100  /* DMA OUT EP - End of Transfer */
#define USB_EVT_IN_DMA_EOT  0x0200  /* DMA  IN EP - End of Transfer */
#define USB_EVT_OUT_DMA_NDR 0x0400  /* DMA OUT EP - New Descriptor Request */
#define USB_EVT_IN_DMA_NDR  0x0800  /* DMA  IN EP - New Descriptor Request */
#define USB_EVT_OUT_DMA_ERR 0x1000  /* DMA OUT EP - Error */
#define USB_EVT_IN_DMA_ERR  0x2000  /* DMA  IN EP - Error */

/* USB Core Events */
#define USB_EVT_SET_CFG     0x0001  /* Set Configuration */
#define USB_EVT_SET_IF      0x0002  /* Set Interface */
#define USB_EVT_SET_FEATURE 0x0004  /* Set Feature */
#define USB_EVT_CLR_FEATURE 0x0008  /* Clear Feature */

/* USB Task IDs */
extern OS_TID USB_DevTask;          /* USB Device Task ID */
extern OS_TID USB_EPTask[16];       /* USB Endpoint Task ID's */
extern OS_TID USB_CoreTask;         /* USB Core Task ID */

/* USB User Functions */
extern void USB_TaskInit (void);    /* USB Task Initialization */


#endif  /* __USBUSER_H__ */
