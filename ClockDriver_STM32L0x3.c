/*
 * ClockDriver_STM32L0x3.c			v.1.0
 *
 *  Created on: Oct 30, 2023
 *      Author: Balazs Farkas
 *
 *v.1.0
 * Below is a custom RCC configuration function, followed by the setup of TIM6 basic timer for time measurement and TIM2 for PWM.
 *
 */

#include "ClockDriver_STM32L0x3.h"
#include "stm32l053xx.h"														//device specific header file for registers

//1)We set up the core clock and the peripheral prescalers/dividers
void SysClockConfig(void) {
	/**
	 * What happens here?
	 * Firstly, we choose a clocking input, which is going to be HSI16 for us (and internal resonator).
	 * Then we set the power interface's clocking and the FLASH NVM clocking, just in case they were not set before (we assume they haven't been). We touch upon the NVM in the NVMDriver project.
	 * Both the NVM and the PWR are set as the standard recommended values. They can be both played with to optimise the power consumption of the mcu, albeit that will be for another project.
	 * We set the AHB, APB1 and APB2 clocks. These are various periphery clocks that will define at what clocking the peripheries will, well, clock.
	 * Knowing what values are set in the AHB/APB1/APB2 is extremely important to have the right clocking on the peripheries (failing to do so will not allow the periphery to work).
	 * Lastly, we set the phased-locked loop or PLL, which allows us to have a base clock of 32 MHz - the maximum for the L0x3 series. In the end, this will be our system clock that will feed the AHB/APB1/APB2
	 *
	 *
	 *
	 * 1)Enable - future - system clock and wait until it becomes available. Originally we are running on MSI.
	 * 2)Set PWREN clock and the VOLTAGE REGULATOR
	 * 3)FLASH prefetch and LATENCY
	 * 4)Set PRESCALER HCLK, PCLK1, PCLK2
	 * 5)Configure PLL
	 * 6)Enable PLL and wait until available
	 * 7)Select clock source for system and wait until available
	 *
	 **/

	//1)
	//HSI16 on, wait for ready flag
	RCC->CR |= (1<<0);															//we turn on HSI16
	while (!(RCC->CR & (1<<2)));												//and wait until it becomes stable. Bit 2 should be 1.


	//2)
	//power control enabled, put to reset value
	RCC->APB1RSTR |= (1<<28);													//reset PWR interface
	PWR->CR |= (1<<11);															//we put scale1 - 1.8V - because that is what CubeMx sets originally (should be the reset value here)
	while ((PWR->CSR & (1<<4)));												//and wait until it becomes stable. Bit 4 should be 0.

	//3)
	//Flash access control register - no prefetch, 1WS latency
	FLASH->ACR |= (1<<0);														//1 WS
	FLASH->ACR &= ~(1<<1);														//no prefetch
	FLASH->ACR |= (1<<6);														//preread
	FLASH->ACR &= ~(1<<5);														//buffer cache enable

	//4)Setting up the clocks
	//Note: this part is always specific to the usecase!
	//Here 32 MHz full speed, HSI16, PLL_mul 4, plldiv 2, pllclk, AHB presclae 1, hclk 32, ahb prescale 1, apb1 clock divider 4, apb2 clockdiv 1, pclk1 2, pclk2 1

	//AHB 1
	RCC->CFGR |= RCC_CFGR_HPRE_DIV1;											//this is the same as putting 0 everywhere. Check the stm32l053xx.h for the exact register definitions

	//APB1 1
	RCC->CFGR |= (5<<8);														//we put 101 to bits [10:8]. This should be a DIV4.
																				//Alternatively it could have been just	RCC->CFGR |= RCC_CFGR_PPRE1_DIV4;

	//APB2 1
	RCC->CFGR |= (4<<11);														//we put 100 to bits [13:11]. This should be a DIV2.
																				//Alternatively it could have been just	RCC->CFGR |= RCC_CFGR_PPRE2_DIV2;

	//PLL source HSI16
	RCC->CFGR &= ~(1<<16);

	//PLL_mul 4
	RCC->CFGR |= (1<<18);

	//pll div 2
	RCC->CFGR |= (1<<22);

	//5)Enable PLL
	//enable and ready flag for PLL
	RCC->CR |= (1<<24);															//we turn on the PLL
	while (!(RCC->CR & (1<<25)));												//and wait until it becomes available

	//6)Set PLL as system source
	RCC->CFGR |= (3<<0);														//PLL as source
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);						//system clock status (set by hardware in bits [3:2]) should be matching the PLL source status set in bits [1:0]

	SystemCoreClockUpdate();													//This CMSIS function must be called to update the system clock! If not done, we will remain in the original clocking (likely MSI).
}

