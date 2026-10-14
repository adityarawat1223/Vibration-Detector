
# Vibration Diagnostic Tool

A vibration diagnostic tool capable of detecting faulty bearings using a real-time frequency graph displayed on an OLED screen.

## Hardware ([Schematic](./Hardware/Docs/VibrationDT(Schematic).pdf))

Components Bill of Materials: [BOM](./Hardware/Assembly/)

![Vibration Diagnostic Board](./Hardware/Docs/VibrationDT(Top).png)

![Vibration Diagnostic Board](./Hardware/Docs/VibrationDT(Botttom).png)

### Working

#### 1. USB-C 6P (Power Only)
There are 6 pins on this USB-C receptacle. I am explaining 3 of them; the other 3 are symmetrical so the cable works when flipped:

* **A5 (B5):** The CC1 pin (CC2 if flipped). This pin tells the power source whether to supply power. The host leaves $V_{BUS}$ unpowered by default. When plugged in, our board pulls this line down to ground using a $5.1\text{ k}\Omega$ resistor. This creates a predictable voltage drop that signals the USB source to supply $5\text{V}$ over $V_{BUS}$.
* **A9 (B9):** $V_{BUS}$ ($5\text{V}$ power rail).
* **A12 (B12):** Ground return path.

#### 2. 3.3V LDO
To avoid frying the STM32F103C8T6 pins, we use an LDO regulator to step down the $5\text{V}$ rail to a stable, clean $3.3\text{V}$.

#### 3. STM32F103C8T6
Why this chip? It is one of the most cost-effective microcontrollers that gets the job done. It fits both the computational requirements for on-device processing and the budget constraints of the board.

#### 4. MPU-6050
A sensor containing an accelerometer and gyroscope. Since we communicate over the $I^2C$ protocol, sampling is typically limited to $100\text{--}400\text{ Hz}$ (the primary communication bottleneck).

#### 5. Miscellaneous
* **5-pin Header:** For programming and debugging the MCU (SWD interface).
* **4-pin Header:** For connecting the OLED display to the MCU via $I^2C$.
* **SW1 Switch:** Pulls the `BOOT0` pin high to enter the internal bootloader.
* **SW2 Switch:** Pulls the `NRST` pin low to reset the MCU.
* **Capacitors & Resistors:** High-frequency decoupling capacitors placed close to the MCU power pins, bulk filtering caps, and necessary pull-up/pull-down resistors.

## Firmware

We have coded the firmware in bare C for learning purposes. You can find all the information in the datasheets or reference manuals, which I have listed here:

* [STM32F103C8 Datasheet](https://www.st.com/resource/en/datasheet/stm32f103c8.pdf)
* [STM32F103 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
* [MPU-6050 Specifications](https://cdn.sparkfun.com/datasheets/Components/General%20IC/PS-MPU-6000A.pdf)

### Explanation 

Initially, clocks are disabled for peripherals. If you look at the memory map in the datasheets of the STM32, you will see that we enabled the clock for I2C2 using:
```c
RCC_APB1ENR |= (1 << 22);
```
And enabled the clock for Port B and AFIO (Alternate Function I/O) using:
```c
RCC_APB2ENR |= (1 << 0);
RCC_APB2ENR |= (1 << 3);
```

If you take a look at the other init functions, similar things are done where we are just enabling the functionalities we want by following the datasheet instructions.

#### Reading MPU-6050
For reading data from the MPU, we are using the $I^2C$ protocol. That is why we have two pins, PB10 and PB11, working as SCL and SDA respectively.

First, we generate the START condition by setting the 8th bit HIGH in `I2C2_CR1` (Control Register 1), then wait for the start bit to be set using Status Register 1 (`I2C2_SR1`).

Then, by reading the registers like this:
```c
temp = I2C2_SR1;
temp = I2C2_SR2;
(void)temp;
```
You are essentially clearing the `ADDR` flag. Then we send `0x3B` using the data register, which is the internal register address of the accelerometer on the MPU to start reading from.

Before we move forward, I would like to highlight that we are getting 6 bytes of data from the MPU, which represents 16-bit acceleration data for the X, Y, and Z axes.

We continue reading data according to the datasheet. Once we read the 4th byte (index 3 in code), the 5th byte is already in the data register. Because the STM32 operates using one internal shift register and one data register, our 6th byte (which is the last byte) is already in the shift register. So, we can safely pull ACK down to LOW (send NACK) and generate the STOP condition before reading the final values.

#### Algorithm 

We used the Cooley-Tukey Radix-2 FFT algorithm here. Since the math behind it is quite lengthy, here are the main highlights(From AI):

* **Power-of-Two Requirement:** It requires the input signal length $N$ to be an integer power of two ($N = 128$ in our case).
* **Divide-and-Conquer:** It recursively splits an $N$-point DFT into two $N/2$-point DFTs—one for even-indexed time samples and one for odd-indexed time samples (Decimation-in-Time, or DIT).
* **Twiddle Factors:** It combines the results of the smaller DFTs using complex exponential multipliers called twiddle factors ($e^{-2\pi j m / N}$).
* **Butterfly Structure:** The addition and subtraction steps used to merge the sub-results form a characteristic cross-over pattern known as a butterfly diagram.

---

## Disclaimer

This is a student hobby and learning project built for experimenting with bare-metal programming and vibration analysis. It is not intended for safety-critical or certified industrial machinery monitoring.

## License

This project is open-source under the [MIT License](LICENSE). Feel free to use the code and hardware design for your own learning or projects!
