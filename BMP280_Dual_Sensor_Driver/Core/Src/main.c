/*
 * EEE316 Microprocessors - Lab 4
 * Author: Selim
 * Description: Bare-Metal Dual Sensor (SPI + I2C) Driver
 * Features: Timeout Protection, Custom UART Driver, Struct-based Calibration
 */

#include "stm32f4xx.h"
#include <stdint.h>

#define REG_ID            0xD0
#define REG_CTRL_MEAS     0xF4
#define REG_CONFIG        0xF5
#define REG_PRESS_MSB     0xF7
#define REG_CALIB_START   0x88

#define I2C_ADDR_W        0xEC
#define I2C_ADDR_R        0xED
#define I2C_TIMEOUT       100000

typedef struct {
    uint16_t T1; int16_t T2, T3;
    uint16_t P1; int16_t P2, P3, P4, P5, P6, P7, P8, P9;
    int32_t t_fine;
} BMP280_Calib;

BMP280_Calib cal;

void Delay(volatile uint32_t count) { while(count--) __asm("nop"); }

void UART2_Init(void) {
    RCC->AHB1ENR |= (1 << 0);
    RCC->APB1ENR |= (1 << 17);
    GPIOA->MODER &= ~((3 << 4) | (3 << 6));
    GPIOA->MODER |= (2 << 4);
    GPIOA->AFR[0] |= (7 << 8);
    USART2->BRR = 0x008B;
    USART2->CR1 |= (1 << 3) | (1 << 13);
}

void UART_Char(char c) {
    while (!(USART2->SR & (1 << 7)));
    USART2->DR = c;
}

void UART_Print(char* str) {
    while (*str) UART_Char(*str++);
}

void UART_Num(int32_t n) {
    char buf[12]; int i = 0;
    if (n < 0) { UART_Char('-'); n = -n; }
    if (n == 0) { UART_Char('0'); return; }
    while (n > 0) { buf[i++] = (n % 10) + '0'; n /= 10; }
    while (i > 0) UART_Char(buf[--i]);
}


void SPI1_Init(void) {
    RCC->APB2ENR |= (1 << 12);
    RCC->AHB1ENR |= (1 << 0);

    GPIOA->MODER &= ~(3 << 8); GPIOA->MODER |= (1 << 8); // PA4 CS Output
    GPIOA->ODR |= (1 << 4);

    GPIOA->MODER &= ~((3 << 10) | (3 << 12) | (3 << 14));
    GPIOA->MODER |= ((2 << 10) | (2 << 12) | (2 << 14)); // AF Modu
    GPIOA->AFR[0] |= ((5 << 20) | (5 << 24) | (5 << 28));

    SPI1->CR1 = (3 << 3) | (1 << 2) | (1 << 9) | (1 << 8) | (1 << 6);
}

uint8_t SPI_TxRx(uint8_t data) {
    while (!(SPI1->SR & (1 << 1))); SPI1->DR = data;
    while (!(SPI1->SR & (1 << 0))); return SPI1->DR;
}

void SPI_Write(uint8_t reg, uint8_t val) {
    GPIOA->ODR &= ~(1 << 4);
    SPI_TxRx(reg & 0x7F); SPI_TxRx(val);
    while (SPI1->SR & (1 << 7));
    GPIOA->ODR |= (1 << 4);
}

void SPI_ReadBurst(uint8_t reg, uint8_t *buf, uint8_t len) {
    GPIOA->ODR &= ~(1 << 4);
    SPI_TxRx(reg | 0x80);
    for (uint8_t i = 0; i < len; i++) buf[i] = SPI_TxRx(0x00);
    while (SPI1->SR & (1 << 7));
    GPIOA->ODR |= (1 << 4);
}