//2) TIM6 setup for precise delay generation
void TIM6Config (void) {
	/**
	 * What happens here?
	 * We first enable the timer, paying VERY close attention on which APB it is connected to (APB1).
	 * We then prescale the (automatically x2 multiplied!) APB clock to have a nice round frequency.
	 * Since we will play with how far this timer counts, we put the automatic maximum value of count value to maximum. This means that this timer can only count 65535 cycles.
	 * We tgeb enable the timers and wait until it is engaged.
	 *
	 * TIM6 is a basic clock that is configured to provide a counter for a simple delay function (see below).
	 * It is connected to AP1B which is clocking at 16 MHz currently (see above the clock config for the PLL setup to generate 32 MHz and then the APB1 clock divider left at DIV4 and a PCLK multiplier of 2 - not possible to change)
	 * 1)Enable TIM6 clocking
	 * 2)Set prescaler and ARR
	 * 3)Enable timer and wait for update flag
	 **/

	//1)
	RCC->APB1ENR |= (1<<4);														//enable TIM6 clocking

	//2)

	TIM6->PSC = 16 - 1;															// 16 MHz/16 = 1 MHz -- 1 us delay
																				// Note: the timer has a prescaler, but so does APB1!
																				// Note: the timer has a x2 multiplier on the APB clock
																				// Here APB1 PCLK is 8 MHz

	TIM6->ARR = 0xFFFF;															//Maximum ARR value - how far can the timer count?

	//3)
	TIM6->CR1 |= (1<<0);														//timer counter enable bit
	while(!(TIM6->SR & (1<<0)));												//wait for the register update flag - UIF update interrupt flag
																				//update the timer if we are overflown/underflow with the counter was reinitialised.
																				//This part is necessary since we can update on the fly. We just need to wait until we are done with a counting cycle and thus an update event has been generated.
																				//also, almost everything is preloaded before it takes effect
																				//update events can be disabled by writing to the UDIS bits in CR1. UDIS as LOW is UDIS ENABLED!!!s
}


//3) Delay function for microseconds
void Delay_us(int micro_sec) {
	/**
	 * Since we can use TIM6 only to count up to maximum 65535 cycles (65535 us), we need to up-scale out counter.
	 * We do that by counting first us seconds...
	 *
	 *
	 * 1)Reset counter for TIM6
	 * 2)Wait until micro_sec
	 **/
	TIM6->CNT = 0;
	while(TIM6->CNT < micro_sec);												//Note: this is a blocking timer counter!
}


//4) Delay function for milliseconds
void Delay_ms(int milli_sec) {
	/*
	 * ...and then, ms seconds.
	 * This function will be equivalent to HAL_Delay().
	 *
	 * */

	for (uint32_t i = 0; i < milli_sec; i++){
		Delay_us(1000);															//we call the custom microsecond delay for 1000 to generate a delay of 1 millisecond
	}
}


//5) Setup for PWM using TIM2 and channel 1.
void TIM2_CH1_PWM_Config_custom (uint16_t PWM_resolution, uint16_t PWM_pulse) {
	/**
	 * We start with setting up the PA5 GPIO to receive the PWM signal: make the pin run its alternate function.
	 * We then set up TIM2 similar to how we did it for TIM6 (no pre-scaling here though). We then tell, after counting up to which value should we restart counting.
	 * We finish with all the enables that are necessary to activate the PWM functionality on the CH1 of the TIM2.
	 *
	 * TIM2 is an advanced clock that is configured to provide PWM control (see below).
	 * It is connected to AP1B which is clocking at 16 MHz currently (see above the clock config for the PLL setup to generate 32 MHz and then the APB1 clock divider left at DIV4 and a PCLK multiplier of 2 - not possible to change)
	 * 1)Set up the output pins			- we will use PA5 (inbuilt LED as the output for the PWM)
	 * 2)Set up/enable TIM2 and then, CH1. Configure CH1 as output for PWM.
	 * 3)Calibrate PWM duty cycle
	 * 4)Update TIM2 with the new values
	 **/

	//1)Enable PORT clocking in the RCC, set MODER and OSPEEDR - PA5 LED2

	RCC->IOPENR |=	(1<<0);														//PORTA clocking - PA5 is LED2
	GPIOA->MODER &= ~(1<<10);													//alternate function for PA5
	GPIOA->MODER |= (1<<11);													//alternate function for PA5

	//OSPEEDR, OTYPER and PUPDR remain as-is since we don't need more than low speed, no open drain and no pull resistors

	//Note: AFR values are in the device datasheet for L0. For TIM2 CH1, it will be AF5 for PA5 as seen on page 45 of the datasheet.
	GPIOA->AFR[0] |= (5<<20);													//PA5 AF5 setup

	//2)Set TIM2_CH1
	RCC->APB1ENR |= (1<<0);														//enable TIM2 clocking
	TIM2->PSC = 1 - 1;															// We should clock TIM2 at 16 MHz. We don't need prescaler.
	TIM2->ARR = PWM_resolution;													//we set the resolution 0f the PWM - how far shall we count before we go back to 0

	//3)Adjust PWM
	TIM2->CCR1 = PWM_pulse;														//we set the duty cycle of the PWM - after counting at which point shall we pull the output in the other direction?
	TIM2->CCMR1 |= (1<<3);														//we activate the PWM functionality by output compare preload enabled
	TIM2->CCMR1 |= (6<<4);														//PWM mode 1 selected. CH1 will be "on" as long as CNT is lower than CCR1
	//NOTE: CCMR1 CC1S is reset to 2'b0, which means CH1 is an output.
	//NOTE: we keep the original polarity (active HIGH) of the output by keeping CCER CC1P as LOW.

	//4)Enable everything
	TIM2->CCER |= (1<<0);														//we enable CH1
	TIM2->CR1 |= (1<<7);														//ARP enabled (ARR will be buffered)
	TIM2->CR1 |= (1<<0);														//timer counter enable bit
	//NOTE: we are edge aligned so no modifications on CR1 CMS

	//5)Update timer registers
	TIM2->EGR |= (1<<0);														//we update TIM2 with the new parameters. This generates an update event.

	while(!(TIM2->SR & (1<<0)));												//wait for the register update flag - UIF update interrupt flag
																				//it will go LOW - and thus, allow the code to pass - when the EGR-controlled TIM2 update has occurred successfully and the registers are updated
																				//we have an update event at the end of each overflow - count up to - or underflow - count down to - moment. Count limit is defined by the ARR.
}
