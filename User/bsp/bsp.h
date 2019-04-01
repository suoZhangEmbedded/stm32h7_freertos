/*
*********************************************************************************************************
*
*	模块名称 : BSP模块(For STM32H7)
*	文件名称 : bsp.h
*	版    本 : V1.0
*	说    明 : 这是硬件底层驱动程序的主文件。每个c文件可以 #include "bsp.h" 来包含所有的外设驱动模块。
*			   bsp = Borad surport packet 板级支持包
*	修改记录 :
*		版本号  日期         作者       说明
*		V1.0    2018-07-29  Eric2013   正式发布
*
*	Copyright (C), 2018-2030, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#ifndef _BSP_H_
#define _BSP_H

#define STM32_V7    


/* 检查是否定义了开发板型号 */
#if !defined (STM32_V7)
	#error "Please define the board model : STM32_V7"
#endif

/* 定义 BSP 版本号 */
#define __STM32H7_BSP_VERSION		"1.1"

/* CPU空闲时执行的函数 */
//#define CPU_IDLE()		bsp_Idle()

/* 开关全局中断的宏 */
#define ENABLE_INT()	__set_PRIMASK(0)	/* 使能全局中断 */
#define DISABLE_INT()	__set_PRIMASK(1)	/* 禁止全局中断 */

/* 这个宏仅用于调试阶段排错 */
#define BSP_Printf		printf
//#define BSP_Printf(...)

#define EXTI9_5_ISR_MOVE_OUT		/* bsp.h 中定义此行，表示本函数移到 stam32f4xx_it.c。 避免重复定义 */

#define ERROR_HANDLER()		Error_Handler(__FILE__, __LINE__);

/* 默认是关闭状态 */
#define  Enable_EventRecorder  0

#if Enable_EventRecorder == 1
	#include "EventRecorder.h"
#endif

#include "stm32h7xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef TRUE
	#define TRUE  1
#endif

#ifndef FALSE
	#define FALSE 0
#endif

/* 定义优先级分组 */
#define NVIC_PREEMPT_PRIORITY	4

/* 通过取消注释或者添加注释的方式控制是否包含底层驱动模块 */
//#include "bsp_msg.h"
//#include "bsp_user_lib.h"
#include "bsp_timer.h"
#include "bsp_led.h"
#include "bsp_key.h"

//#include "bsp_cpu_rtc.h"
//#include "bsp_cpu_adc.h"
//#include "bsp_cpu_dac.h"
#include "bsp_uart_fifo.h"
//#include "bsp_uart_gps.h"
//#include "bsp_uart_esp8266.h"
//#include "bsp_uart_sim800.h"

//#include "bsp_spi_bus.h"
//#include "bsp_spi_ad9833.h"
//#include "bsp_spi_ads1256.h"
//#include "bsp_spi_dac8501.h"
//#include "bsp_spi_dac8562.h"
//#include "bsp_spi_flash.h"
//#include "bsp_spi_tm7705.h"
//#include "bsp_spi_vs1053b.h"

//#include "bsp_fmc_sdram.h"
//#include "bsp_fmc_nand_flash.h"
//#include "bsp_fmc_ad7606.h"
//#include "bsp_fmc_oled.h"
#include "bsp_fmc_io.h"

//#include "bsp_i2c_gpio.h"
//#include "bsp_i2c_bh1750.h"
//#include "bsp_i2c_bmp085.h"
//#include "bsp_i2c_eeprom_24xx.h"
//#include "bsp_i2c_hmc5883l.h"
//#include "bsp_i2c_mpu6050.h"
//#include "bsp_i2c_si4730.h"
//#include "bsp_i2c_wm8978.h"

//#include "bsp_tft_h7.h"
//#include "bsp_tft_lcd.h"
//#include "bsp_ts_touch.h"
//#include "bsp_ts_ft5x06.h"
//#include "bsp_ts_gt811.h"
//#include "bsp_ts_gt911.h"
//#include "bsp_ts_stmpe811.h"

#include "bsp_beep.h"
#include "bsp_tim_pwm.h"
//#include "bsp_sdio_sd.h"
//#include "bsp_dht11.h"
//#include "bsp_ds18b20.h"
//#include "bsp_ps2.h"
//#include "bsp_ir_decode.h"
//#include "bsp_camera.h"
//#include "bsp_rs485_led.h"
//#include "bsp_can.h"

/* 提供给其他C文件调用的函数 */
void bsp_Init(void);
void bsp_Idle(void);

void bsp_GetCpuID(uint32_t *_id);
void Error_Handler(char *file, uint32_t line);

#endif

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
