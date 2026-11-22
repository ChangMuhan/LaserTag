#ifndef DFPLAYER_H
#define DFPLAYER_H

#include <stdint.h>

// DFPlayer Mini MP3 播放模块驱动（UART 控制）
// 接线：DFPlayer RX → 开发板 PD7(UART2 TX)，DFPlayer TX → 开发板 PD6(UART2 RX)
//       DFPlayer VCC → 5V，DFPlayer GND → GND
//       DFPlayer SPK_1/SPK_2 → 喇叭（自带功放，无需 PAM8403）

void DFPlayer_Init(void);              // 初始化 UART2（9600 波特率）
void DFPlayer_PlayTrack(uint16_t n);   // 播放第 n 首（对应 SD 卡 000n.mp3）
void DFPlayer_SetVolume(uint8_t vol);  // 设置音量 0~30

#endif
