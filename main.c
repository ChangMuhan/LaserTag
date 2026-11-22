#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "photoresistor.h"
#include "st7735.h"
#include "uart.h"

// 外接 RGB 灯（共阴）：R=PB0, G=PB5, B=PB6，用来做「命中变色」
#define LED_PORT GPIO_PORTB_BASE
#define LED_R    GPIO_PIN_0
#define LED_G    GPIO_PIN_5
#define LED_B    GPIO_PIN_6
#define LED_ALL  (LED_R | LED_G | LED_B)

int main(void)
{
    uint32_t score = 0;

    // 40MHz 系统时钟（TM4C123G 用 16MHz 晶振 + PLL）
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL |
                   SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    UART_Init();                 // 串口
    Photoresistor_Init();        // 光敏电阻 ADC
    Photoresistor_Calibrate();   // 采环境光基线
    ST7735_Init();               // 显示屏

    // RGB 灯初始化：待机亮绿灯
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    GPIOPinTypeGPIOOutput(LED_PORT, LED_ALL);
    GPIOPinWrite(LED_PORT, LED_ALL, LED_G);   // 绿 = 待机

    // 板载蓝灯 PF2，命中时也闪一下
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);

    // 屏幕初始显示 0 分
    ST7735_FillScreen(ST7735_BLACK);
    ST7735_PrintInt(20, 60, score, ST7735_WHITE, ST7735_BLACK, 5);

    UART_Send("Ready. Baseline=");
    UART_SendNum(Photoresistor_GetBaseline());
    UART_Send("\r\n");

    while (1)
    {
        if (Photoresistor_IsHit())
        {
            score++;

            UART_Send("HIT ");
            UART_SendNum(score);
            UART_Send("\r\n");

            // 命中变色：绿 → 红（持续约 200ms），再回绿
            GPIOPinWrite(LED_PORT, LED_ALL, LED_R);                    // 变红
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);     // 板载蓝灯亮
            ST7735_FillScreen(ST7735_RED);                              // 屏幕闪红
            SysCtlDelay(SysCtlClockGet() / 15);                         // ~200ms
            GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);               // 蓝灯灭
            GPIOPinWrite(LED_PORT, LED_ALL, LED_G);                     // 回绿

            ST7735_FillScreen(ST7735_BLACK);
            ST7735_PrintInt(20, 60, score, ST7735_WHITE, ST7735_BLACK, 5);
        }

        SysCtlDelay(SysCtlClockGet() / 3000);   // ~1ms，控制主循环速度
    }
}
