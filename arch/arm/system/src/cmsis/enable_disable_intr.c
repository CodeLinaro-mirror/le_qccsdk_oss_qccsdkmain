/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <stdint.h>
#include <ExceptionHandlers.h>
#include "uart.h"
#include "ferm_hkadc_drv.h"

#define WRITE_REGISTER(reg, val) *((volatile unsigned long*)(reg)) = (val)
#define READ_REGISTER(reg) *((volatile unsigned long*)(reg))

#define NT_NVIC_ICTR	0xE000E004    //Interrupt Controller type reg
#define NT_NVIC_ISER0	0xE000E100    //Irq 0 to 31 Set Enable Register
#define NT_NVIC_ISER1	0xE000E104	  //Irq 32 to 63 Set Enable Register
#define NT_NVIC_ISER2	0xE000E108		//Irq 64 to 73 Set Enable Register
#define NT_NVIC_ICER0	0xE000E180    //Irq 0 to 31 Clear Enable Register
#define NT_NVIC_ICER1	0xE000E184    //Irq 32 to 63 Clear Enable Register
#define NT_NVIC_ICER2 	0xE000E188		//Irq 64 to 73 Clear enable register
#define NT_NVIC_ICPR0   	0xE000E280		//irq 0 to 31 Clear Pending register
#define NT_NVIC_ICPR1	0xE000E284		//Irq 32 to 63 Clear Pending Register
#define NT_NVIC_ICPR2	0xE000E288		//Irq 64 to 73 Clear Pending Register
#define NT_NVIC_IABR0	0xE000E300    //Irq 0 to 31 Active Bit Register
#define NT_ICSR			0xE000E300    //Interrupt Control State Register
#define NT_IPSR			0xE000E400    //Interrupt Control State Register
#define ENABLE_ALL		0xFFFFFFFF	  // Enable 32 interrupts
#define ENABLE_24		0xFFFFFF	  // Enable 24 interrupts
#define CLEAR_IRQ		0xFFFFFFFF		  // Disable interrupts
#define ENABLE_UART_DXE 0xFFE00008	  // Enable UART and DXE interrupts
#define ENABLE_DXE1		0x00800005	  // Enable DXE interrupt
#define NVIC_ISER0_I2C_ENABLE         0x4 // I2C int enable
#define NVIC_ISER1_GPIO_ENABLE        (0x1<<48) // RTC GPIO intr enable
void nt_enable_irq()
{
	uint32_t value;
	WRITE_REGISTER(NT_NVIC_ISER0,ENABLE_UART_DXE);
	//WRITE_REGISTER(NT_NVIC_ISER1,ENABLE_DXE1);
	value = READ_REGISTER(NT_NVIC_ISER1);
	value |= (ENABLE_DXE1);
	WRITE_REGISTER(NT_NVIC_ISER1,value);

}

void __attribute__((section(".__sect_ps_txt"))) nt_set_priority( uint32_t int_num )
{
	int i;
	WRITE_REGISTER(NT_IPSR,0x40606060);
	for (i= 1; i < ( ( 32 * ( int_num + 1 ) ) >> 2 ); i++)
	{
		WRITE_REGISTER(NT_IPSR + (0x4 * i),0x60606060);
	}
}
void __attribute__((section(".__sect_ps_txt"))) nt_disable_irq()
{

	WRITE_REGISTER(NT_NVIC_ICER0,CLEAR_IRQ);
	WRITE_REGISTER(NT_NVIC_ICER1,CLEAR_IRQ);
	WRITE_REGISTER(NT_NVIC_ICER2,CLEAR_IRQ);
}
void nt_disable_nvic(uint64_t irq_number)
{
	if(irq_number < 0x100000000)
	{
		WRITE_REGISTER(NT_NVIC_ICER0,irq_number);
	}
	else
	{
		WRITE_REGISTER(NT_NVIC_ICER1,(irq_number));
	}
}
void __attribute__((section(".__sect_ps_txt"))) nt_global_irq_init(void)
{
	__asm volatile(
		"dsb \n"
		"isb \n"
		);
	nt_disable_irq();  // Disable all the interrupts
	uint32_t int_num = READ_REGISTER(NT_NVIC_ICTR);//Configure the NVIC to support 64 interrupts
	nt_set_priority(int_num);  // Setting the priority

#ifdef CONFIG_WIFI_FW_COREDUMP_SUPPORT
	//
	// Enable fault on divide-by-zero and unaligned access
	//
	SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk | SCB_CCR_BFHFNMIGN_Msk;
	
	// TODO: Evaluate and enable Unaligned Access Fault bit (SCB_CCR_UNALIGN_TRP_Msk)
	
	//
	// Enable usage fault, bus fault, and mem manage fault
	//
	SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk; // enable Usage-/Bus-/MPU Fault
	
	// TODO: Disable Context state stacking on exception entry with the FP extension
	// FPU->FPCCR &= ~FPU_FPCCR_ASPEN_Msk;
#endif
}

//function that clears the nth interrupt in the nvic
void nt_clear_device_irq(enum nt_interrupts Intr_Number){
	uint32_t preset=0;

	if(Intr_Number<32){
		preset=Intr_Number;
		WRITE_REGISTER(NT_NVIC_ICPR0,(uint32_t)(0x01<<preset));
	}
	else if(Intr_Number>31 && Intr_Number<64){
		preset=Intr_Number-32;
		WRITE_REGISTER(NT_NVIC_ICPR1,(uint32_t)(0x01<<preset));
	}
	else if(Intr_Number>63 && Intr_Number<74){
		preset=Intr_Number-64;
		WRITE_REGISTER(NT_NVIC_ICPR2,(uint32_t)(0x01<<preset));
	}
	else
	{
#ifdef NT_DEBUG
		nt_dbg_print("Intr NonExistant,OOBound");
#endif
	}
}

//function that checks the irq id enabled at NVIC or not and returns the value at the current address registers

uint8_t nt_check_enabled_device_irq(enum nt_interrupts Intr_Number)
{
	uint32_t value=0,preset=0;

		if(Intr_Number<32){

			preset=Intr_Number;
			value=READ_REGISTER(NT_NVIC_ICER0);

			if(value & 0x01<<preset)
				return 0x01;
			else
				return 0x0;
		}

		else if(Intr_Number>31 && Intr_Number<64){

			preset=Intr_Number-32;
			value=READ_REGISTER(NT_NVIC_ICER1);

			if(value & 0x01<<preset)
				return 0x00;
			else
				return 0x01;
		}
		else if(Intr_Number>63 && Intr_Number<74){
			preset=Intr_Number-64;
			value=READ_REGISTER(NT_NVIC_ICER2);

			if(value & 0x01<<preset)
				return 0x00;
			else
				return 0x01;
		}

		else{
#ifdef NT_DEBUG
		nt_dbg_print("Intr NonExistant,OOBound");
#endif
			return 0xF;

		}

}

//functions that disables the nth interrupt in the NVIC side

void nt_disable_device_irq(enum nt_interrupts Intr_Number){
	uint32_t preset=0;

	if(Intr_Number<32){
		preset=Intr_Number;
		WRITE_REGISTER(NT_NVIC_ICER0,(uint32_t)(0x01<<preset));
	}

	else if(Intr_Number>31 && Intr_Number<64){
		preset=Intr_Number-32;
		WRITE_REGISTER(NT_NVIC_ICER1,(uint32_t)(0x01<<preset));
	}
	else if(Intr_Number>63 && Intr_Number<74){
		preset=Intr_Number-64;
		WRITE_REGISTER(NT_NVIC_ICER2,(uint32_t)(0x01<<preset));
	}

	else
	{
#ifdef NT_DEBUG
		nt_dbg_print("Intr NonExistant,OOBound");
#endif
	}
}

void nt_enable_device_irq(enum nt_interrupts Intr_Number){
	uint32_t preset=0;

		if(Intr_Number<32){
			preset=Intr_Number;
			WRITE_REGISTER(NT_NVIC_ISER0,(uint32_t)(0x01<<preset));
		}

		else if(Intr_Number>31 && Intr_Number<64){
			preset=Intr_Number-32;
			WRITE_REGISTER(NT_NVIC_ISER1,(uint32_t)(0x01<<preset));
		}
		else if(Intr_Number>63 && Intr_Number<74){
			preset=Intr_Number-64;
			WRITE_REGISTER(NT_NVIC_ISER2,(uint32_t)(0x01<<preset));
		}

		else
		{
	#ifdef NT_DEBUG
			nt_dbg_print("Intr NonExistant,OOBound");
	#endif
		}


}
