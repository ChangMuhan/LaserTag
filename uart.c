#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/pin_map.h"
#include "uart.h"

void UART_Init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
        UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
}

void UART_SendChar(char c)
{
    UARTCharPut(UART0_BASE, c);
}

void UART_Send(const char *s)
{
    while (*s) UARTCharPut(UART0_BASE, *s++);
}

void UART_SendNum(uint32_t n)
{
    char buf[12];
    int i = 0;
    if (n == 0)
    {
        UARTCharPut(UART0_BASE, '0');
        return;
    }
    while (n)
    {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    while (i) UARTCharPut(UART0_BASE, buf[--i]);
}