void I2C1_Init(void) {
    RCC->APB1ENR |= (1 << 21);
    RCC->AHB1ENR |= (1 << 1);

    GPIOB->MODER &= ~((3 << 12) | (3 << 14));
    GPIOB->MODER |= ((2 << 12) | (2 << 14));
    GPIOB->OTYPER |= ((1 << 6) | (1 << 7));
    GPIOB->PUPDR |= ((1 << 12) | (1 << 14));
    GPIOB->AFR[0] |= ((4 << 24) | (4 << 28));

    I2C1->CR1 |= (1 << 15); I2C1->CR1 &= ~(1 << 15);
    I2C1->CR2 = 16; I2C1->CCR = 80; I2C1->TRISE = 17;
    I2C1->CR1 |= (1 << 0);
}

uint8_t I2C_Write(uint8_t reg, uint8_t val) {
    uint32_t t = I2C_TIMEOUT;
    I2C1->CR1 |= (1 << 8);
    while (!(I2C1->SR1 & (1 << 0))) { if (--t == 0) return 1; }
    I2C1->DR = I2C_ADDR_W;
    t = I2C_TIMEOUT; while (!(I2C1->SR1 & (1 << 1))) { if (--t == 0) return 1; }
    (void)I2C1->SR2;
    I2C1->DR = reg;
    t = I2C_TIMEOUT; while (!(I2C1->SR1 & (1 << 7))) { if (--t == 0) return 1; }
    I2C1->DR = val;
    t = I2C_TIMEOUT; while (!(I2C1->SR1 & (1 << 7))) { if (--t == 0) return 1; }
    I2C1->CR1 |= (1 << 9);
    return 0;
}

uint8_t I2C_ReadBurst(uint8_t reg, uint8_t *buf, uint8_t len) {
    uint32_t t = I2C_TIMEOUT;
    I2C1->CR1 |= (1 << 8);
    while (!(I2C1->SR1 & (1 << 0))) { if (--t == 0) return 1; }
    I2C1->DR = I2C_ADDR_W;
    t = I2C_TIMEOUT; while (!(I2C1->SR1 & (1 << 1))) { if (--t == 0) return 1; }
    (void)I2C1->SR2;
    I2C1->DR = reg;
    t = I2C_TIMEOUT; while (!(I2C1->SR1 & (1 << 7))) { if (--t == 0) return 1; }
    I2C1->CR1 |= (1 << 8);
    t = I2C_TIMEOUT; while (!(I2C1->SR1 & (1 << 0))) { if (--t == 0) return 1; }
    I2C1->DR = I2C_ADDR_R;
    t = I2C_TIMEOUT; while (!(I2C1->SR1 & (1 << 1))) { if (--t == 0) return 1; }
    (void)I2C1->SR2;
    I2C1->CR1 |= (1 << 10);
    for (uint8_t i = 0; i < len; i++) {
        if (i == len - 1) { I2C1->CR1 &= ~(1 << 10); I2C1->CR1 |= (1 << 9); }
        t = I2C_TIMEOUT; while (!(I2C1->SR1 & (1 << 6))) { if (--t == 0) return 1; }
        buf[i] = I2C1->DR;
    }
    return 0;
}

int32_t Compensate_T(int32_t adc_T) {
    int32_t v1 = ((((adc_T >> 3) - ((int32_t)cal.T1 << 1))) * ((int32_t)cal.T2)) >> 11;
    int32_t v2 = (((((adc_T >> 4) - ((int32_t)cal.T1)) * ((adc_T >> 4) - ((int32_t)cal.T1))) >> 12) * ((int32_t)cal.T3)) >> 14;
    cal.t_fine = v1 + v2;
    return (cal.t_fine * 5 + 128) >> 8;
}

