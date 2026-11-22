#ifndef PHOTORESISTOR_H
#define PHOTORESISTOR_H

#include <stdint.h>
#include <stdbool.h>

// 光敏电阻模块（AO 模拟输出）驱动
// AO 接 PB4 = ADC0 模拟输入 AIN10

void     Photoresistor_Init(void);       // 初始化 ADC
void     Photoresistor_Calibrate(void);  // 采 200 次取环境光基线
uint32_t Photoresistor_Read(void);       // 读一次 ADC，返回 0~4095
uint32_t Photoresistor_GetBaseline(void); // 返回当前环境光基线
void     Photoresistor_SetMargin(uint32_t margin); // 设置突增阈值（默认 300）
bool     Photoresistor_IsHit(void);      // 边沿触发：光从暗变亮返回一次 true

#endif
