/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2014 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

// ----------------------------------------------------------------------------

#include "cortexm/ExceptionHandlers.h"
#include "cmsis_device.h"
#include "arm/semihosting.h"
#include "diag/Trace.h"
#include <string.h>
//#include "nt_i2c.h" //For i2c handler
#include "nt_common.h" // For register read
#include "ExceptionHandlers.h"
#ifdef PLATFORM_FERMION
#include "Fermion_seq_hwioreg.h"
#endif //PLATFORM_FERMION
#include "fermion_hw_reg.h"
#include "err.h"
#include "errlog.h"
// ----------------------------------------------------------------------------

#define WDOG_INTR_PENDING_BITMASK 0x400000
#ifdef CONFIG_WIFI_FW_COREDUMP_SUPPORT
extern coredump_type * coredump;
#endif

extern void
__attribute__((noreturn,weak))
_start (void);

// ----------------------------------------------------------------------------
// Default exception handlers. Override the ones here by defining your own
// handler routines in your application code.
// ----------------------------------------------------------------------------

#if defined(DEBUG)

// The DEBUG version is not naked, but has a proper stack frame,
// to allow setting breakpoints at Reset_Handler.
void __attribute__ ((section(".after_vectors"),noreturn))
Reset_Handler (void)
{
  _start ();
}

#else

// The Release version is optimised to a quick branch to _start.
void __attribute__ ((section(".after_vectors"),naked))
Reset_Handler(void)
  {
    asm volatile
    (
        " ldr     r0,=_start \n"
        " bx      r0"
        :
        :
        :
    );
  }

#endif

char g_assert_file_func_line[600];
void assert_handler(const char *  file,const char* func, const uint32_t line)
{
#ifdef CONFIG_WIFI_FW_COREDUMP_SUPPORT
  ERR_FATAL_ASSERTION("Assertion Detected", line, file)
#else
	char *intr_str = g_assert_file_func_line;
	snprintf(intr_str,600,"Assert @%s,ln %d in %s\r\n", func,line,file);
	UART_Send_direct(intr_str,strlen(intr_str));
#endif
}

void __attribute__ ((section(".after_ram_vectors"),weak))
NMI_Handler (void)
{
#ifdef CONFIG_WIFI_FW_COREDUMP_SUPPORT
  ERR_FATAL_EXCEPTION("WDOG Bark", __LINE__, __FILE__);
#else
  nt_system_sw_reset();
#endif
#if (FERMION_CHIP_VERSION == 2)
		/*This is a software workaround for the AON WDT issue in 2.0, after a WDT bite,
		the control goes to NMI handler, after entering reset vector. So in the
		NMI handler, we de-assert the last bark signal and return
		*/
		//Clearing the WDOG bark interrupt
		uint32_t control_reg, wdog_status;
		//Disable NMI
	
		//DEBUG_MODE_PRINTF("NMI\r\n");
	
		HWIO_OUTXF(SEQ_WCSS_CCU_OFFSET, CCU_CCU_R_CCU_MISC_CTL, CCU_CCPU_NMI_EN, 0);
		//Clearing watchdog interrupt in NVIC
		NVIC->ICPR[1] = NVIC->ICPR[1]& WDOG_INTR_PENDING_BITMASK;
		//Sampling WDOG status to print
		wdog_status = NT_REG_RD(QWLAN_PMU_WDOG_STS_REG); 
	  
		control_reg = NT_REG_RD(QWLAN_PMU_AON_WDOG_CTL_REG);		 //AON - read the watchdog control reg.
		control_reg |= QWLAN_PMU_AON_WDOG_CTL_WDOG_RESET_MASK;
		NT_REG_WR(QWLAN_PMU_AON_WDOG_CTL_REG, control_reg); 		 //AON - wirte 1 to AON dog ctl reg by using AON wdog reset mask field.
	  
		control_reg &= (uint32_t)(~(QWLAN_PMU_AON_WDOG_CTL_WDOG_RESET_MASK));
		NT_REG_WR(QWLAN_PMU_AON_WDOG_CTL_REG,control_reg);			 // write 0 to AON wdog ctl reg by using wdog reset mask field
	  
		volatile uint32_t count = NT_REG_RD(QWLAN_PMU_AON_WDOG_COUNT_REG);
		(void )wdog_status;
		while(0 != count)
		{
			count = NT_REG_RD(QWLAN_PMU_AON_WDOG_COUNT_REG);
		}
		//Disable AON-WDOG
		//control_reg = NT_REG_RD(QWLAN_PMU_AON_WDOG_CTL_REG);					   
		//control_reg &=	(uint32_t)(~( QWLAN_PMU_AON_WDOG_CTL_WDOG_ENABLE_MASK));
		//NT_REG_WR(QWLAN_PMU_AON_WDOG_CTL_REG,control_reg);

		return;
#else

#ifdef PLATFORM_FERMION
    uint32_t exception_num = HWIO_INXF(SEQ_WCSS_CCU_OFFSET,CCU_CCU_R_CCU_MISC_CTL,CCU_CCPU_NMI_SEL) ;
    if(exception_num == WCSS_wdog_bark)
    {
        nt_clear_device_irq(WCSS_wdog_bark);
        nt_wdt_int_wcss_wdog_bark();
    }
else
#endif // PLATFORM_FERMION
    {
#if defined(DEBUG)
          __DEBUG_BKPT();
#endif
          while (1)
            {
            }
    }

#endif // CONFIG_WIFI_FW_COREDUMP_SUPPORT
}

