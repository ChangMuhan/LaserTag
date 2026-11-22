#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/adc.h"
#include "photoresistor.h"

// 光敏电阻 AO 引脚：PB4 = ADC0 的 AIN10
#define LDR_PORT   GPIO_PORTB_BASE
#define LDR_PIN    GPIO_PIN_4
#define LDR_ADC_CH ADC_CTL_CH10

// 注意：这个模块光越强，AO 电压越低，所以「命中」是 ADC 明显下降。
#define HIT_MARGIN 200   // 命中阈值：低于「基线 - 200」算命中
#define NEAR_BAND  150   // 正常环境光允许的波动范围，超出说明被遮住或被打中

static uint32_t baseline   = 0;    // 环境光基线
static bool     armed      = true; // 是否允许计分（光离开后才重新允许）
static uint32_t upd_count  = 0;    // 基线更新计数器

// 读一次 ADC（序列 3，处理器触发）
static uint32_t adc_read(void)
{
    uint32_t v[1];
    ADCIntClear(ADC0_BASE, 3);
    ADCProcessorTrigger(ADC0_BASE, 3);
    while (!ADCIntStatus(ADC0_BASE, 3, false)) {}
    ADCSequenceDataGet(ADC0_BASE, 3, v);
    return v[0];
}

void Photoresistor_Init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    GPIOPinTypeADC(LDR_PORT, LDR_PIN);
    ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);
    ADCSequenceStepConfigure(ADC0_BASE, 3, 0,
                             LDR_ADC_CH | ADC_CTL_IE | ADC_CTL_END);
    ADCSequenceEnable(ADC0_BASE, 3);
}

void Photoresistor_Calibrate(void)
{
    uint32_t sum = 0;
    int i;

    for (i = 0; i < 200; i++)
    {
        sum += adc_read();
        SysCtlDelay(SysCtlClockGet() / 3000);  // ~1ms
    }
    baseline = sum / 200;
}

uint32_t Photoresistor_Read(void)
{
    return adc_read();
}

uint32_t Photoresistor_GetBaseline(void)
{
    return baseline;
}

void Photoresistor_SetMargin(uint32_t margin)
{
    // 供外部调命中阈值用（当前 main 里未调用）
}

// 边沿触发命中判断：
//   - 读数明显低于基线（更亮）→ 命中一次
//   - 光持续亮着 → 不再计
//   - 读数回到基线附近 → 重新武装，并缓慢更新基线
bool Photoresistor_IsHit(void)
{
    uint32_t now = adc_read();
    bool bright = (now < baseline - HIT_MARGIN);

    if (bright && armed)
    {
        armed = false;
        return true;
    }

    if (!bright)
    {
        armed = true;
        // 关键：只在「读数接近基线」（正常环境光）时才缓慢更新基线。
        // 手遮住会让读数远高于基线，此时不更新，避免基线被拉高，
        // 否则手挪开时「正常光」会被误判成更亮 → 误触发。
        if (now < baseline + NEAR_BAND)
        {
            upd_count++;
            if (upd_count >= 200)   // 主循环约 1ms，约 200ms 更新一次
            {
                upd_count = 0;
                if (now < baseline)
                    baseline -= (baseline - now) / 8;
                else
                    baseline += (now - baseline) / 8;
            }
        }
    }
    return false;
}
