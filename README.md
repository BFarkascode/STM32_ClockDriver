# STM32_ClockDriver

## General description
We will be looking at various things in this project. Mind, none of the elements within the project are particularly necessary: unlike other drivers, I am yet to bump into bugs in the HAL clock/timer code that would seriously impede my capacity to program an STM32_L0x3. Nevertheless, for my own sanity, I have decided to transition away from HAL on all aspects, even if they were not showing any signs of being annoying.

As such, I want to discuss a bit the clock configuration, how to set it up and why it is important, followed by setting up a timer to generate a precise delay. We will close the project by defining a modified timer that will generate a PWM signal on the in-built LED of the nucleo board, making it gradually turn on and off slowly.

### Clocking
There are multiple ways to define the base clocking of the mcu. One can use an external source or internally one, a source with a high frequency or a low one. These sources then define, how fast peripheries can clock afterwards. (Of note, there are some designated clocks – like the 48 MHz one or the 32.768 kHz one - in there that are used uniquely to clock certain peripheries. We don’t touch upon those here.) I generally use the HSI16 – high-speed internal – source for my projects that runs at 16 MHz.

The question would arise: why wouldn’t we just clock everything as fast as possible and get on with it? Well, for most use cases, that indeed is a good approach. Nevertheless, if one want’s to be extremely power efficient, clocking the mcu faster than absolutely necessary is a no-go: the mcu’s power consumption IS frequency dependent after all.

Apart from the source clocks we use to clock the mcu, we have also the base clock (SYSCLK) of the mcu – derived from the source – that will define the actual frequency the mcu will function at. We can either use our source directly as this base clock, or we can use a phase-locked loop (PLL) to generate multiples of this source or a pre-scaler to divide it further. Of note, the maximum SYSCLK value an L0xx series mcu can tolerate is 32 MHz.

Once the SYSCLK is set – which can already be used by some peripheries if chosen - we go one layer further and set the AHB clock “bus”. This is done by setting the pre-scaler for the AHB, effectively dividing the SYSCLK. Mind, the AHB clock is the one that feeds into the direct periphery clock busses APB1 and APB2, while also being the clock source for some advanced peripherals, such as DMA.

Lastly, we move to the direct periphery clocks, APB1 and APB2 by setting their own pre-scalers.

In summary, we have multiple layers derived from the original source – SYSCLK, AHB, APB1/APB2 – where each layer has a prescaler, sometimes even a multiplier. We often have some flexibility to pick, which layer (or element) we intend to use to clock a peripheral. The benefit is that at the end of the day, we will have multiple – potentially - independent clocks in our setup that can be then synchronized to whatever we wish (or leave them asynchronous, that is also sometimes desired).

ALL peripherals must have their clocking enabled to make them work. This is made so as to allow a designated “on” switch for each peripheral. Without such, we would not be able to complete shut down peripherals, thus allowing them to unnecessarily take power.
Just a hint: we can tell, which bus is clocking our peripheral by checking, in which RCC register can we find its designated clock enable bit.

### Timers
Timers are one of the crucial control systems of any design. Similar to interrupts (IRQs) they allows us to adjust the reaction of the mcu to “human” levels and thus provide an interface for the user to interact with the mcu. If it weren’t for timers, we wouldn’t be able to tell an mcu to measure something, say, every five seconds. The mcu has, by itself, no perception of time.

What timers are essentially doing that they “tick” on a frequency defined by the clock they are attached to (APB1 or APB2) modified further by the timer’s own pre-scaler. We can then count these ticks (actually the timer’s CNT register will do it for us) and react to when a certain number of ticks is reached. This way, we can have a (relatively) precise time measuring system. (Precision can be further improved if we attach an interrupt to the timer.)

There are various types of timers, depending on how blingy they are. In the L0x3, we have two basic timers (TIM6/TIM7) which can only count, advanced timers (TIM21/TIM22) which have additional PWM, synchronization and interrupt control and super advanced timer (TIM2) which is the same as an advanced timer with some encoding added to the mix. (There are also the LPTIM – low power – timers but I don’t want to discuss them here. Maybe in the future when I manage to figure out, how to put together a proper power management project.)

While TIM2/TIM6/TIM7 are clocked from APB1, TIM21/TIM22 are clocked from APB2. Since one can pre-scale the two APBs separately, this could lead to confusion on what frequencies the timers are actually running at. (I messed it up in the Bootloader project and had to then adjust the count values to compensate.)

Of note, despite what the reference manual (refman) might suggest, there is no TIM3 on the L0x3. It does exist on other L0xx devices though.

### PWM
We can use timers in other use cases as well, for instance to generate a specific output signal like pulsed-width modulation (PWM). To generate PWM, a timer gives out either a HIGH or a LOW signal (digital signal) with the ratio between the two adjusted by:
- how far the timer has to count to restart – called the resolution of the PWM (ARR register)
- during that counting, at which point shall it change the output from HIGH to LOW (or vice versa) – called the duty cycle/pulse length of the PWM
At any rate, the frequency of the timer does not actually matter for PWM. It is only the ratio between the resolution and the duty cycle that will define it the integral of the voltage we will extract on the output.

