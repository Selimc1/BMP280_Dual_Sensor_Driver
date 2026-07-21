# BMP280 Dual Sensor Driver

![STM32](https://img.shields.io/badge/Microcontroller-STM32F407VG-blue)
![Architecture](https://img.shields.io/badge/Architecture-Bare%20Metal-orange)

## 📌 Project Overview
Developed as part of the EEE316 Microprocessors course (Lab 4), this project features a comprehensive, register-level (bare-metal) driver for the **BMP280 Temperature and Barometric Pressure Sensor**. The unique aspect of this project is its dual-protocol support: it implements drivers to communicate with the sensor using both **SPI** and **I2C** communication protocols entirely from scratch.

## 🚀 Key Features
- **Dual Communication Protocols:** Capable of reading sensor registers via both I2C and SPI, allowing for flexible hardware integration.
- **Bare-Metal Drivers:** No HAL/LL libraries used. Direct manipulation of `I2C1`, `SPI1`, and GPIO registers.
- **Sensor Calibration Logic:** Implements the complex factory calibration algorithm required by the BMP280 to convert raw ADC readings into actual Celsius and Hectopascal (hPa) values.
- **Robust Error Handling:** Features timeout-protected I2C reading mechanisms to prevent bus freezing.
- **Custom Print Functions:** Includes lightweight UART transmit functions to send the calculated floating-point or integer data to a PC terminal.

## 🛠️ Hardware Requirements
- **STM32F407G-DISC1** Development Board.
- **BMP280 Breakout Board**.
- **USB-to-TTL Serial Converter** (for data visualization).

## ⚙️ How It Works
1. **Initialization:** The microcontroller sets up the clocks and configures the respective GPIO pins for Alternate Function (SDA/SCL for I2C, or SCK/MISO/MOSI for SPI).
2. **Calibration:** Reads the factory calibration coefficients stored in the BMP280's non-volatile memory upon startup.
3. **Data Acquisition:** The sensor is put into normal or forced mode. The STM32 reads the burst data (temperature and pressure MSB, LSB, XLSB).
4. **Compensation:** Applies the datasheet compensation formulas using the read coefficients to get the exact temperature and pressure.

## 💻 Getting Started
Open the project in **STM32CubeIDE**, compile, and flash. Ensure your BMP280 is connected to the correct pins (depending on whether you select the I2C or SPI macro in the code) and monitor the output via a TTL converter.