// ----------------------------------------------------------------------------

#if defined(TRACE)

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

// The values of BFAR and MMFAR stay unchanged if the BFARVALID or
// MMARVALID is set. However, if a new fault occurs during the
// execution of this fault handler, the value of the BFAR and MMFAR
// could potentially be erased. In order to ensure the fault addresses
// accessed are valid, the following procedure should be used:
// 1. Read BFAR/MMFAR.
// 2. Read CFSR to get BFARVALID or MMARVALID. If the value is 0, the
//    value of BFAR or MMFAR accessed can be invalid and can be discarded.
// 3. Optionally clear BFARVALID or MMARVALID.
// (See Joseph Yiu's book).

void
dumpExceptionStack (ExceptionStackFrame* frame,
                uint32_t cfsr, uint32_t mmfar, uint32_t bfar,
                                        uint32_t lr)
{
  trace_printf ("Stack frame:\n");
  trace_printf (" R0 =  %08X\n", frame->r0);
  trace_printf (" R1 =  %08X\n", frame->r1);
  trace_printf (" R2 =  %08X\n", frame->r2);
  trace_printf (" R3 =  %08X\n", frame->r3);
  trace_printf (" R12 = %08X\n", frame->r12);
  trace_printf (" LR =  %08X\n", frame->lr);
  trace_printf (" PC =  %08X\n", frame->pc);
  trace_printf (" PSR = %08X\n", frame->psr);
  trace_printf ("FSR/FAR:\n");
  trace_printf (" CFSR =  %08X\n", cfsr);
  trace_printf (" HFSR =  %08X\n", SCB->HFSR);
  trace_printf (" DFSR =  %08X\n", SCB->DFSR);
  trace_printf (" AFSR =  %08X\n", SCB->AFSR);

  if (cfsr & (1UL << 7))
    {
      trace_printf (" MMFAR = %08X\n", mmfar);
    }
  if (cfsr & (1UL << 15))
    {
      trace_printf (" BFAR =  %08X\n", bfar);
    }
  trace_printf ("Misc\n");
  trace_printf (" LR/EXC_RETURN= %08X\n", lr);
}

#endif // defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

#if defined(__ARM_ARCH_6M__)

void
dumpExceptionStack (ExceptionStackFrame* frame, uint32_t lr)
{
  trace_printf ("Stack frame:\n");
  trace_printf (" R0 =  %08X\n", frame->r0);
  trace_printf (" R1 =  %08X\n", frame->r1);
  trace_printf (" R2 =  %08X\n", frame->r2);
  trace_printf (" R3 =  %08X\n", frame->r3);
  trace_printf (" R12 = %08X\n", frame->r12);
  trace_printf (" LR =  %08X\n", frame->lr);
  trace_printf (" PC =  %08X\n", frame->pc);
  trace_printf (" PSR = %08X\n", frame->psr);
  trace_printf ("Misc\n");
  trace_printf (" LR/EXC_RETURN= %08X\n", lr);
}

#endif // defined(__ARM_ARCH_6M__)

#endif // defined(TRACE)

// ----------------------------------------------------------------------------

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

#if defined(OS_USE_SEMIHOSTING) || defined(OS_USE_TRACE_SEMIHOSTING_STDOUT) || defined(OS_USE_TRACE_SEMIHOSTING_DEBUG)

int
isSemihosting (ExceptionStackFrame* frame, uint16_t opCode);

/**
 * This function provides the minimum functionality to make a semihosting program execute even without the debugger present.
 * @param frame pointer to an exception stack frame.
 * @param opCode the 16-bin word of the BKPT instruction.
 * @return 1 if the instruction was a valid semihosting call; 0 otherwise.
 */
