/**
 * board_pins.h
 * TM4C123GH6PM 引脚分配
 * 项目: 激光打靶系统
 */

#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include <stdint.h>
#include "inc/hw_memmap.h"

/* ===== 枪端(Gun A)引脚分配 ===== */

// 激光发射 - GPIO
#define GUN_LASER_PORT          GPIO_PORTF_BASE
#define GUN_LASER_PIN           GPIO_PIN_1      // PF1 红色LED/激光

// 扳机按键
#define GUN_TRIGGER_PORT        GPIO_PORTF_BASE
#define GUN_TRIGGER_PIN         GPIO_PIN_4      // PF4 带上拉

// 震动马达(可选)
#define GUN_VIBRATION_PORT      GPIO_PORTF_BASE
#define GUN_VIBRATION_PIN       GPIO_PIN_3      // PF3

// ST7735 LCD (枪端)
#define GUN_LCD_SSI_BASE        SSI0_BASE
#define GUN_LCD_GPIO_BASE       GPIO_PORTA_BASE
#define GUN_LCD_SCK             GPIO_PIN_2      // PA2
#define GUN_LCD_MOSI            GPIO_PIN_5      // PA5
#define GUN_LCD_DC              GPIO_PIN_6      // PA6
#define GUN_LCD_CS              GPIO_PIN_3      // PA3
#define GUN_LCD_RST             GPIO_PIN_7      // PA7

// DFPlayer Mini (枪端音效) - UART
#define GUN_AUDIO_UART_BASE     UART1_BASE
#define GUN_AUDIO_GPIO_BASE     GPIO_PORTB_BASE
#define GUN_AUDIO_TX            GPIO_PIN_1      // PB1
#define GUN_AUDIO_RX            GPIO_PIN_0      // PB0


/* ===== 靶端(Target B)引脚分配 ===== */

// 光敏传感器 - ADC
#define TARGET_SENSOR_ADC       ADC0_BASE
#define TARGET_SENSOR_CH        ADC_CTL_CH0     // AIN0
#define TARGET_SENSOR_GPIO      GPIO_PORTE_BASE
#define TARGET_SENSOR_PIN       GPIO_PIN_3      // PE3/AIN0

// ST7735 LCD (靶端)
#define TARGET_LCD_SSI_BASE     SSI0_BASE
#define TARGET_LCD_GPIO_BASE    GPIO_PORTA_BASE
#define TARGET_LCD_SCK          GPIO_PIN_2      // PA2
#define TARGET_LCD_MOSI         GPIO_PIN_5      // PA5
#define TARGET_LCD_DC           GPIO_PIN_6      // PA6
#define TARGET_LCD_CS           GPIO_PIN_3      // PA3
#define TARGET_LCD_RST          GPIO_PIN_7      // PA7

// DFPlayer Mini (靶端命中音) - UART
#define TARGET_AUDIO_UART_BASE  UART1_BASE
#define TARGET_AUDIO_GPIO_BASE  GPIO_PORTB_BASE
#define TARGET_AUDIO_TX         GPIO_PIN_1      // PB1
#define TARGET_AUDIO_RX         GPIO_PIN_0      // PB0


/* ===== 双端共用: ESP-01S Wi-Fi模块 ===== */

// ESP-01S 使用 UART
#define ESP_UART_BASE           UART2_BASE
#define ESP_GPIO_BASE           GPIO_PORTD_BASE
#define ESP_TX_PIN              GPIO_PIN_7      // PD7
#define ESP_RX_PIN              GPIO_PIN_6      // PD6

// ESP 复位与使能(可选)
#define ESP_RST_PORT            GPIO_PORTD_BASE
#define ESP_RST_PIN             GPIO_PIN_2      // PD2
#define ESP_EN_PORT             GPIO_PORTD_BASE
#define ESP_EN_PIN              GPIO_PIN_3      // PD3


/* ===== 系统参数 ===== */
#define SYSTEM_CLOCK_HZ         80000000        // 80MHz
#define ESP_UART_BAUD           115200
#define AUDIO_UART_BAUD         9600

#endif // BOARD_PINS_H
