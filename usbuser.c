/*----------------------------------------------------------------------------
 * U S B  -  K e r n e l
 *----------------------------------------------------------------------------
 * Name:    usbuser.c
 * Purpose: USB Custom User Module
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

#include <RTL.h>

#include "usb.h"
#include "usbcfg.h"
#include "usbhw.h"
#include "usbcore.h"
#include "usbuser.h"

#include "cdcuser.h"


OS_TID USB_DevTask;                         /* USB Device Task ID */
OS_TID USB_EPTask[16];                      /* USB Endpoint Task ID's */
OS_TID USB_CoreTask;                        /* USB Core Task ID */


/*
 *  USB Device Task
 *   Handles USB Device Events
 */

__task void USB_Device (void) {
  U16 evt;

  for (;;) {

    os_evt_wait_or(0xFFFF, 0xFFFF);         /* Wait for an Event */
    evt = os_evt_get();                     /* Get Event Flags */

#if USB_POWER_EVENT
    if (evt & USB_EVT_POWER_ON) {
    }
    if (evt & USB_EVT_POWER_OFF) {
    }
#endif

#if USB_RESET_EVENT
    if (evt & USB_EVT_RESET) {
      USB_ResetCore();
    }
#endif

#if USB_WAKEUP_EVENT
    if (evt & USB_EVT_WAKEUP) {
    }
#endif

#if USB_SUSPEND_EVENT
    if (evt & USB_EVT_SUSPEND) {
    }
#endif

#if USB_RESUME_EVENT
    if (evt & USB_EVT_RESUME) {
    }
#endif

#if USB_SOF_EVENT
    if (evt & USB_EVT_SOF) {
      CDC_StartOfFrame();
    }
#endif

#if USB_ERROR_EVENT
    if (evt & USB_EVT_ERROR) {
    }
#endif

  }
}


/*
 *  USB Endpoint 1 Task
 *   Handles USB Endpoint 1 Events
 */

#if (USB_EP_EVENT & (1 << 1))
__task void USB_EndPoint1 (void) {
         U16 evt;
         U16 temp;
  static U16 serialState;

  for (;;) {
    os_evt_wait_or(0xFFFF, 0xFFFF);         /* Wait for an Event */
    evt = os_evt_get();                     /* Get Event Flags */

	if (evt & USB_EVT_IN) {
      temp = CDC_GetSerialState();
      if (serialState != temp) {
         serialState = temp;
         CDC_NotificationIn();              /* send SERIAL_STATE notification */
      }
    }
  }
}
#endif


/*
 *  USB Endpoint 2 Task
 *   Handles USB Endpoint 2 Events
 */

#if (USB_EP_EVENT & (1 << 2))
__task void USB_EndPoint2 (void) {
  U16 evt;

  for (;;) {
    os_evt_wait_or(0xFFFF, 0xFFFF);         /* Wait for an Event */
    evt = os_evt_get();                     /* Get Event Flags */

    if (evt & USB_EVT_OUT) {
      CDC_BulkOut ();                       /* data received from Host */
    }
    if (evt & USB_EVT_IN) {
        CDC_BulkIn ();                      /* data expected from Host */
    }
  }
}
#endif


/*
 *  USB Endpoint 3 Task
 *   Handles USB Endpoint 3 Events
 */

#if (USB_EP_EVENT & (1 << 3))
__task void USB_EndPoint3 (void) {
}
#endif


/*
 *  USB Endpoint 4 Task
 *   Handles USB Endpoint 4 Events
 */

#if (USB_EP_EVENT & (1 << 4))
__task void USB_EndPoint4 (void) {
}
#endif


/*
 *  USB Endpoint 5 Task
 *   Handles USB Endpoint 5 Events
 */

#if (USB_EP_EVENT & (1 << 5))
__task void USB_EndPoint5 (void) {
}
#endif


/*
 *  USB Endpoint 6 Task
 *   Handles USB Endpoint 6 Events
 */

#if (USB_EP_EVENT & (1 << 6))
__task void USB_EndPoint6 (void) {
}
#endif


/*
 *  USB Endpoint 7 Task
 *   Handles USB Endpoint 7 Events
 */

#if (USB_EP_EVENT & (1 << 7))
__task void USB_EndPoint7 (void) {
}
#endif


/*
 *  USB Endpoint 8 Task
 *   Handles USB Endpoint 8 Events
 */

#if (USB_EP_EVENT & (1 << 8))
__task void USB_EndPoint8 (void) {
}
#endif


/*
 *  USB Endpoint 9 Task
 *   Handles USB Endpoint 9 Events
 */

#if (USB_EP_EVENT & (1 << 9))
__task void USB_EndPoint9 (void) {
}
#endif


/*
 *  USB Endpoint 10 Task
 *   Handles USB Endpoint 10 Events
 */