int
isSemihosting (ExceptionStackFrame* frame, uint16_t opCode)
{
  uint16_t* pw = (uint16_t*) frame->pc;
  if (*pw == opCode)
    {
      uint32_t r0 = frame->r0;
#if defined(OS_DEBUG_SEMIHOSTING_FAULTS) || defined(OS_USE_SEMIHOSTING) || defined(OS_USE_TRACE_SEMIHOSTING_STDOUT)
      uint32_t r1 = frame->r1;
#endif
#if defined(OS_USE_SEMIHOSTING) || defined(OS_USE_TRACE_SEMIHOSTING_STDOUT)
      uint32_t* blk = (uint32_t*) r1;
#endif

#if defined(OS_DEBUG_SEMIHOSTING_FAULTS)
      // trace_printf ("sh r0=%d\n", r0);
#endif

      switch (r0)
        {

#if defined(OS_USE_SEMIHOSTING)

        case SEMIHOSTING_SYS_CLOCK:
        case SEMIHOSTING_SYS_ELAPSED:
        case SEMIHOSTING_SYS_FLEN:
        case SEMIHOSTING_SYS_GET_CMDLINE:
        case SEMIHOSTING_SYS_REMOVE:
        case SEMIHOSTING_SYS_RENAME:
        case SEMIHOSTING_SYS_SEEK:
        case SEMIHOSTING_SYS_SYSTEM:
        case SEMIHOSTING_SYS_TICKFREQ:
        case SEMIHOSTING_SYS_TMPNAM:
        case SEMIHOSTING_SYS_ISTTY:
          frame->r0 = (uint32_t)-1; // the call is not successful or not supported
          break;

        case SEMIHOSTING_SYS_CLOSE:
          frame->r0 = 0; // call is successful
          break;

        case SEMIHOSTING_SYS_ERRNO:
          frame->r0 = 0; // the value of the C library errno variable.
          break;

        case SEMIHOSTING_SYS_HEAPINFO:
          blk[0] = 0; // heap_base
          blk[1] = 0; // heap_limit
          blk[2] = 0; // stack_base
          blk[3] = 0; // stack_limit
          break;

        case SEMIHOSTING_SYS_ISERROR:
          frame->r0 = 0; // 0 if the status word is not an error indication
          break;

        case SEMIHOSTING_SYS_READ:
          // If R0 contains the same value as word 3, the call has
          // failed and EOF is assumed.
          frame->r0 = blk[2];
          break;

        case SEMIHOSTING_SYS_READC:
          frame->r0 = '\0'; // the byte read from the console.
          break;

        case SEMIHOSTING_SYS_TIME:
          frame->r0 = 0; // the number of seconds since 00:00 January 1, 1970.
          break;

        case SEMIHOSTING_ReportException:

          NVIC_SystemReset ();
          // Should not reach here
          return 0;

#endif // defined(OS_USE_SEMIHOSTING)

#if defined(OS_USE_SEMIHOSTING) || defined(OS_USE_TRACE_SEMIHOSTING_STDOUT)

#define HANDLER_STDIN   (1)
#define HANDLER_STDOUT  (2)
#define HANDLER_STDERR  (3)

        case SEMIHOSTING_SYS_OPEN:
          // Process only standard io/out/err and return 1/2/3
          if (strcmp ((char*) blk[0], ":tt") == 0)
            {
              if ((blk[1] == 0))
                {
                  frame->r0 = HANDLER_STDIN;
                  break;
                }
              else if (blk[1] == 4)
                {
                  frame->r0 = HANDLER_STDOUT;
                  break;
                }
              else if (blk[1] == 8)
                {
                  frame->r0 = HANDLER_STDERR;
                  break;
                }
            }
          frame->r0 = (uint32_t)-1; // the call is not successful or not supported
          break;

        case SEMIHOSTING_SYS_WRITE:
          // Silently ignore writes to stdout/stderr, fail on all other handler.
          if ((blk[0] == HANDLER_STDOUT) || (blk[0] == HANDLER_STDERR))
            {
#if defined(OS_DEBUG_SEMIHOSTING_FAULTS)
              frame->r0 = (uint32_t) blk[2]
                  - trace_write ((char*) blk[1], blk[2]);
#else
              frame->r0 = 0; // all sent, no more.
#endif // defined(OS_DEBUG_SEMIHOSTING_FAULTS)
            }
          else
            {
              // If other handler, return the total number of bytes
              // as the number of bytes that are not written.
              frame->r0 = blk[2];
            }
          break;

#endif // defined(OS_USE_SEMIHOSTING) || defined(OS_USE_TRACE_SEMIHOSTING_STDOUT)

#if defined(OS_USE_SEMIHOSTING) || defined(OS_USE_TRACE_SEMIHOSTING_STDOUT) || defined(OS_USE_TRACE_SEMIHOSTING_DEBUG)

        case SEMIHOSTING_SYS_WRITEC:
#if defined(OS_DEBUG_SEMIHOSTING_FAULTS)
          {
            char ch = *((char*) r1);
            trace_write (&ch, 1);
          }
#endif
          // Register R0 is corrupted.
          break;

        case SEMIHOSTING_SYS_WRITE0:
#if defined(OS_DEBUG_SEMIHOSTING_FAULTS)
          {
            char* p = ((char*) r1);
            trace_write (p, strlen (p));
          }
#endif
          // Register R0 is corrupted.
          break;

#endif

        default:
          return 0;
        }

      // Alter the PC to make the exception returns to
      // the instruction after the faulty BKPT.
      frame->pc += 2;
      return 1;
    }
  return 0;
}

