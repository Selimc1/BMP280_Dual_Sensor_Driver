# BMP280 Dual Sensor Driver

Developed for the EEE316 Microprocessors course (Lab 4). It is a bare-metal (register-level) driver that reads data from a BMP280 temperature and pressure sensor using both SPI and I2C protocols.

**Hardware & Features:**
- Communication with the same sensor via both SPI and I2C
- Custom bare-metal drivers and calibration functions
- Timeout-protected I2C driver
- Custom UART numeric and string print functions
