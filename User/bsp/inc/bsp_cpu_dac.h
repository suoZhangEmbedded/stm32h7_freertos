	/*
*********************************************************************************************************
*
*	模块名称 : DAC输出波形
*	文件名称 : dac_wave.h
*	版    本 : V1.0
*
*	Copyright (C), 2015-2016, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#ifndef __BSP_DAC_WAVE_H
#define __BSP_DAC_WAVE_H

/* 这些函数是通用的设置，软件控制DAC数据 */
void bsp_InitDAC1(void);
void bsp_SetDAC1(uint16_t _dac);

void bsp_InitDAC2(void);
void bsp_SetDAC2(uint16_t _dac);

/* 下面的函数用于DMA波形发生器 */
void dac1_InitForDMA(uint32_t _BufAddr, uint32_t _Count, uint32_t _DacFreq);
void dac1_SetSinWave(uint16_t _vpp, uint32_t _freq);
void dac1_SetRectWave(uint16_t _low, uint16_t _high, uint32_t _freq, uint16_t _duty);
void dac1_SetTriWave(uint16_t _low, uint16_t _high, uint32_t _freq, uint16_t _duty);
void dac1_StopWave(void);

void dac2_InitForDMA(uint32_t _BufAddr, uint32_t _Count, uint32_t _DacFreq);
void dac2_SetSinWave(uint16_t _vpp, uint32_t _freq);
void dac2_SetRectWave(uint16_t _low, uint16_t _high, uint32_t _freq, uint16_t _duty);
void dac2_SetTriWave(uint16_t _low, uint16_t _high, uint32_t _freq, uint16_t _duty);
void dac2_StopWave(void);

extern uint16_t g_Wave1[128];
extern uint16_t g_Wave2[128];

#endif
