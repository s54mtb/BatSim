/*----------------------------------------------------------------------------
 * U S B  -  K e r n e l
 *----------------------------------------------------------------------------
 * Name:    usbhw.c
 * Purpose: USB Hardware Layer Module for Philips LPC23xx/24xx
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
 *----------------------------------------------------------------------------
 * History:
 *          V1.20 Added USB_ClearEPBuf
 *                Added Mutex 
 *          V1.00 Initial Version
 *----------------------------------------------------------------------------*/

#include <LPC214X.H>                        /* LPC214x definitions */

#include <RTL.h>

#include "usb.h"
#include "usbcfg.h"
#include "usbreg.h"
#include "usbhw.h"
#include "usbcore.h"
#include "usbuser.h"


#pragma diag_suppress 1441


static OS_MUT USB_HW_Mutex;
int USBERROR;


#define EP_MSK_CTRL 0x0001      /* Control Endpoint Logical Address Mask */
#define EP_MSK_BULK 0xC924      /* Bulk Endpoint Logical Address Mask */
#define EP_MSK_INT  0x4492      /* Interrupt Endpoint Logical Address Mask */
#define EP_MSK_ISO  0x1248      /* Isochronous Endpoint Logical Address Mask */


#if USB_DMA

#pragma arm section zidata = "USB_RAM"
U32 UDCA[USB_EP_NUM];                       /* UDCA in USB RAM */
U32 DD_NISO_Mem[4*DD_NISO_CNT];             /* Non-Iso DMA Descriptor Memory */
U32 DD_ISO_Mem [5*DD_ISO_CNT];              /* Iso DMA Descriptor Memory */
#pragma arm section zidata
U32 udca[USB_EP_NUM];                       /* UDCA saved values */

U32 DDMemMap[2];                            /* DMA Descriptor Memory Usage */

#endif


/*
 *  Get Endpoint Physical Address
 *    Parameters:      EPNum: Endpoint Number
 *                       EPNum.0..3: Address
 *                       EPNum.7:    Dir
 *    Return Value:    Endpoint Physical Address
 */

U32 EPAdr (U32 EPNum) {
  U32 val;

  val = (EPNum & 0x0F) << 1;
  if (EPNum & 0x80) {
    val += 1;
  }
  return (val);
}


/*
 *  Write Command
 *    Parameters:      cmd:   Command
 *    Return Value:    None
 */

void __swi(8) WrCmd (U32 cmd);
void __SWI_8        (U32 cmd) {

  DEV_INT_CLR = CCEMTY_INT;
  CMD_CODE = cmd;
  while ((DEV_INT_STAT & CCEMTY_INT) == 0);
}


/*
 *  Write Command Data
 *    Parameters:      cmd:   Command
 *                     val:   Data
 *    Return Value:    None
 */

void __swi(9) WrCmdDat (U32 cmd, U32 val);
void __SWI_9           (U32 cmd, U32 val) {

  DEV_INT_CLR = CCEMTY_INT;
  CMD_CODE = cmd;
  while ((DEV_INT_STAT & CCEMTY_INT) == 0);
  DEV_INT_CLR = CCEMTY_INT;
  CMD_CODE = val;
  while ((DEV_INT_STAT & CCEMTY_INT) == 0);
}


/*
 *  Write Command to Endpoint
 *    Parameters:      cmd:   Command
 *                     val:   Data
 *    Return Value:    None
 */

void __swi(10) WrCmdEP (U32 EPNum, U32 cmd);
void __SWI_10          (U32 EPNum, U32 cmd) {

int i=0;

  DEV_INT_CLR = CCEMTY_INT;
  CMD_CODE = CMD_SEL_EP(EPAdr(EPNum));
  while (((DEV_INT_STAT & CCEMTY_INT) == 0)  & (i<0x5000))
  {
    i++;
  }
  if (i==0x5000) USBERROR = 1;
  DEV_INT_CLR = CCEMTY_INT;
  CMD_CODE = cmd;
  if (i<0x5000)
  {
  	i = 0;
    while (((DEV_INT_STAT & CCEMTY_INT) == 0) & (i<0x5000)) i++;
    if (i==0x5000) USBERROR = 1;
  }
}


