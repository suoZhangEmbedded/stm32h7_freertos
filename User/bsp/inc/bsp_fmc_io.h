/*
*********************************************************************************************************
*
*	模块名称 : STM32-V6开发板扩展IO驱动程序
*	文件名称 : bsp_ext_io.g
*	说    明 : V6开发板在FMC总线上扩展了32位输出IO。地址为 (0x6820 0000)
*
*	Copyright (C), 2015-2020, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/
#ifndef __BSP_EXT_IO_H
#define __BSP_EXT_IO_H

/* 供外部调用的函数声明 */
/*
	安富莱STM32-V6 开发板扩展口线分配: 总线地址 = 0x6820 0000
	D0  - GPRS_RERM_ON
	D1  - GPRS_RESET
	D2  - NRF24L01_CE
	D3  - NRF905_TX_EN
	D4  - NRF905_TRX_CE/VS1053_XDCS
	D5  - NRF905_PWR_UP
	D6  - ESP8266_G0
	D7  - ESP8266_G2
	
	D8  - LED1
	D9  - LED2
	D10 - LED3
	D11 - LED4
	D12 - TP_NRST
	D13 - AD7606_OS0
	D14 - AD7606_OS1
	D15 - AD7606_OS2
	
	预留的8个5V输出IO: Y50_0 - Y50_1
	D16  - Y50_0
	D17  - Y50_1
	D18  - Y50_2
	D19  - Y50_3
	D20  - Y50_4
	D21  - Y50_5
	D22  - Y50_6
	D23  - Y50_7	

	预留的8个3.3V输出IO: Y33_0 - Y33_1
	D24  - AD7606_RESET
	D25  - AD7606_RAGE
	D26  - Y33_2
	D27  - Y33_3
	D28  - Y33_4
	D29  - Y33_5
	D30  - Y33_6
	D31  - Y33_7				
*/

#ifndef GPIO_PIN_0
	#define GPIO_PIN_0                 ((uint16_t)0x0001)  /* Pin 0 selected */
	#define GPIO_PIN_1                 ((uint16_t)0x0002)  /* Pin 1 selected */
	#define GPIO_PIN_2                 ((uint16_t)0x0004)  /* Pin 2 selected */
	#define GPIO_PIN_3                 ((uint16_t)0x0008)  /* Pin 3 selected */
	#define GPIO_PIN_4                 ((uint16_t)0x0010)  /* Pin 4 selected */
	#define GPIO_PIN_5                 ((uint16_t)0x0020)  /* Pin 5 selected */
	#define GPIO_PIN_6                 ((uint16_t)0x0040)  /* Pin 6 selected */
	#define GPIO_PIN_7                 ((uint16_t)0x0080)  /* Pin 7 selected */
	#define GPIO_PIN_8                 ((uint16_t)0x0100)  /* Pin 8 selected */
	#define GPIO_PIN_9                 ((uint16_t)0x0200)  /* Pin 9 selected */
	#define GPIO_PIN_10                ((uint16_t)0x0400)  /* Pin 10 selected */
	#define GPIO_PIN_11                ((uint16_t)0x0800)  /* Pin 11 selected */
	#define GPIO_PIN_12                ((uint16_t)0x1000)  /* Pin 12 selected */
	#define GPIO_PIN_13                ((uint16_t)0x2000)  /* Pin 13 selected */
	#define GPIO_PIN_14                ((uint16_t)0x4000)  /* Pin 14 selected */
	#define GPIO_PIN_15                ((uint16_t)0x8000)  /* Pin 15 selected */
#endif	

#define GPIO_PIN_16                 ((uint32_t)0x00010000)  /* Pin 0 selected */
#define GPIO_PIN_17                 ((uint32_t)0x00020000)  /* Pin 1 selected */
#define GPIO_PIN_18                 ((uint32_t)0x00040000)  /* Pin 2 selected */
#define GPIO_PIN_19                 ((uint32_t)0x00080000)  /* Pin 3 selected */
#define GPIO_PIN_20                 ((uint32_t)0x00100000)  /* Pin 4 selected */
#define GPIO_PIN_21                 ((uint32_t)0x00200000)  /* Pin 5 selected */
#define GPIO_PIN_22                 ((uint32_t)0x00400000)  /* Pin 6 selected */
#define GPIO_PIN_23                 ((uint32_t)0x00800000)  /* Pin 7 selected */
#define GPIO_PIN_24                 ((uint32_t)0x01000000)  /* Pin 8 selected */
#define GPIO_PIN_25                 ((uint32_t)0x02000000)  /* Pin 9 selected */
#define GPIO_PIN_26                 ((uint32_t)0x04000000)  /* Pin 10 selected */
#define GPIO_PIN_27                 ((uint32_t)0x08000000)  /* Pin 11 selected */
#define GPIO_PIN_28                 ((uint32_t)0x10000000)  /* Pin 12 selected */
#define GPIO_PIN_29                 ((uint32_t)0x20000000)  /* Pin 13 selected */
#define GPIO_PIN_30                 ((uint32_t)0x40000000)  /* Pin 14 selected */
#define GPIO_PIN_31                 ((uint32_t)0x80000000)  /* Pin 15 selected */

/* 为了方便记忆，重命名上面的宏 */
#define	GPRS_TERM_ON   GPIO_PIN_0
#define	GPRS_RESET     GPIO_PIN_1
#define	NRF24L01_CE    GPIO_PIN_2
#define	NRF905_TX_EN   GPIO_PIN_3
	#define	VS1053_XRESET  GPIO_PIN_3
#define	NRF905_TRX_CE  GPIO_PIN_4
	#define VS1053_XDCS    GPIO_PIN_4	/* NRF905_TRX_CE， VS1053_XDCS 复用 */
#define	NRF905_PWR_UP  GPIO_PIN_5
#define	ESP8266_G0     GPIO_PIN_6
#define	ESP8266_G2     GPIO_PIN_7
	
#define	LED1           GPIO_PIN_8
#define	LED2           GPIO_PIN_9
#define	LED3           GPIO_PIN_10
#define	LED4           GPIO_PIN_11
#define	TP_NRST        GPIO_PIN_12
#define	AD7606_OS0     GPIO_PIN_13
#define	AD7606_OS1     GPIO_PIN_14
#define	AD7606_OS2     GPIO_PIN_15
	
#define	Y50_0          GPIO_PIN_16
#define	Y50_1          GPIO_PIN_17
#define	Y50_2          GPIO_PIN_18
#define	Y50_3          GPIO_PIN_19
#define	Y50_4          GPIO_PIN_20
#define	Y50_5          GPIO_PIN_21
#define	Y50_6          GPIO_PIN_22
#define	Y50_7          GPIO_PIN_23	

#define	AD7606_RESET   GPIO_PIN_24
#define	AD7606_RANGE   GPIO_PIN_25
#define	Y33_2          GPIO_PIN_26
#define	Y33_3          GPIO_PIN_27
#define	Y33_4          GPIO_PIN_28
#define	Y33_5          GPIO_PIN_29
#define	Y33_6          GPIO_PIN_30
#define	Y33_7          GPIO_PIN_31

void bsp_InitExtIO(void);
void HC574_SetPin(uint32_t _pin, uint8_t _value);
uint8_t HC574_GetPin(uint32_t _pin);
void HC574_TogglePin(uint32_t _pin);

extern __IO uint32_t g_HC574;

#endif

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