#if (USB_EP_EVENT & (1 << 10))
__task void USB_EndPoint10 (void) {
}
#endif


/*
 *  USB Endpoint 11 Task
 *   Handles USB Endpoint 11 Events
 */

#if (USB_EP_EVENT & (1 << 11))
__task void USB_EndPoint11 (void) {
}
#endif


/*
 *  USB Endpoint 12 Task
 *   Handles USB Endpoint 12 Events
 */

#if (USB_EP_EVENT & (1 << 12))
__task void USB_EndPoint12 (void) {
}
#endif


/*
 *  USB Endpoint 13 Task
 *   Handles USB Endpoint 13 Events
 */

#if (USB_EP_EVENT & (1 << 13))
__task void USB_EndPoint13 (void) {
}
#endif


/*
 *  USB Endpoint 14 Task
 *   Handles USB Endpoint 14 Events
 */

#if (USB_EP_EVENT & (1 << 14))
__task void USB_EndPoint14 (void) {
}
#endif


/*
 *  USB Endpoint 15 Task
 *   Handles USB Endpoint 15 Events
 */

#if (USB_EP_EVENT & (1 << 15))
__task void USB_EndPoint15 (void) {
}
#endif


/*
 *  USB Core Task
 *   Handles USB Core Events
 */

__task void USB_Core (void) {
#if (USB_CONFIGURE_EVENT || USB_INTERFACE_EVENT || USB_FEATURE_EVENT)
  U16 evt;
#endif

  for (;;) {

    os_evt_wait_or(0xFFFF, 0xFFFF);         /* Wait for an Event */

#if (USB_CONFIGURE_EVENT || USB_INTERFACE_EVENT || USB_FEATURE_EVENT)
    evt = os_evt_get();                     /* Get Event Flags */
#endif

#if USB_CONFIGURE_EVENT
    if (evt & USB_EVT_SET_CFG) {
    }
#endif

#if USB_INTERFACE_EVENT
    if (evt & USB_EVT_SET_IF) {
    }
#endif

#if USB_FEATURE_EVENT
    if (evt & USB_EVT_SET_FEATURE) {
    }
    if (evt & USB_EVT_CLR_FEATURE) {
    }
#endif

  }
}


/*
 *  USB Task Initialization
 */

void USB_TaskInit (void) {

  USB_DevTask = os_tsk_create(USB_Device, 3);

#if (USB_EP_EVENT & (1 << 0))
  USB_EPTask[0]  = os_tsk_create(USB_EndPoint0,  2);
#endif
#if (USB_EP_EVENT & (1 << 1))
  USB_EPTask[1]  = os_tsk_create(USB_EndPoint1,  2);
#endif
#if (USB_EP_EVENT & (1 << 2))
  USB_EPTask[2]  = os_tsk_create(USB_EndPoint2,  2);
#endif
#if (USB_EP_EVENT & (1 << 3))
  USB_EPTask[3]  = os_tsk_create(USB_EndPoint3,  2);
#endif
#if (USB_EP_EVENT & (1 << 4))
  USB_EPTask[4]  = os_tsk_create(USB_EndPoint4,  2);
#endif
#if (USB_EP_EVENT & (1 << 5))
  USB_EPTask[5]  = os_tsk_create(USB_EndPoint5,  2);
#endif
#if (USB_EP_EVENT & (1 << 6))
  USB_EPTask[6]  = os_tsk_create(USB_EndPoint6,  2);
#endif
#if (USB_EP_EVENT & (1 << 7))
  USB_EPTask[7]  = os_tsk_create(USB_EndPoint7,  2);
#endif
#if (USB_EP_EVENT & (1 << 8))
  USB_EPTask[8]  = os_tsk_create(USB_EndPoint8,  2);
#endif
#if (USB_EP_EVENT & (1 << 9))
  USB_EPTask[9]  = os_tsk_create(USB_EndPoint9,  2);
#endif
#if (USB_EP_EVENT & (1 << 10))
  USB_EPTask[10] = os_tsk_create(USB_EndPoint10, 2);
#endif
#if (USB_EP_EVENT & (1 << 11))
  USB_EPTask[11] = os_tsk_create(USB_EndPoint11, 2);
#endif
#if (USB_EP_EVENT & (1 << 12))
  USB_EPTask[12] = os_tsk_create(USB_EndPoint12, 2);
#endif
#if (USB_EP_EVENT & (1 << 13))
  USB_EPTask[13] = os_tsk_create(USB_EndPoint13, 2);
#endif
#if (USB_EP_EVENT & (1 << 14))
  USB_EPTask[14] = os_tsk_create(USB_EndPoint14, 2);
#endif
#if (USB_EP_EVENT & (1 << 15))
  USB_EPTask[15] = os_tsk_create(USB_EndPoint15, 2);
#endif

  USB_CoreTask = os_tsk_create(USB_Core, 2);
}