/*
 *  Read Command Data
 *    Parameters:      cmd:   Command
 *    Return Value:    Data Value
 */

U32 __swi(11) RdCmdDat (U32 cmd);
U32 __SWI_11           (U32 cmd) {

  DEV_INT_CLR = CCEMTY_INT | CDFULL_INT;
  CMD_CODE = cmd;
  while ((DEV_INT_STAT & CDFULL_INT) == 0);
  return (CMD_DATA);
}


/*
 *  USB Initialize Function
 *   Called by the User to initialize USB
 *    Return Value:    None
 */

void USB_Init (void) {

  os_mut_init(USB_HW_Mutex);

  USB_TaskInit();                           /* USB Task Initialization */

  PINSEL1 &= ~0xC000C000;
  PINSEL1 |=  0x80004000;                   /* Select Connect, VBUS */

  PCONP |= 0x80000000;                      /* Turn On USB PCLK */

  /* Configure 48MHz USB Clock;  FOsc = 12MHz, M = 4, P = 2 */
  PLL48CFG  = 0x23;                         /* M = 4, P = 2 */
  PLL48CON  = PLLCON_PLLE;                  /* PLL Enable */
  PLL48FEED = 0xAA;                         /* Feed Sequence 1 */
  PLL48FEED = 0x55;                         /* Feed Sequence 2 */

  while ((PLL48STAT & PLLSTAT_PLOCK) == 0); /* Wait for PLL Lock */

  PLL48CON  = PLLCON_PLLE | PLLCON_PLLC;    /* PLL Enable & Connect */
  PLL48FEED = 0xAA;                         /* Feed Sequence 1 */
  PLL48FEED = 0x55;                         /* Feed Sequence 2 */

  VICVectAddr0 = (U32)USB_ISR;              /* USB Interrupt -> Vector 0 */
  VICVectCntl0 = 0x20 | 22;                 /* USB Interrupt -> IRQ Slot 0 */
  VICIntEnable = 1 << 22;                   /* Enable USB Interrupt */

//  DEV_INT_EN = DEV_STAT_INT;              /* Enable Device Status Interrupt */

#if 1 /* Partial Manual Reset since Automatic Bus Reset is not working */
  USB_Reset();
  USB_SetAddress(0);
#endif
}


/*
 *  USB Connect Function
 *   Called by the User to Connect/Disconnect USB
 *    Parameters:      con:   Connect/Disconnect
 *    Return Value:    None
 */

void USB_Connect (BOOL con) {
  WrCmdDat(CMD_SET_DEV_STAT, DAT_WR_BYTE(con ? DEV_CON : 0));
}


/*
 *  USB Reset Function
 *   Called automatically on USB Reset
 *    Return Value:    None
 */

void USB_Reset (void) {
#if USB_DMA
  U32 n;
#endif

  EP_INDEX = 0;
  MAXPACKET_SIZE = USB_MAX_PACKET0;
  EP_INDEX = 1;
  MAXPACKET_SIZE = USB_MAX_PACKET0;
  while ((DEV_INT_STAT & EP_RLZED_INT) == 0);

  EP_INT_CLR  = 0xFFFFFFFF;
  EP_INT_EN   = 0xFFFFFFFF ^ USB_DMA_EP;
  DEV_INT_CLR = 0xFFFFFFFF;
  DEV_INT_EN  = DEV_STAT_INT    | EP_SLOW_INT    |
               (USB_SOF_EVENT   ? FRAME_INT : 0) |
               (USB_ERROR_EVENT ? ERR_INT   : 0);

#if USB_DMA
  UDCA_HEAD   = USB_RAM_ADR;
  DMA_REQ_CLR = 0xFFFFFFFF;
  EP_DMA_DIS  = 0xFFFFFFFF;
  EP_DMA_EN   = USB_DMA_EP;
  EOT_INT_CLR = 0xFFFFFFFF;
  NDD_REQ_INT_CLR = 0xFFFFFFFF;
  SYS_ERR_INT_CLR = 0xFFFFFFFF;
  DMA_INT_EN  = 0x00000007;
  DDMemMap[0] = 0x00000000;
  DDMemMap[1] = 0x00000000;
  for (n = 0; n < USB_EP_NUM; n++) {
    udca[n] = 0;
    UDCA[n] = 0;
  }
#endif
}


