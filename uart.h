#ifndef UART_H
#define UART_H

#include <stdint.h>

// UART0（PA0=RX, PA1=TX，经 USB 虚拟串口输出）调试打印

void UART_Init(void);
void UART_SendChar(char c);
void UART_Send(const char *s);
void UART_SendNum(uint32_t n);

#endif
