#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "photoresistor.h"
#include "st7735.h"
#include "uart.h"
#include "dfplayer.h"

int main(void)
{
    uint32_t score = 0;

    // 40MHz 系统时钟（TM4C123G 用 16MHz 晶振 + PLL）
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL |
                   SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    UART_Init();                 // 调试串口 UART0
    Photoresistor_Init();        // 光敏电阻 ADC
    Photoresistor_Calibrate();   // 采环境光基线
    ST7735_Init();               // 显示屏
    DFPlayer_Init();             // 音效播放模块（UART2）

    // 板载蓝灯 PF2，命中时闪一下
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);

    // 等 DFPlayer 初始化完成（读 SD 卡需要时间），约 1 秒
    SysCtlDelay(SysCtlClockGet() / 3);
    DFPlayer_SetVolume(20);      // 音量 20（范围 0~30）

    // 屏幕初始显示 0 分
    ST7735_FillScreen(ST7735_BLACK);
    ST7735_PrintInt(20, 60, score, ST7735_WHITE, ST7735_BLACK, 5);

    UART_Send("Ready. Baseline=");
    UART_SendNum(Photoresistor_GetBaseline());
    UART_Send("\r\n");

    while (1)
    {
        // 激光直射 → IsHit 只在「光从暗变亮」时返回一次 true
        if (Photoresistor_IsHit())
        {
            score++;

            UART_Send("HIT ");
            UART_SendNum(score);
            UART_Send("\r\n");

            // 命中反馈：播放音效 + 蓝灯闪 + 屏幕闪红
            DFPlayer_PlayTrack(1);                              // 播放 SD 卡 0001.mp3
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);  // 蓝灯亮
            ST7735_FillScreen(ST7735_RED);
            SysCtlDelay(SysCtlClockGet() / 30);                      // ~100ms
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);            // 蓝灯灭

            ST7735_FillScreen(ST7735_BLACK);
            ST7735_PrintInt(20, 60, score, ST7735_WHITE, ST7735_BLACK, 5);
        }

        SysCtlDelay(SysCtlClockGet() / 3000);   // ~1ms，控制主循环速度
    }
}