/*
 *  USB Suspend Function
 *   Called automatically on USB Suspend
 *    Return Value:    None
 */

void USB_Suspend (void) {
  /* Performed by Hardware */
}


/*
 *  USB Resume Function
 *   Called automatically on USB Resume
 *    Return Value:    None
 */

void USB_Resume (void) {
  /* Performed by Hardware */
}


/*
 *  USB Remote Wakeup Function
 *   Called automatically on USB Remote Wakeup
 *    Return Value:    None
 */

void USB_WakeUp (void) {

  if (USB_DeviceStatus & USB_GETSTATUS_REMOTE_WAKEUP) {
    WrCmdDat(CMD_SET_DEV_STAT, DAT_WR_BYTE(DEV_CON));
  }
}


/*
 *  USB Remote Wakeup Configuration Function
 *    Parameters:      cfg:   Enable/Disable
 *    Return Value:    None
 */

void USB_WakeUpCfg (BOOL cfg) {
  /* Not needed */
}


/*
 *  USB Set Address Function
 *    Parameters:      adr:   USB Address
 *    Return Value:    None
 */

void USB_SetAddress (U32 adr) {
  WrCmdDat(CMD_SET_ADDR, DAT_WR_BYTE(DEV_EN | adr)); /* Don't wait for next */
  WrCmdDat(CMD_SET_ADDR, DAT_WR_BYTE(DEV_EN | adr)); /*  Setup Status Phase */
}


/*
 *  USB Configure Function
 *    Parameters:      cfg:   Configure/Deconfigure
 *    Return Value:    None
 */

void USB_Configure (BOOL cfg) {

  WrCmdDat(CMD_CFG_DEV, DAT_WR_BYTE(cfg ? CONF_DVICE : 0));

  REALIZE_EP = 0x00000003;
  while ((DEV_INT_STAT & EP_RLZED_INT) == 0);
  DEV_INT_CLR = EP_RLZED_INT;
}


/*
 *  Configure USB Endpoint according to Descriptor
 *    Parameters:      pEPD:  Pointer to Endpoint Descriptor
 *    Return Value:    None
 */

void USB_ConfigEP (USB_ENDPOINT_DESCRIPTOR *pEPD) {
  U32 num;

  num = EPAdr(pEPD->bEndpointAddress);
  REALIZE_EP |= (1 << num);
  EP_INDEX = num;
  MAXPACKET_SIZE = pEPD->wMaxPacketSize;
  while ((DEV_INT_STAT & EP_RLZED_INT) == 0);
  DEV_INT_CLR = EP_RLZED_INT;
}


/*
 *  Set Direction for USB Control Endpoint
 *    Parameters:      dir:   Out (dir == 0), In (dir <> 0)
 *    Return Value:    None
 */

void USB_DirCtrlEP (U32 dir) {
  /* Not needed */
}


/*
 *  Enable USB Endpoint
 *    Parameters:      EPNum: Endpoint Number
 *                       EPNum.0..3: Address
 *                       EPNum.7:    Dir
 *    Return Value:    None
 */

void USB_EnableEP (U32 EPNum) {
  WrCmdDat(CMD_SET_EP_STAT(EPAdr(EPNum)), DAT_WR_BYTE(0));
}


/*
 *  Disable USB Endpoint
 *    Parameters:      EPNum: Endpoint Number
 *                       EPNum.0..3: Address
 *                       EPNum.7:    Dir
 *    Return Value:    None
 */

void USB_DisableEP (U32 EPNum) {
  WrCmdDat(CMD_SET_EP_STAT(EPAdr(EPNum)), DAT_WR_BYTE(EP_STAT_DA));
}


