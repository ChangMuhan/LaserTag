#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/pin_map.h"
#include "dfplayer.h"

// DFPlayer Mini 使用 UART2：PD6 = RX，PD7 = TX（默认 9600 波特率）

void DFPlayer_Init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    GPIOPinConfigure(GPIO_PD6_U2RX);
    GPIOPinConfigure(GPIO_PD7_U2TX);
    GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_6 | GPIO_PIN_7);
    UARTConfigSetExpClk(UART2_BASE, SysCtlClockGet(), 9600,
                        UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE);
}

// 发送一条 10 字节 DFPlayer 指令
static void send_cmd(uint8_t cmd, uint8_t param_hi, uint8_t param_lo)
{
    uint8_t buf[10];
    uint16_t sum;
    uint16_t checksum;
    int i;

    buf[0] = 0x7E;        // 起始
    buf[1] = 0xFF;        // 版本
    buf[2] = 0x06;        // 长度
    buf[3] = cmd;         // 命令
    buf[4] = 0x00;        // 反馈（0 = 无反馈）
    buf[5] = param_hi;    // 参数高字节
    buf[6] = param_lo;    // 参数低字节

    // 校验和 = 字节 1~6 之和的 16 位补码
    sum = 0xFF + 0x06 + cmd + 0x00 + param_hi + param_lo;
    checksum = 0 - sum;
    buf[7] = (uint8_t)(checksum >> 8);
    buf[8] = (uint8_t)(checksum & 0xFF);

    buf[9] = 0xEF;        // 结束

    for (i = 0; i < 10; i++)
    {
        UARTCharPut(UART2_BASE, buf[i]);
    }
}

void DFPlayer_PlayTrack(uint16_t n)
{
    send_cmd(0x03, (uint8_t)(n >> 8), (uint8_t)(n & 0xFF));
}

void DFPlayer_SetVolume(uint8_t vol)
{
    if (vol > 30)
    {
        vol = 30;
    }
    send_cmd(0x06, 0x00, vol);
}
