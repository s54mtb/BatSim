/*----------------------------------------------------------------------------
 *      R T L  -  K e r n e l
 *----------------------------------------------------------------------------
 *      Name:    RTX_CONFIG.C
 *      Purpose: Configuration of RTX Kernel for Philips LPC21xx
 *      Rev.:    V3.40
 *----------------------------------------------------------------------------
 *      This code is part of the RealView Run-Time Library.
 *      Copyright (c) 2004-2008 KEIL - An ARM Company. All rights reserved.
 *---------------------------------------------------------------------------*/

#include <RTL.h>
#include <LPC21xx.H>                     /* LPC21xx definitions              */

/*----------------------------------------------------------------------------
 *      RTX User configuration part BEGIN
 *---------------------------------------------------------------------------*/

//-------- <<< Use Configuration Wizard in Context Menu >>> -----------------
//
// <h>Task Definitions
// ===================
//
//   <o>Number of concurrent running tasks <0-250>
//   <i> Define max. number of tasks that will run at the same time.
//   <i> Default: 6
#ifndef OS_TASKCNT
 #define OS_TASKCNT     8
#endif

//   <o>Number of tasks with user-provided stack <0-250>
//   <i> Define the number of tasks that will use a bigger stack.
//   <i> The memory space for the stack is provided by the user.
//   <i> Default: 0
#ifndef OS_PRIVCNT
 #define OS_PRIVCNT     0
#endif

//   <o>Task stack size [bytes] <20-4096:8><#/4>
//   <i> Set the stack size for tasks which is assigned by the system.
//   <i> Default: 200
#ifndef OS_STKSIZE
 #define OS_STKSIZE     300
#endif

// <q>Check for the stack overflow
// ===============================
// <i> Include the stack checking code for a stack overflow.
// <i> Note that additional code reduces the Kernel performance.
#ifndef OS_STKCHECK
 #define OS_STKCHECK    1
#endif

//   <o>Number of user timers <0-250>
//   <i> Define max. number of user timers that will run at the same time.
//   <i> Default: 0  (User timers disabled)
#ifndef OS_TIMERCNT
 #define OS_TIMERCNT    1
#endif

// </h>
// <h>System Timer Configuration
// =============================
//   <o>RTX Kernel timer <0=> Timer 0 <1=> Timer 1
//   <i> Define the ARM timer used as a system tick timer.
//   <i> Default: Timer 0
#ifndef OS_TIMER
 #define OS_TIMER       0
#endif

//   <o>Timer clock value [Hz] <1-1000000000>
//   <i> Set the timer clock value for selected timer.
//   <i> Default: 15000000  (15MHz at 60MHz CCLK and VPBDIV = 4)
#ifndef OS_CLOCK
 #define OS_CLOCK       30000000
#endif

//   <o>Timer tick value [us] <1-1000000>
//   <i> Set the timer tick value for selected timer.
//   <i> Default: 10000  (10ms)
#ifndef OS_TICK
 #define OS_TICK        1000
#endif

// </h>
// <e>Round-Robin Task switching
// =============================
// <i> Enable Round-Robin Task switching.
#ifndef OS_ROBIN
 #define OS_ROBIN       1
#endif

//   <o>Round-Robin Timeout [ticks] <1-1000>
//   <i> Define how long a task will execute before a task switch.
//   <i> Default: 5
#ifndef OS_ROBINTOUT
 #define OS_ROBINTOUT   5
#endif

// </e>

//------------- <<< end of configuration section >>> -----------------------

// Standard library system mutexes
// ===============================
//  Define max. number system mutexes that are used to protect 
//  the arm standard runtime library. For microlib they are not used.
#ifndef OS_MUTEXCNT
 #define OS_MUTEXCNT    8
#endif

/*----------------------------------------------------------------------------
 *      RTX User configuration part END
 *---------------------------------------------------------------------------*/

#if   (OS_TIMER == 0)                                   /* Timer 0          */
 #define OS_TID_        4                               /*  Timer ID        */
 #define TIMx(reg)      T0##reg
#elif (OS_TIMER == 1)                                   /* Timer 1          */
 #define OS_TID_        5                               /*  Timer ID        */
 #define TIMx(reg)      T1##reg
#else
 #error OS_TIMER invalid
#endif

#define OS_TIM_         (1 << OS_TID_)                  /*  Interrupt Mask  */
#define OS_TRV          ((U32)(((double)OS_CLOCK*(double)OS_TICK)/1E6)-1)
#define OS_TVAL         TIMx(TC)                        /*  Timer Value     */
#define OS_TOVF         (TIMx(IR) & 1)                  /*  Overflow Flag   */
#define OS_TFIRQ()      VICSoftInt    = OS_TIM_;        /*  Force Interrupt */
#define OS_TIACK()      TIMx(IR) = 1;                   /*  Interrupt Ack   */ \
                        VICSoftIntClr = OS_TIM_;                               \
                        VICVectAddr   = 0;
#define OS_TINIT()      TIMx(MR0) = OS_TRV;             /*  Initialization  */ \
                        TIMx(MCR) = 3;                                         \
                        TIMx(TCR) = 1;                                         \
                        VICDefVectAddr = (U32)os_def_interrupt;                \
                        VICVectAddr15  = (U32)os_clock_interrupt;              \
                        VICVectCntl15  = 0x20 | OS_TID_;

#define OS_IACK()       VICVectAddr   = 0;              /* Interrupt Ack    */

#define OS_LOCK()       VICIntEnClr   = OS_TIM_;        /* Task Lock        */
#define OS_UNLOCK()     VICIntEnable  = OS_TIM_;        /* Task Unlock      */

/* WARNING: Using IDLE mode might cause you troubles while debugging. */
#define _idle_()        PCON = 1;


/*----------------------------------------------------------------------------
 *      Global Functions
 *---------------------------------------------------------------------------*/

/*--------------------------- os_idle_demon ---------------------------------*/

__task void os_idle_demon (void) {
   /* The idle demon is a system task, running when no other task is ready */
   /* to run. The 'os_xxx' function calls are not allowed from this task.  */

   for (;;) {
   /* HERE: include optional user code to be executed when no task runs.*/
   }
}


/*--------------------------- os_tmr_call -----------------------------------*/

void os_tmr_call (U16 info) {
   /* This function is called when the user timer has expired. Parameter   */
   /* 'info' holds the value, defined when the timer was created.          */
   if (info == 0x1234) IOCLR0 = 0x00000001;	// clear pulse output
   /* HERE: include optional user code to be executed on timeout. */
}


/*--------------------------- os_stk_overflow -------------------------------*/

void os_stk_overflow (OS_TID task_id) {
   /* This function is called when a stack overflow is detected. Parameter */
   /* 'task_id' holds the id of this task. You can use 'RTX Kernel' dialog,*/
   /* page 'Active Tasks' to check, which task needs a bigger stack.       */

   /* HERE: include optional code to be executed on stack overflow. */
   for (;;);
}


/*----------------------------------------------------------------------------
 *      RTX Configuration Functions
 *---------------------------------------------------------------------------*/

static void os_def_interrupt (void) __irq  {
   /* Default Interrupt Function: may be called when timer ISR is disabled */
   OS_IACK();
}

#include <RTX_lib.c>

/*----------------------------------------------------------------------------
 * end of file
 *---------------------------------------------------------------------------*/