/*
 *  Reset USB Endpoint
 *    Parameters:      EPNum: Endpoint Number
 *                       EPNum.0..3: Address
 *                       EPNum.7:    Dir
 *    Return Value:    None
 */

void USB_ResetEP (U32 EPNum) {
  WrCmdDat(CMD_SET_EP_STAT(EPAdr(EPNum)), DAT_WR_BYTE(0));
}


/*
 *  Set Stall for USB Endpoint
 *    Parameters:      EPNum: Endpoint Number
 *                       EPNum.0..3: Address
 *                       EPNum.7:    Dir
 *    Return Value:    None
 */

void USB_SetStallEP (U32 EPNum) {
  WrCmdDat(CMD_SET_EP_STAT(EPAdr(EPNum)), DAT_WR_BYTE(EP_STAT_ST));
}


/*
 *  Clear Stall for USB Endpoint
 *    Parameters:      EPNum: Endpoint Number
 *                       EPNum.0..3: Address
 *                       EPNum.7:    Dir
 *    Return Value:    None
 */

void USB_ClrStallEP (U32 EPNum) {
  WrCmdDat(CMD_SET_EP_STAT(EPAdr(EPNum)), DAT_WR_BYTE(0));
}


/*
 *  Clear USB Endpoint Buffer
 *    Parameters:      EPNum: Endpoint Number
 *                       EPNum.0..3: Address
 *                       EPNum.7:    Dir
 *    Return Value:    None
 */

void USB_ClearEPBuf (U32 EPNum) {
  WrCmdEP(EPNum, CMD_CLR_BUF);
}


/*
 *  Read USB Endpoint Data
 *    Parameters:      EPNum: Endpoint Number
 *                       EPNum.0..3: Address
 *                       EPNum.7:    Dir
 *                     pData: Pointer to Data Buffer
 *    Return Value:    Number of bytes read
 */

U32 USB_ReadEP (U32 EPNum, U8 *pData) {
  U32 cnt, n;

  os_mut_wait(USB_HW_Mutex, 0xFFFF);

  USB_CTRL = ((EPNum & 0x0F) << 2) | CTRL_RD_EN;

  do {
    cnt = RX_PLENGTH;
  } while ((cnt & PKT_RDY) == 0);
  cnt &= PKT_LNGTH_MASK;

  for (n = 0; n < (cnt + 3) / 4; n++) {
    *((__packed U32 *)pData) = RX_DATA;
    pData += 4;
  }

  USB_CTRL = 0;

  if (((EP_MSK_ISO >> EPNum) & 1) == 0) {   /* Non-Isochronous Endpoint */
    WrCmdEP(EPNum, CMD_CLR_BUF);
  }

  os_mut_release(USB_HW_Mutex);

  return (cnt);
}


/*
 *  Write USB Endpoint Data
 *    Parameters:      EPNum: Endpoint Number
 *                       EPNum.0..3: Address
 *                       EPNum.7:    Dir
 *                     pData: Pointer to Data Buffer
 *                     cnt:   Number of bytes to write
 *    Return Value:    Number of bytes written
 */

U32 USB_WriteEP (U32 EPNum, U8 *pData, U32 cnt) {
  U32 n;

  os_mut_wait(USB_HW_Mutex, 0xFFFF);

  USB_CTRL = ((EPNum & 0x0F) << 2) | CTRL_WR_EN;

  TX_PLENGTH = cnt;

  for (n = 0; n < (cnt + 3) / 4; n++) {
    TX_DATA = *((__packed U32 *)pData);
    pData += 4;
  }

  USB_CTRL = 0;

  WrCmdEP(EPNum, CMD_VALID_BUF);

  os_mut_release(USB_HW_Mutex);

  return (cnt);
}


#if USB_DMA


/* DMA Descriptor Memory Layout */
const U32 DDAdr[2] = { DD_NISO_ADR, DD_ISO_ADR };
const U32 DDSz [2] = { 16,          20         };


/*
 *  Setup USB DMA Transfer for selected Endpoint
 *    Parameters:      EPNum: Endpoint Number
 *                     pDD: Pointer to DMA Descriptor
 *    Return Value:    TRUE - Success, FALSE - Error
 */

