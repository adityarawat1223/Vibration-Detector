# Vibration Diagnostic Tool

A vibration diagnostic tool capable of detecting faulty bearings using a real-time frequency graph displayed on an OLED screen.

## Hardware ([Schematic](./Hardware/Docs/VibrationDT(Schematic).pdf))

Components Bill of Materials: [BOM](./Hardware/Assembly/)


![Vibration Diagonstic Board](.//Hardware/Docs/VibrationDT(Top).png)

![Vibration Diagonstic Board](.//Hardware/Docs/VibrationDT(Botttom).png)
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

## Software

To be updated after completion of firmware.