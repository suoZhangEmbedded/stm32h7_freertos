/*
*********************************************************************************************************
*
*	模块名称 : 数据观察点与跟踪(DWT)模块
*	文件名称 : bsp_dwt.c
*	版    本 : V1.0
*	说    明 : 在CM3，CM4，CM7中可以有3种跟踪源：ETM, ITM 和DWT，本驱动主要实现
*              DWT中的时钟周期（CYCCNT）计数功能，此功能非常重要，可以很方便的
*              计算程序执行的时钟周期个数。
*	修改记录 :
*		版本号    日期        作者     说明
*		V1.0    2019-02-24   Eric2013 正式发布
*
*	Copyright (C), 2019-2030, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#ifndef __BSP_DWT_H
#define __BSP_DWT_H

/* 宏定义 */
#define  DWT_CYCCNT  *(volatile unsigned int *)0xE0001004
#define  DWT_CR      *(volatile unsigned int *)0xE0001000
#define  DEM_CR      *(volatile unsigned int *)0xE000EDFC
#define  DBGMCU_CR   *(volatile unsigned int *)0xE0042004
	
/* 函数声明*/
void bsp_InitDWT(void);
//void bsp_DelayDWT(uint32_t _ulDelayTime);
//void bsp_DelayUS(uint32_t _ulDelayTime);
//void bsp_DelayMS(uint32_t _ulDelayTime);

#endif

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