BOOL USB_DMA_Setup(U32 EPNum, USB_DMA_DESCRIPTOR *pDD) {
  U32 num, ptr, nxt, iso, n;

  iso = pDD->Cfg.Type.IsoEP;                /* Iso or Non-Iso Descriptor */
  num = EPAdr(EPNum);                       /* Endpoint's Physical Address */

  ptr = 0;                                  /* Current Descriptor */
  nxt = udca[num];                          /* Initial Descriptor */
  while (nxt) {                             /* Go through Descriptor List */
    ptr = nxt;                              /* Current Descriptor */
    if (!pDD->Cfg.Type.Link) {              /* Check for Linked Descriptors */
      n = (ptr - DDAdr[iso]) / DDSz[iso];   /* Descriptor Index */
      DDMemMap[iso] &= ~(1 << n);           /* Unmark Memory Usage */
    }
    nxt = *((U32 *)ptr);                    /* Next Descriptor */
  }

  for (n = 0; n < 32; n++) {                /* Search for available Memory */
    if ((DDMemMap[iso] & (1 << n)) == 0) {
      break;                                /* Memory found */
    }
  }
  if (n == 32) return (__FALSE);            /* Memory not available */

  DDMemMap[iso] |= 1 << n;                  /* Mark Memory Usage */
  nxt = DDAdr[iso] + n * DDSz[iso];         /* Next Descriptor */

  if (ptr && pDD->Cfg.Type.Link) {
    *((U32 *)(ptr + 0))  = nxt;             /* Link in new Descriptor */
    *((U32 *)(ptr + 4)) |= 0x00000004;      /* Next DD is Valid */
  } else {
    udca[num] = nxt;                        /* Save new Descriptor */
    UDCA[num] = nxt;                        /* Update UDCA in USB */
  }

  /* Fill in DMA Descriptor */
  *(((U32 *)nxt)++) =  0;                   /* Next DD Pointer */
  *(((U32 *)nxt)++) =  pDD->Cfg.Type.ATLE |
                       (pDD->Cfg.Type.IsoEP << 4) |
                       (pDD->MaxSize <<  5) |
                       (pDD->BufLen  << 16);
  *(((U32 *)nxt)++) =  pDD->BufAdr;
  *(((U32 *)nxt)++) =  pDD->Cfg.Type.LenPos << 8;
  if (iso) {
    *((U32 *)nxt) =  pDD->InfoAdr;
  }

  return (__TRUE); /* Success */
}


/*
 *  Enable USB DMA Endpoint
 *    Parameters:      EPNum: Endpoint Number
 *                       EPNum.0..3: Address
 *                       EPNum.7:    Dir
 *    Return Value:    None
 */

void USB_DMA_Enable (U32 EPNum) {
  EP_DMA_EN = 1 << EPAdr(EPNum);
}


/*
 *  Disable USB DMA Endpoint
 *    Parameters:      EPNum: Endpoint Number
 *                       EPNum.0..3: Address
 *                       EPNum.7:    Dir
 *    Return Value:    None
 */

void USB_DMA_Disable (U32 EPNum) {
  EP_DMA_DIS = 1 << EPAdr(EPNum);
}


/*
 *  Get USB DMA Endpoint Status
 *    Parameters:      EPNum: Endpoint Number
 *                       EPNum.0..3: Address
 *                       EPNum.7:    Dir
 *    Return Value:    DMA Status
 */

U32 USB_DMA_Status (U32 EPNum) {
  U32 ptr, val;

  ptr = UDCA[EPAdr(EPNum)];                 /* Current Descriptor */
  if (ptr == 0) return (USB_DMA_INVALID);

  val = *((U32 *)(ptr + 3*4));              /* Status Information */
  switch ((val >> 1) & 0x0F) {
    case 0x00:                              /* Not serviced */
      return (USB_DMA_IDLE);
    case 0x01:                              /* Being serviced */
      return (USB_DMA_BUSY);
    case 0x02:                              /* Normal Completition */
      return (USB_DMA_DONE);
    case 0x03:                              /* Data Under Run */
      return (USB_DMA_UNDER_RUN);
    case 0x08:                              /* Data Over Run */
      return (USB_DMA_OVER_RUN);
    case 0x09:                              /* System Error */
      return (USB_DMA_ERROR);
  }

  return (USB_DMA_UNKNOWN);
}


