/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _NT_BL_COMMON_H_
#define _NT_BL_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif



/******************************************************************************\
 *                  Include header files from standard path                   *
\******************************************************************************/
#include <stdint.h>



/******************************************************************************\
 *                  Include header files from project path                    *
\******************************************************************************/


/******************************************************************************\
 *                        Variable Declaration                                *
\******************************************************************************/


/******************************************************************************\
 *                  Enum Definitions                                          *
\******************************************************************************/

typedef enum rst
{
	B_UNKNOWN = -1,
	SLEEP = 0,
	DEEPSLEEP,
	WUR_CPU_WAKE,
	WUR_MAIN_WAKE,
	WMAC_WAKE,
	SW_RST,
	WDT_RST,
	BROWNOUT_RST,
	P6V_TOGGLE,
	FRESH_POWER_ON
}nt_rstreason;

typedef enum type
{
	COLD_BOOT = 0,
	WARM_BOOT
}nt_bl_type;


typedef struct
{
	nt_bl_type type;
	nt_rstreason reason;
}nt_boot_type_reason;


typedef struct
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
} exception_stack_t;

typedef struct
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t r12;
    uint32_t sp;
    uint32_t lr;
    uint32_t pc;
} arm_core_regs_t;

/******************************************************************************\
 *                    Standard typedef Definitions                            *
\******************************************************************************/

typedef void (* const pHandler)(void);

/******************************************************************************\
 *                          Macro Definitions                                 *
\******************************************************************************/
#define NT_PRNG

#if defined(DEBUG)
#define __DEBUG_BKPT()  asm volatile ("bkpt 0")
#endif

#define UNUSED(x) (void)(x)
/** Definitions for error constants. */

#define NT_FAIL				-1			/** FAILUE */
#define NT_OK               0			/** No error, everything OK. */
#define NT_EPARAM           1			/** Parameter errors,  Illegal argument. */
#define NT_ERANGE           2			/** Out of range.(Total number exceeds the set maximum) */
#define NT_ENOMEM           3			/** Not enough memory. */
#define NT_EINVFRM          4			/** Invalid frame. */
#define NT_ENORES			5			/** Resource allocation failed.	*/
#define NT_EINVSTATE        6			/** State machine error. */
#define NT_EPENDING         7			/** Event pending. */
#define NT_ECONSUMED        8			/** Object consumed. */
#define NT_ETXFAIL			9			/** Transmit failure. */
#define NT_ERXFAIL			10			/** Receive failure. */
#define NT_ETIMEOUT			11          /** Timeout occurred. */
#define NT_EINVPTR			12          /** Invalid pointer. */
#define NT_EADDRUSED		13			/** Address in use.	*/
#define NT_EFAIL			14			/** Operation fail */

#define NT_ECANCELED        15			/** Action/Connection aborted.	*/
#define NT_ECONNRST			16			/** Connection reset. */
#define NT_ECONN			17			/** Not connected. */
#define NT_ECONNCLSD		18			/** Connection closed. */
#define NT_EISCONN			19			/** Already connected. */

#define NT_EIF				20			/** Low-level netif error. */
#define NT_ERTE				21			/** Routing problem. */
#define NT_EINPROGRESS		22			/** Operation in progress. */

#define NT_EINVIPADDR		23			/** Invalid IP address format received. */
#define NT_EQUEUEFAIL		24			/** Queue send/receive failed. */
#define NT_ESEMPFAIL		25			/** Semaphore give/take failed. */

#define NT_SAEFAIL			26			/** SAE Auth failures */

#define NT_ERRMAX           27			/** Error max - keep this as last entry */

// write data to particular location and read from data particular location.
#define HAL_REG_WR(_reg, _val) *((volatile unsigned long*)(_reg)) = ((unsigned long)(_val))
#define HAL_REG_RD(_reg) *((volatile unsigned long*)(_reg))
#define NT_REG_WR(_reg, _val) HAL_REG_WR(_reg, _val)
#define NT_REG_RD(_reg) HAL_REG_RD(_reg)

//Register Read and Write
#define HW_REG_WR( _reg, _val ) ( *( ( volatile uint32_t * ) (_reg ) ) ) =  _val ;\
		__asm volatile("dsb" ::: "memory")
#define HW_REG_RD( _reg ) ( *( ( volatile uint32_t* )( _reg ) ) )