#endif

// Hard Fault handler wrapper in assembly.
// It extracts the location of stack frame and passes it to handler
// in C as a pointer. We also pass the LR value as second
// parameter.
// (Based on Joseph Yiu's, The Definitive Guide to ARM Cortex-M3 and
// Cortex-M4 Processors, Third Edition, Chap. 12.8, page 402).

void __attribute__ ((section(".after_ram_vectors"),weak,naked))
HardFault_Handler (void)
{
  asm volatile(
      " tst lr,#4       \n"
      " ite eq          \n"
      " mrseq r0,msp    \n"
      " mrsne r0,psp    \n"
      " mov r1,lr       \n"
      " ldr r2,=HardFault_Handler_C \n"
      " bx r2"

      : /* Outputs */
      : /* Inputs */
      : /* Clobbers */
  );
}

void __attribute__ ((section(".after_ram_vectors"),weak,used))
HardFault_Handler_C (ExceptionStackFrame* frame ,
                     uint32_t lr __attribute__((unused)))
{
#ifdef CONFIG_WIFI_FW_COREDUMP_SUPPORT
  ERR_FATAL_EXCEPTION("Exception Detected: Hard Fault", __LINE__, __FILE__);
#else
	int str_len;

#if defined(OS_USE_SEMIHOSTING) || defined(OS_USE_TRACE_SEMIHOSTING_STDOUT) || defined(OS_USE_TRACE_SEMIHOSTING_DEBUG)

  // If the BKPT instruction is executed with C_DEBUGEN == 0 and MON_EN == 0,
  // it will cause the processor to enter a HardFault exception, with DEBUGEVT
  // in the Hard Fault Status register (HFSR) set to 1, and BKPT in the
  // Debug Fault Status register (DFSR) also set to 1.

  if (((SCB->DFSR & SCB_DFSR_BKPT_Msk) != 0)
      && ((SCB->HFSR & SCB_HFSR_DEBUGEVT_Msk) != 0))
    {
      if (isSemihosting (frame, 0xBE00 + (AngelSWI & 0xFF)))
        {
          // Clear the exception cause in exception status.
          SCB->HFSR = SCB_HFSR_DEBUGEVT_Msk;

          // Continue after the BKPT
          return;
        }
    }

#endif

#if defined(TRACE)
  trace_printf ("[HardFault]\n");
  dumpExceptionStack (frame, cfsr, mmfar, bfar, lr);
#endif // defined(TRACE)

	static char interrupt_string[200] = {0};
	str_len =  snprintf((char *)interrupt_string,
			sizeof(interrupt_string),
			"HARDFAULT HANDLER :\r\n"
			"R0    %08X   R1    %08X\r\n"
			"R2    %08X   R3    %08X\r\n"
			"R12   %08X   LR    %08X\r\n"
			"PC    %08X   PSR   %08X\r\n",
			frame->r0 ,frame->r1 ,
			frame->r2 ,frame->r3 ,
			frame->r12, frame->lr ,
			frame->pc ,frame->psr );
	if( str_len > sizeof(interrupt_string) )
	{
		str_len = sizeof(interrupt_string);
	}
	UART_Send_direct(interrupt_string , str_len);
	memset(interrupt_string,0,sizeof(interrupt_string));

	str_len = snprintf( (char *)interrupt_string,
			sizeof(interrupt_string),
			"ICSR  %08X   VTOR  %08X\r\n"
			"AIRCR %08X   SCR   %08X \r\n"
			"CCR   %08X\r\n" ,
			SCB->ICSR , SCB->VTOR ,
			SCB->AIRCR , SCB->SCR ,
			SCB->CCR );
	if( str_len > sizeof(interrupt_string) )
	{
		str_len = sizeof(interrupt_string);
	}
	UART_Send_direct(interrupt_string , str_len);
	memset(interrupt_string,0,sizeof(interrupt_string));

	str_len = snprintf( (char *)interrupt_string,
			sizeof(interrupt_string),
			"SHPR1 %02x%02x%02x%02x  SHPR2 %02x%02x%02x%02x \r\n"
			"SHPR3 %02x%02x%02x%02x\r\n",
			SCB->SHP[0] , SCB->SHP[1] , SCB->SHP[2] , SCB->SHP[3] ,
			SCB->SHP[4] , SCB->SHP[5] , SCB->SHP[6] , SCB->SHP[7] ,
			SCB->SHP[8] , SCB->SHP[9] , SCB->SHP[10] , SCB->SHP[11] );
	if( str_len > sizeof(interrupt_string) )
	{
		str_len = sizeof(interrupt_string);
	}
	UART_Send_direct(interrupt_string , str_len);
	memset(interrupt_string,0,sizeof(interrupt_string));

	str_len = snprintf( (char *)interrupt_string,
			sizeof(interrupt_string),
			"SHCSR %08X   CFSR  %08X \r\n"
			"HFSR  %08X   DFSR  %08X \r\n"
			"MMFAR %08X\r\n",
			SCB->SHCSR , SCB->CFSR ,
			SCB->HFSR , SCB->DFSR ,
			SCB->MMFAR );
	if( str_len > sizeof(interrupt_string) )
	{
		str_len = sizeof(interrupt_string);
	}
	UART_Send_direct(interrupt_string , str_len);
	memset(interrupt_string,0,sizeof(interrupt_string));

	str_len = snprintf( (char *)interrupt_string,
			sizeof(interrupt_string),
			"BFAR  %08X   AFSR %08X \r\n"
			"PFR0  %08X   PFR1 %08X \r\n"
			"DFR   %08X\r\n",
			SCB->BFAR , SCB->AFSR ,
			SCB->PFR[0] , SCB->PFR[1] ,
			SCB->DFR );
	if( str_len > sizeof(interrupt_string) )
	{
		str_len = sizeof(interrupt_string);
	}
	UART_Send_direct(interrupt_string , str_len);
	memset(interrupt_string,0,sizeof(interrupt_string));

	str_len = snprintf( (char *)interrupt_string,
			sizeof(interrupt_string),
			"ADR     %08X   MMFR[0] %08X \r\n"
			"MMFR[1] %08X   MMFR[2] %08X \r\n"
			"MMFR[3] %08X\r\n",
			SCB->ADR , SCB->MMFR[0] ,
			SCB->MMFR[1] , SCB->MMFR[2] ,
			SCB->MMFR[3] );
	if( str_len > sizeof(interrupt_string) )
	{
		str_len = sizeof(interrupt_string);
	}
	UART_Send_direct(interrupt_string , str_len);
	memset(interrupt_string,0,sizeof(interrupt_string));

	str_len = snprintf( (char *)interrupt_string,
			sizeof(interrupt_string),
			"ISAR[0] %08X  ISAR[1] %08X \r\n"
			"ISAR[2] %08X  ISAR[3] %08X \r\n"
			"ISAR[4] %08X  CPACR   %08X\r\n",
			SCB->ISAR[0] , SCB->ISAR[1] ,
			SCB->ISAR[2] , SCB->ISAR[3] ,
			SCB->ISAR[4] , SCB->CPACR);
	if( str_len > sizeof(interrupt_string) )
	{
		str_len = sizeof(interrupt_string);
	}
	UART_Send_direct(interrupt_string , str_len);
	memset(interrupt_string,0,sizeof(interrupt_string));

	str_len = snprintf((char *)interrupt_string,
			sizeof(interrupt_string),
			"NVIC_ISPR0 %08X   NVIC_ISPR1 %08X   NVIC_ISPR2 %08X\r\n",
			NVIC->ISPR[0] ,NVIC->ISPR[1] ,NVIC->ISPR[2] );
	if( str_len > sizeof(interrupt_string) )
	{
		str_len = sizeof(interrupt_string);
	}
	UART_Send_direct(interrupt_string , str_len);
	memset(interrupt_string,0,sizeof(interrupt_string));

	str_len = snprintf((char *)interrupt_string,
			sizeof(interrupt_string),
			"NVIC_ISER0 %08X   NVIC_ICER1 %08X   NVIC_ICER2 %08X\r\n",
			NVIC->ICER[0] , NVIC->ICER[1] , NVIC->ICER[2] );
	if( str_len > sizeof(interrupt_string) )
	{
		str_len = sizeof(interrupt_string);
	}
	UART_Send_direct(interrupt_string , str_len);
	memset(interrupt_string,0,sizeof(interrupt_string));

	str_len = snprintf((char *)interrupt_string,
			sizeof(interrupt_string),
			"NVIC_ISER0 %08X   NVIC_ISER1 %08X   NVIC_ISER2 %08X\r\n",
			NVIC->ISER[0] , NVIC->ISER[1] , NVIC->ISER[2] );
	if( str_len > sizeof(interrupt_string) )
	{
		str_len = sizeof(interrupt_string);
	}
	UART_Send_direct(interrupt_string , str_len);
	//memset(interrupt_string,0,sizeof(interrupt_string));

#if defined(DEBUG)
   __DEBUG_BKPT();
#endif
  while (1)
    {
    }
#endif
}

#endif // defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)


#if defined(__ARM_ARCH_6M__)

// Hard Fault handler wrapper in assembly.
// It extracts the location of stack frame and passes it to handler
// in C as a pointer. We also pass the LR value as second
// parameter.
// (Based on Joseph Yiu's, The Definitive Guide to ARM Cortex-M0
// First Edition, Chap. 12.8, page 402).

void __attribute__ ((section(".after_vectors"),weak,naked))
HardFault_Handler (void)
{
  asm volatile(
      " movs r0,#4      \n"
      " mov r1,lr       \n"
      " tst r0,r1       \n"
      " beq 1f          \n"
      " mrs r0,psp      \n"
      " b   2f          \n"
      "1:               \n"
      " mrs r0,msp      \n"
      "2:"
      " mov r1,lr       \n"
      " ldr r2,=HardFault_Handler_C \n"
      " bx r2"

      : /* Outputs */
      : /* Inputs */
      : /* Clobbers */
  );
}

void __attribute__ ((section(".after_vectors"),weak,used))
HardFault_Handler_C (ExceptionStackFrame* frame __attribute__((unused)),
                     uint32_t lr __attribute__((unused)))
{
#ifdef CONFIG_WIFI_FW_COREDUMP_SUPPORT
  ERR_FATAL_EXCEPTION("Exception Detected: Hard Fault", __LINE__, __FILE__);
#else
  // There is no semihosting support for Cortex-M0, since on ARMv6-M
  // faults are fatal and it is not possible to return from the handler.
#if defined(TRACE)
  trace_printf ("[HardFault]\n");
  dumpExceptionStack (frame, lr);
#endif // defined(TRACE)

#if defined(DEBUG)
  __DEBUG_BKPT();
#endif
  while (1)
    {
    }
#endif
}