/*
 *  Get USB DMA Endpoint Current Buffer Address
 *    Parameters:      EPNum: Endpoint Number
 *                       EPNum.0..3: Address
 *                       EPNum.7:    Dir
 *    Return Value:    DMA Address (or -1 when DMA is Invalid)
 */

U32 USB_DMA_BufAdr (U32 EPNum) {
  U32 ptr, val;

  ptr = UDCA[EPAdr(EPNum)];                 /* Current Descriptor */
  if (ptr == 0) return ((U32)(-1));         /* DMA Invalid */

  val = *((U32 *)(ptr + 2*4));              /* Buffer Address */

  return (val);                             /* Current Address */
}


/*
 *  Get USB DMA Endpoint Current Buffer Count
 *   Number of transfered Bytes or Iso Packets
 *    Parameters:      EPNum: Endpoint Number
 *                       EPNum.0..3: Address
 *                       EPNum.7:    Dir
 *    Return Value:    DMA Count (or -1 when DMA is Invalid)
 */

U32 USB_DMA_BufCnt (U32 EPNum) {
  U32 ptr, val;

  ptr = UDCA[EPAdr(EPNum)];                 /* Current Descriptor */
  if (ptr == 0) return ((U32)(-1));         /* DMA Invalid */

  val = *((U32 *)(ptr + 3*4));              /* Status Information */

  return (val >> 16);                       /* Current Count */
}


#endif /* USB_DMA */


/*
 *  Get USB Last Frame Number
 *    Parameters:      None
 *    Return Value:    Frame Number
 */

U32 USB_GetFrame (void) {
  U32 val;

  WrCmd(CMD_RD_FRAME);
  val = RdCmdDat(DAT_RD_FRAME);
  val = val | (RdCmdDat(DAT_RD_FRAME) << 8);

  return (val);
}


U32 LastError;                              /* Last Error */

/*
 *  Get USB Last Error Code
 *    Parameters:      None
 *    Return Value:    Error Code
 */

U32 USB_GetError (void) {
  return (LastError);
}


/*
 *  USB Interrupt Service Routine
 */

void USB_ISR (void) __irq {
  U32 disr, val, n, m;
  U32 episr, episrCur;
  int i = 0;

  disr = DEV_INT_STAT;                      /* Device Interrupt Status */

  /* Device Status Interrupt (Reset, Connect change, Suspend/Resume) */
  if (disr & DEV_STAT_INT) {
    DEV_INT_CLR = DEV_STAT_INT;
    WrCmd(CMD_GET_DEV_STAT);
    val = RdCmdDat(DAT_GET_DEV_STAT);       /* Device Status */
    if (val & DEV_RST) {                    /* Reset */
      USB_Reset();
#if   USB_RESET_EVENT
      isr_evt_set(USB_EVT_RESET, USB_DevTask);
#endif
    }
    if (val & DEV_CON_CH) {                 /* Connect change */
#if   USB_POWER_EVENT
      if (val & DEV_CON) {
        isr_evt_set(USB_EVT_POWER_ON,  USB_DevTask);
      } else {
        isr_evt_set(USB_EVT_POWER_OFF, USB_DevTask);
      }
#endif
    }
    if (val & DEV_SUS_CH) {                 /* Suspend/Resume */
      if (val & DEV_SUS) {                  /* Suspend */
        USB_Suspend();
#if     USB_SUSPEND_EVENT
        isr_evt_set(USB_EVT_SUSPEND, USB_DevTask);
#endif
      } else {                              /* Resume */
        USB_Resume();
#if     USB_RESUME_EVENT
        isr_evt_set(USB_EVT_RESUME,  USB_DevTask);
#endif
      }
    }
    goto isr_end;
  }

#if USB_SOF_EVENT
  /* Start of Frame Interrupt */
  if (disr & FRAME_INT) {
    DEV_INT_CLR = FRAME_INT;
    isr_evt_set(USB_EVT_SOF, USB_DevTask);
  }
#endif

#if USB_ERROR_EVENT
  /* Error Interrupt */
  if (disr & ERR_INT) {
    DEV_INT_CLR = ERR_INT;
    WrCmd(CMD_RD_ERR_STAT);
    LastError = RdCmdDat(DAT_RD_ERR_STAT);
    isr_evt_set(USB_EVT_ERROR, USB_DevTask);
  }
#endif

  /* Endpoint's Slow Interrupt */
  if (disr & EP_SLOW_INT) {
    episrCur = 0;
    episr    = EP_INT_STAT;
    for (n = 0; n < USB_EP_NUM; n++) {      /* Check All Endpoints */
      if (episr == episrCur) break;         /* break if all EP interrupts handled */
      if (episr & (1 << n)) {
        episrCur |= (1 << n);
        m = n >> 1;
  
        EP_INT_CLR = (1 << n);
		i = 0;
        while (((DEV_INT_STAT & CDFULL_INT) == 0) & (i<0x5000)) i++;
        val = CMD_DATA;
  
        if ((n & 1) == 0) {                 /* OUT Endpoint */
          if (n == 0) {                     /* Control OUT Endpoint */
            if (val & EP_SEL_STP) {         /* Setup Packet */
              isr_evt_set(USB_EVT_SETUP, USB_EPTask[0]);
              continue;
            }
          }
          isr_evt_set(USB_EVT_OUT, USB_EPTask[m]);
        } else {                            /* IN Endpoint */
          isr_evt_set(USB_EVT_IN,  USB_EPTask[m]);
        }
      }
    }
    DEV_INT_CLR = EP_SLOW_INT;
  }

#if USB_DMA
  if (DMA_INT_STAT & 0x00000001) {          /* End of Transfer Interrupt */
    val = EOT_INT_STAT;
    for (n = 2; n < USB_EP_NUM; n++) {      /* Check All Endpoints */
      if (val & (1 << n)) {
        m = n >> 1;
        if ((n & 1) == 0) {                 /* OUT Endpoint */
          isr_evt_set(USB_EVT_OUT_DMA_EOT, USB_EPTask[m]);
        } else {                            /* IN Endpoint */
          isr_evt_set(USB_EVT_IN_DMA_EOT,  USB_EPTask[m]);
        }
      }
    }
    EOT_INT_CLR = val;
  }

  if (DMA_INT_STAT & 0x00000002) {          /* New DD Request Interrupt */
    val = NDD_REQ_INT_STAT;
    for (n = 2; n < USB_EP_NUM; n++) {      /* Check All Endpoints */
      if (val & (1 << n)) {
        m = n >> 1;
        if ((n & 1) == 0) {                 /* OUT Endpoint */
          isr_evt_set(USB_EVT_OUT_DMA_NDR, USB_EPTask[m]);
        } else {                            /* IN Endpoint */
          isr_evt_set(USB_EVT_IN_DMA_NDR,  USB_EPTask[m]);
        }
      }
    }
    NDD_REQ_INT_CLR = val;
  }

  if (DMA_INT_STAT & 0x00000004) {          /* System Error Interrupt */
    val = SYS_ERR_INT_STAT;
    for (n = 2; n < USB_EP_NUM; n++) {      /* Check All Endpoints */
      if (val & (1 << n)) {
        m = n >> 1;
        if ((n & 1) == 0) {                 /* OUT Endpoint */
          isr_evt_set(USB_EVT_OUT_DMA_ERR, USB_EPTask[m]);
        } else {                            /* IN Endpoint */
          isr_evt_set(USB_EVT_IN_DMA_ERR,  USB_EPTask[m]);
        }
      }
    }
    SYS_ERR_INT_CLR = val;
  }
#endif /* USB_DMA */

isr_end:
  VICVectAddr = 0;                          /* Acknowledge Interrupt */
}
