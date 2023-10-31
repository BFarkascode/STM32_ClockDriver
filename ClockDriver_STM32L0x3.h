/*
 * ClockDriver_STM32L0x3.h			v.1.0
 *
 *  Created on: Oct 30, 2023
 *      Author: Balazs Farkas
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