#endif // defined(__ARM_ARCH_6M__)


#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

void __attribute__ ((section(".after_ram_vectors"),weak))
MemManage_Handler (void)
{
#ifdef CONFIG_WIFI_FW_COREDUMP_SUPPORT
  asm volatile(
    " tst lr,#4       \n"
    " ite eq          \n"
    " mrseq r0,msp    \n"
    " mrsne r0,psp    \n"
    " mov r1,lr       \n"
    " ldr r2,=MemManage_Handler_C \n"
    " bx r2"

    : /* Outputs */
    : /* Inputs */
    : /* Clobbers */
  );
#else
  MemManage_Handler_C();
#if defined(DEBUG)
  __DEBUG_BKPT();
#endif
  while (1)
    {
    }
#endif
}
void __attribute__ ((section(".after_ram_vectors"),weak,used))
MemManage_Handler_C (ExceptionStackFrame* frame __attribute__((unused)),
                    uint32_t lr __attribute__((unused)))
{
#ifdef CONFIG_WIFI_FW_COREDUMP_SUPPORT
  ERR_FATAL_EXCEPTION("Exception Detected: Memory Management Fault", __LINE__, __FILE__);
#else
	uint32_t * mmfar = SCB->MMFAR; // MemManage Fault Address.
	uint32_t * bfar = SCB->BFAR; // Bus Fault Address.
	uint32_t cfsr = SCB->CFSR; // Configurable Fault Status Registers.

	static char interrupt_string[200] = {0};
	snprintf((char *)interrupt_string, 200, "HARDFAULT HANDLER :\r\nR0  =  %08X\r\nR1  =  %08X\r\nR2  =  %08X\r\nR3  =  %08X\r\nR12 =  %08XLR  =  %08X\r\nPC  =  %08X\r\nPSR =  %08X\r\n",
			frame->r0,frame->r1,frame->r2,frame->r3,frame->r12,frame->lr,frame->pc,frame->psr);
	UART_Send_direct(interrupt_string,sizeof(interrupt_string));
	memset(interrupt_string,0,200);

	snprintf((char *)interrupt_string, 200, "\r\nMMFAR  =  %08X\r\nBFAR  =  %08X\r\nCFSR  =  %08X",&mmfar,&bfar,cfsr);
	UART_Send_direct(interrupt_string,sizeof(interrupt_string));
	memset(interrupt_string,0,200);
#endif  // ifdef CONFIG_WIFI_FW_COREDUMP_SUPPORT
}

void __attribute__ ((section(".after_ram_vectors"),weak,naked))
BusFault_Handler (void)
{
  asm volatile(
      " tst lr,#4       \n"
      " ite eq          \n"
      " mrseq r0,msp    \n"
      " mrsne r0,psp    \n"
      " mov r1,lr       \n"
      " ldr r2,=BusFault_Handler_C \n"
      " bx r2"

      : /* Outputs */
      : /* Inputs */
      : /* Clobbers */
  );
}

void __attribute__ ((section(".after_ram_vectors"),weak,used))
BusFault_Handler_C (ExceptionStackFrame* frame __attribute__((unused)),
                    uint32_t lr __attribute__((unused)))
{
#ifdef CONFIG_WIFI_FW_COREDUMP_SUPPORT
  ERR_FATAL_EXCEPTION("Exception Detected: Bus Fault", __LINE__, __FILE__);
#endif
#if defined(TRACE)
  uint32_t mmfar = SCB->MMFAR; // MemManage Fault Address
  uint32_t bfar = SCB->BFAR; // Bus Fault Address
  uint32_t cfsr = SCB->CFSR; // Configurable Fault Status Registers

  trace_printf ("[BusFault]\n");
  dumpExceptionStack (frame, cfsr, mmfar, bfar, lr);
#endif // defined(TRACE)

#if defined(DEBUG)
  __DEBUG_BKPT();
#endif
  while (1)
    {
    }
}

void __attribute__ ((section(".after_ram_vectors"),weak,naked))
UsageFault_Handler (void)
{
  asm volatile(
      " tst lr,#4       \n"
      " ite eq          \n"
      " mrseq r0,msp    \n"
      " mrsne r0,psp    \n"
      " mov r1,lr       \n"
      " ldr r2,=UsageFault_Handler_C \n"
      " bx r2"

      : /* Outputs */
      : /* Inputs */
      : /* Clobbers */
  );
}

void __attribute__ ((section(".after_ram_vectors"),weak,used))
UsageFault_Handler_C (ExceptionStackFrame* frame __attribute__((unused)),
                      uint32_t lr __attribute__((unused)))
{
#ifdef CONFIG_WIFI_FW_COREDUMP_SUPPORT
  ERR_FATAL_EXCEPTION("Exception Detected: Usage fault", __LINE__, __FILE__);
#endif
#if defined(TRACE)
  uint32_t mmfar = SCB->MMFAR; // MemManage Fault Address
  uint32_t bfar = SCB->BFAR; // Bus Fault Address
  uint32_t cfsr = SCB->CFSR; // Configurable Fault Status Registers
#endif

#if defined(OS_DEBUG_SEMIHOSTING_FAULTS)

  if ((cfsr & (1UL << 16)) != 0) // UNDEFINSTR
    {
      // For testing purposes, instead of BKPT use 'setend be'.
      if (isSemihosting (frame, AngelSWITestFaultOpCode))
        {
          return;
        }
    }

#endif

#if defined(TRACE)
  trace_printf ("[UsageFault]\n");
  dumpExceptionStack (frame, cfsr, mmfar, bfar, lr);
#endif // defined(TRACE)

#if defined(DEBUG)
  __DEBUG_BKPT();
#endif
  while (1)
    {
    }
}

#endif



#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

void __attribute__ ((section(".after_ram_vectors"),weak))
DebugMon_Handler (void)
{
#ifdef CONFIG_WIFI_FW_COREDUMP_SUPPORT
  ERR_FATAL_EXCEPTION("Exception Detected: DebugMon", __LINE__, __FILE__);
#endif
#if defined(DEBUG)
  __DEBUG_BKPT();
#endif
  while (1)
    {
    }
}

#endif

void __attribute__ ((section(".after_ram_vectors"),weak))
PendSV_Handler (void)
{
#ifdef CONFIG_WIFI_FW_COREDUMP_SUPPORT
  ERR_FATAL_EXCEPTION("Exception Detected: PendSV", __LINE__, __FILE__);
#endif
#if defined(DEBUG)
  __DEBUG_BKPT();
#endif
  while (1)
    {
    }
}

// Neutrino Interrupts
uint64_t value=0;

void __attribute__ ((section(".after_ram_vectors"),weak))
uart_irq_handler (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();
#endif

  value = 0x00000008;
  nt_disable_nvic(value);
}

void __attribute__ ((section(".after_ram_vectors"),weak))
I2C_irq_handler (void)
{
/*#if defined(DEBUG)
  __DEBUG_BKPT();
#endif*/
    //static char a[50];
    //rxd_data= HAL_REG_RD(QWLAN_I2C_I2C_IC_CLR_INTR_REG);
//
//
////HAL_REG_WR(QWLAN_I2C_I2C_IC_DATA_CMD_REG,0xA4);
////HAL_REG_WR(QWLAN_I2C_I2C_IC_DATA_CMD_REG,0x0F);
////HAL_REG_WR(QWLAN_I2C_I2C_IC_DATA_CMD_REG,0x208);
//// raw_status=HAL_REG_RD(QWLAN_I2C_I2C_IC_CLR_INTR_REG);
//uint32_t raw_status=HAL_REG_RD(QWLAN_I2C_I2C_IC_RAW_INTR_STAT_REG);
//uint32_t status=0;
//if(raw_status == 16)
//{
//raw_status=HAL_REG_RD(QWLAN_I2C_I2C_IC_CLR_INTR_REG);
//}
//else
//{
//
//status=HAL_REG_RD(QWLAN_I2C_I2C_IC_RAW_INTR_STAT_REG);
//status=HAL_REG_RD(QWLAN_I2C_I2C_IC_CLR_TX_ABRT_REG);//QWLAN_I2C_I2C_IC_CLR_INTR_REG
//status=HAL_REG_RD(QWLAN_I2C_I2C_IC_CLR_RX_OVER_REG);
//status=HAL_REG_RD(QWLAN_I2C_I2C_IC_CLR_RD_REQ_REG);
//}
//status++;
 // value = 0x00000004;
 // nt_disable_nvic(value);
}

void __attribute__ ((section(".after_ram_vectors"),weak))
ext_irq_handler (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();
#endif

  value = 0x00000029;
  nt_disable_nvic(value);
}

void __attribute__ ((section(".after_ram_vectors"),weak))
nt_dxe_interrupt_handler (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();
#endif
  value = 0xFFE00000;
  nt_disable_nvic(value);
  value = 0x100000000;
  nt_disable_nvic(value);

}

void __attribute__ ((section(".after_ram_vectors"),weak))
wlan_ccu_fiq_handler (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();
#endif

//  value = 0x00000004;
//  nt_disable_nvic(value);
}

void __attribute__ ((section(".after_ram_vectors"),weak))
Aon_cmnss_wlan_slp_tmr_int (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();
#endif

//  value = 0x00000004;
//  nt_disable_nvic(value);
}

void __attribute__ ((section(".after_ram_vectors"),weak))
nt_wdt_int_wcss_wdog_bark  (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();
#endif

//  value = 0x00000004;
//  nt_disable_nvic(value);
}

void __attribute__ ((section(".after_ram_vectors"),weak))
wur_cpu_int  (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();
#endif

//  value = 0x00000004;
//  nt_disable_nvic(value);
}

void __attribute__ ((section(".after_ram_vectors"),weak))
nt_spi_slv_interrupt  (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();

#endif

}

void __attribute__ ((section(".after_ram_vectors"),weak))
 nt_cpr_isr_handler(void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();

#endif

}

void __attribute__ ((section(".after_ram_vectors"),weak))
nt_gpio_interrupt_enable  (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();

#endif

}

#if (NT_CHIP_VERSION==2)
void __attribute__ ((section(".after_ram_vectors"),weak))
pmu_ccpu_slp_cal_done_intr  (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();

#endif

}
void __attribute__ ((section(".after_ram_vectors"),weak))
pmu_ccpu_temp_mon_done_intr  (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();

#endif

}
void __attribute__ ((section(".after_ram_vectors"),weak))
pmu_ccpu_vbat_mon_done_intr  (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();

#endif

}
void __attribute__ ((section(".after_ram_vectors"),weak))
pmu_ccpu_bbpll_lock_timeout_intr  (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();

#endif

}
void __attribute__ ((section(".after_ram_vectors"),weak))
pmu_ccpu_bbpll_lock_toggle_intr  (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();

#endif

}
void __attribute__ ((section(".after_ram_vectors"),weak))
pmu_ccpu_xip_pwr_down_early_warn_to_ok_to_intr  (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();

#endif

}
void __attribute__ ((section(".after_ram_vectors"),weak))
pmu_ccpu_g2p_pready_timeout_intr  (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();

#endif

}
void __attribute__ ((section(".after_ram_vectors"),weak))
pmu_ccpu_boot_strap_config_otp_read_err_intr  (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();

#endif

}
void __attribute__ ((section(".after_ram_vectors"),weak))
pmu_ccpu_rfa_pmic_otp_read_err_intr  (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();

#endif

}
void __attribute__ ((section(".after_ram_vectors"),weak))
pmu_ccpu_boot_strap_config_otp_read_to_intr  (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();

#endif

}
void __attribute__ ((section(".after_ram_vectors"),weak))
pmu_ccpu_rfa_pmic_otp_read_to_intr  (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();

#endif

}
void __attribute__ ((section(".after_ram_vectors"),weak))
pmu_ccpu_wlan_sleep_ack_timout_intr  (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();

#endif

}
void __attribute__ ((section(".after_ram_vectors"),weak))
pmu_ccpu_wlan_sleep_ack_err_intr  (void)
{
#if defined(DEBUG)
  __DEBUG_BKPT();

#endif

}
void __attribute__ ((section(".after_ram_vectors"),weak))
pmu_ccpu_wlan_coext_int_err_intr  (void)
{
#if defined(DEBUG)
 __DEBUG_BKPT();

#endif

}
void __attribute__ ((section(".after_ram_vectors"),weak))
pmu_ccpu_wlan_coext_int_done_timout_intr (void)
{
#if defined(DEBUG)
 __DEBUG_BKPT();

#endif

}
void __attribute__ ((section(".after_ram_vectors"),weak))
aon_cmnss_dvs_done_int (void)
{
#if defined(DEBUG)
 __DEBUG_BKPT();

#endif

}

void __attribute__ ((section(".after_ram_vectors"),weak))
aon_cmnss_pwfm_outoff_min_max_range_int (void)
{
#if defined(DEBUG)
 __DEBUG_BKPT();

#endif

}

void __attribute__ ((section(".after_ram_vectors"),weak))
aon_cmnss_ulpm_outoff_min_max_range_int (void)
{
#if defined(DEBUG)
 __DEBUG_BKPT();

#endif

}
#endif //(NT_CHIP_VERSION==2)
// ----------------------------------------------------------------------------