## To read
For the RCC clocking, the following parts are "must reads":
- 7.2 Clocks (up until 7.2.9): explains the clocking options. Figure 17 is particularly important!
- 7.3 RCC registers

For the basic timer:
- 23.3. TIM6/7 functional description
- 23.4 TIM6/7 registers

And for PWM:
- 21.3.1 to 21.3.3 : they are pretty much identical to the 23.3 section in TIM6/7
- 21.3.4 Capture/compare channels: what timer channels are and what are they for
- 21.3.8 Output compare mode: to control the output, necessary for PWM
- 21.3.9 PWM mode: generation of a specific waveform (PWM)
- 21.4 TIM2/3 registers

## Particularities

### Clocking
All sources I have mentioned above must be enabled in the clocking section. If memory serves, without any enabled, we only have the MSI running at 2 MHz.

There is an entire function within the code called “SystemCoreClockUpdate()” which we haven’t defined. This function is similarly to the “NVICSetPriority” function we used within the IRQ project, is a CMSIS in-built function that is already existing within the core coding environment, so we don’t need to redefine it. We do need to use this function at the end of our clock definition code though, otherwise the clocking will not be updated.

### Timers
Timers are on the APB clocks, BUT they have a x2 multiplier on the clock to receive their own frequency. Afterwards, they are pre-scaled locally.

Once a timer is engaged, it will be running in the background, independent to the mcu. The timer’s state can be polled – which impacts precision quite significantly – or we can use IRQs on the timer.

### PWM
TIM2 was picked as the source of PWM in this project since the TIM2 PWM output of the TIM2 channel 1 is on the PA5 LED…the same as where the in-built LED is connected. Thus we can use the in-built LED to see the PWM output, we won’t need an additional LED connected to our nucleo board. This info can be found in the datasheet on Table 15.

The fact that PA5 is where the in-built LED is can be found on the schematics of the nucleo board. It is usually shown on any of the pin diagrams as well, such as the one on the official nucleo MBED webpage.

We have a lot of things to set up and enable on the TIM2 to make if PWM capable. We choose the channel we intend to use as the PWM output, followed by enabling output buffers to do something called “output compare”. We technically allow CH1 to be a HIGH output while we are lower than the pulse width we define, and then CH1 will be a LOW output until we reach the resolution value. (We can flip this in the registers in case we wish to.)

Since TIM2 needs to be flexible to allow changing in the PWM value, we don’t need to disable it to adjust the resolution or the duty cycle. Instead, these values will also be buffered when changed by code, the buffer being imported into the TIM2 when the timer is ordered to update itself (see EGR register’s UG bit). Thus, we can update TIM2 on the fly, the update being activated on the next cycle over after the UG bit is set.

### LED brightness control
A little extra information I want to share here concerns the brightness of LEDS. LEDs don’t actually react linear to voltage changes, meaning that whenever we wish to increase the (perceived) brightness of an LED linearly, we would need to change the voltage on a function closer to being exponential.

To circumvent this problem, we often use something called a “gamma correction matrix”, which is technically a lookup table to match a linearly increasing value to a linearly increasing perceived brightness.

At the very end of the project, we will implement a 256 element gamma correction matrix on the in-built LED to improve its reaction to voltage change. Thus, we won’t be giving our PWM generator direct values: we will give it the value that are stored in the gamma matrix instead. (Of note, we will have a “for” loop like before, but instead of giving these values the for loop generates directly to the PWM function, we will extract the respective element from the matrix to plug in.) 

## User guide
We do some power interface and flash memory clocking definition at the very start of the clock config function, assuming we don’t have them set before (I am not sure, what their reset value is). These are standard values here that weren’t changed from the recommended (see Table 41 in the refman for the power, section “3.3.3 Reading the NVM” for the flash). They must be set though for the mcu to work. (Playing with these values will likely be part of a yet-to-be-done power management project.)

We will be using TIM6 basic timer to provide a time measurement and TIM2 to provide PWM. The choice is such since TIM6 is simply enough for the purpose we need it for, no need to block a more advanced timer for it. In my applications, TIM2/TIM21/TIM22 are practically interchangeable, I never used the encoding option on TIM2.

TIM6 will be clocked from the APB1 clock, running at 8 MHz. The frequency of TIM6 thus will be 16 MHz. We will pre-scale the TIM6 by 16 to receive a round number for each tick (once every 1 us).

TIM6’s CNT register is being polled by the code. We aren’t using an IRQ. As mentioned, this is significantly less accurate than an IRQ, but since we are in the ballpark of milliseconds, we don’t care. We will use TIM6 time measurement to toggle the internal LED.

On the PWM, TIM2 will be 16 MHz when clocked without pre-scale from the APB1 clock. We are choosing the ARR value as 0xFF (256) since we wish to implement a small gamma correction matrix.

For simple “linear” feedback, we feed the rolling value into the function. Here, we can see that the brightness change of the LED is not uniform, it seemingly stays at full brightness a lot longer than necessary.

For “gamma corrected” feedback, we feed in the value found as the “i”-th element of the gamma matrix. With this, the previously noticed "long maximum" has been eliminated. The correction matrix is defined within the main header file.