uint32_t Compensate_P(int32_t adc_P) {
    int32_t v1, v2; uint32_t p;
    v1 = (((int32_t)cal.t_fine) >> 1) - (int32_t)64000;
    v2 = (((v1 >> 2) * (v1 >> 2)) >> 11) * ((int32_t)cal.P6);
    v2 = v2 + ((v1 * ((int32_t)cal.P5)) << 1);
    v2 = (v2 >> 2) + (((int32_t)cal.P4) << 16);
    v1 = (((cal.P3 * (((v1 >> 2) * (v1 >> 2)) >> 13)) >> 3) + ((((int32_t)cal.P2) * v1) >> 1)) >> 18;
    v1 = ((((32768 + v1)) * ((int32_t)cal.P1)) >> 15);
    if (v1 == 0) return 0;
    p = (((uint32_t)(((int32_t)1048576) - adc_P) - (v2 >> 12))) * 3125;
    if (p < 0x80000000) p = (p << 1) / ((uint32_t)v1); else p = (p / (uint32_t)v1) * 2;
    v1 = (((int32_t)cal.P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
    v2 = (((int32_t)(p >> 2)) * ((int32_t)cal.P8)) >> 13;
    return (uint32_t)((int32_t)p + ((v1 + v2 + cal.P7) >> 4));
}

int main(void) {
    UART2_Init();
    SPI1_Init();
    I2C1_Init();

    uint8_t b[24];
    int32_t rT, rP, tC;
    uint32_t pP;

    UART_Print("\r\n=========================================\r\n");
    UART_Print("EEE316 - DUAL SENSOR (SPI + I2C) TESTI\r\n");
    UART_Print("=========================================\r\n");

    // İki sensörü de uyandır
    SPI_Write(REG_CTRL_MEAS, 0x27);
    SPI_Write(REG_CONFIG, 0x00);
    I2C_Write(REG_CTRL_MEAS, 0x27);
    I2C_Write(REG_CONFIG, 0x00);

    Delay(20000);

    // Kalibrasyon verilerini SPI sensöründen çek
    SPI_ReadBurst(REG_CALIB_START, b, 24);
    cal.T1 = (b[1]<<8)|b[0]; cal.T2 = (b[3]<<8)|b[2]; cal.T3 = (b[5]<<8)|b[4];
    cal.P1 = (b[7]<<8)|b[6]; cal.P2 = (b[9]<<8)|b[8]; cal.P3 = (b[11]<<8)|b[10];
    cal.P4 = (b[13]<<8)|b[12]; cal.P5 = (b[15]<<8)|b[14]; cal.P6 = (b[17]<<8)|b[16];
    cal.P7 = (b[19]<<8)|b[18]; cal.P8 = (b[21]<<8)|b[20]; cal.P9 = (b[23]<<8)|b[22];

    while (1) {
        SPI_ReadBurst(REG_PRESS_MSB, b, 6);
        rP = (b[0]<<12)|(b[1]<<4)|(b[2]>>4);
        rT = (b[3]<<12)|(b[4]<<4)|(b[5]>>4);
        tC = Compensate_T(rT);
        pP = Compensate_P(rP);

        UART_Print("[SPI SENSOR] Temp: "); UART_Num(tC/100); UART_Print("."); UART_Num(tC%100 < 0 ? -(tC%100) : tC%100);
        UART_Print(" C | Pressure: "); UART_Num(pP); UART_Print(" Pa\r\n");

        Delay(2000000);

        if (I2C_ReadBurst(REG_PRESS_MSB, b, 6) == 0) {
            rP = (b[0]<<12)|(b[1]<<4)|(b[2]>>4);
            rT = (b[3]<<12)|(b[4]<<4)|(b[5]>>4);
            tC = Compensate_T(rT);
            pP = Compensate_P(rP);

            UART_Print("[I2C SENSOR] Temp: "); UART_Num(tC/100); UART_Print("."); UART_Num(tC%100 < 0 ? -(tC%100) : tC%100);
            UART_Print(" C | Pressure: "); UART_Num(pP); UART_Print(" Pa\r\n");
        } else {
            UART_Print("[I2C SENSOR] HATA: Iletisim Saglanamadi!\r\n");
        }

        UART_Print("--------------------------------------------------\r\n");
        Delay(4000000);
    }
}
