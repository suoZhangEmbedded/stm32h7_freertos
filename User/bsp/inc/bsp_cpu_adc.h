/*
*********************************************************************************************************
*
*	模块名称 : 示波器模块ADC底层的驱动
*	文件名称 : bsp_adc_dso.h
*	说    明 : 头文件
*
*	Copyright (C), 2015-2020, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#ifndef __BSP_ADC_DSO_H
#define __BSP_ADC_DSO_H

//#define D112_1
#define D112_2

void DSO_ConfigCtrlGPIO(void);
void DSO_SetDC(uint8_t _ch, uint8_t _mode);
void DSO_SetGain(uint8_t _ch, uint8_t _gain);

void DSO_StartADC(uint16_t **_AdcBuf1, uint16_t **_AdcBuf2, uint32_t _uiFreq);
void DSO_PauseADC(void);
void DSO_StopADC(void);

void DSO_SetSampRate(uint32_t _ulFreq);

#endif