//Commaon Macro
//endian conversion
#define ENDIANNESS_CHANGE(A) ((((uint32_t)(A) & 0xff000000) >> 24)  \
		| (((uint32_t)(A) & 0x00ff0000) >> 8) \
		| (((uint32_t)(A) & 0x0000ff00) << 8) \
		| (((uint32_t)(A) & 0x000000ff) << 24))

//Check the bit is set or not
#define CHECK_BIT_SET(value, pos) ( value & (1 << pos) )

// System Status Register
#if defined(PLATFORM_NT)
#define NT_COLD_BOOT_MASK ( QWLAN_PMU_SYSTEM_STATUS_SYS_RESET_BY_P6V_POK_TOGGLE_MASK | QWLAN_PMU_SYSTEM_STATUS_SYS_RESET_BY_VBATT_BROWN_OUT_MASK | \
							QWLAN_PMU_SYSTEM_STATUS_SYS_RESET_BY_WDOG_EXPIRE_MASK | QWLAN_PMU_SYSTEM_STATUS_SYS_RESET_BY_SW_HOST_MASK )
#endif

#define NT_BBPLL_TIMEOUT_LOCKED ( QWLAN_PMU_BBPLL_STATUS_BBPLL_LOCK_DET_MASK | QWLAN_PMU_BBPLL_STATUS_BBPLL_LOCK_TIMEOUT_MASK )

#define NT_SOFTWARE_RESET(){\
		HW_REG_WR( QWLAN_PMU_SYS_SOFT_RESET_REQ_REG, QWLAN_PMU_SYS_SOFT_RESET_REQ_SYS_SOFT_RESET_REQ_MASK);}
#if 0
// System Control Block
#define NT_SCB_VTOR ( 0xE000ED08 )
#define NT_SYST_CVR ( 0xE000E018 )
#define NT_NVIC_ICTR ( 0xE000E004 )
// NVIC Interrupt Set-Enable Register
#define NT_NVIC_ISER0 ( 0xE000E100 )
#define NT_NVIC_ISER1 ( 0xE000E104 )
#define NT_NVIC_ISER2 ( 0xE000E108 )
// NVIC Interrupt Clear-Enable Register
#define NT_NVIC_ICER0 ( 0xE000E180 )
#define NT_NVIC_ICER1 ( 0xE000E184 )
#define NT_NVIC_ICER2 ( 0xE000E188 )

#define NT_NVIC_IPR0 ( 0xE000E400 )    //Interrupt Control State Register

#define CLEAR_IRQ ( 0xFFFFFFFF )  // Disable interrupts
#define CLEAR10_IRQ ( 0x3FF )
#endif
/*********************Systick****************************/
#ifdef PLATFORM_FERMION
#define SYST_CLOCK ( 60000000u )
#else
#define SYST_CLOCK ( 15000000u )
#endif

#define TICK_FREQ ( 1000ul ) // 1ms tick count

#define SYST_CSR_REG ( 0xE000E010 )
#define SYST_RVR_REG ( 0xE000E014 )
#define SYST_CVR_REG ( 0xE000E018 )
#define SYST_CALIB_REG ( 0xE000E01C )

#define DEFAULT_SYST_RVR ( ( SYST_CLOCK / TICK_FREQ) - 1u )
// This will be used for Neutrino 2.0
#define SYST_TENMS ( HW_REG_RD(SYST_CALIB_REG) & 0x00FFFFFF )
#define LOAD_REG_VALUE ( SYST_TENMS ? (((SYST_TENMS * 100) / TICK_FREQ) - 1) : DEFAULT_SYST_RVR)

#define SYST_CSR_ENABLE ( 1 << 0u )
#define SYST_CSR_TICKINT ( 1 << 1u )
#define SYST_CSR_CLKSRC ( 1 << 2u )

#define NT_DISABLE_SYSTICK(){\
		HW_REG_WR( SYST_CSR_REG, (HW_REG_RD(SYST_CSR_REG) & (~(SYST_CSR_ENABLE))));}
#define NT_DISABLE_SYSTICK_INT(){\
		HW_REG_WR( SYST_CSR_REG, (HW_REG_RD(SYST_CSR_REG) & (~(SYST_CSR_TICKINT))));}

/******************************************************************************\
 *                  Extern functions prototype declaration                     *
\******************************************************************************/
void nt_boot( void );

/******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif
