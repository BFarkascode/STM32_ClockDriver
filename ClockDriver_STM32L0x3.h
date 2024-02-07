/*
 *  Created on: Oct 30, 2023
 *  Project: STM32_ClockDriver
 *  File: ClockDriver_STM32L0x3.h
 *  Author: BalazsFarkas
 *  Processor: STM32L053R8
 *  Compiler: ARM-GCC (STM32 IDE)
 *  Program version: 1.0
 */

#ifndef INC_RCCTIMPWMDELAY_CUSTOM_H_
#define INC_RCCTIMPWMDELAY_CUSTOM_H_

#include "stdint.h"

//LOCAL CONSTANT

//FUNCTION PROTOTYPES
void SysClockConfig(void);
void TIM6Config (void);
void Delay_us(int micro_sec);
void Delay_ms(int milli_sec);
void TIM2_CH1_PWM_Config_custom (uint16_t PWM_resolution, uint16_t PWM_pulse);

#endif /* RCCTIMPWMDELAY_CUSTOM_H_ */
