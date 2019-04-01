/*
*********************************************************************************************************
*
*	模块名称 : 红外遥控接收器驱动模块
*	文件名称 : bsp_ir_decode.c
*	版    本 : V1.0
*	说    明 : 红外遥控接收的红外信号送入CPU的 PB0/TIM3_CH3.  本驱动程序使用TIM3_CH3通道的输入捕获功能来
*				协助解码。
*
*	修改记录 :
*		版本号  日期         作者     说明
*		V1.0    2014-02-12   armfly  正式发布
*		V1.1	2015-12-09   armfly  根据CPU主频设置TIM分频系数。解决192M时，无法正确解码问题。
*
*	Copyright (C), 2015-2020, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#include "bsp.h"

#define IR_REPEAT_SEND_EN		1	/* 连发使能 */
#define IR_REPEAT_FILTER		10	/* 遥控器108ms 发持续按下脉冲, 连续按下1秒后启动重发 */

/* 定义GPIO端口 */
#define IRD_CLK_ENABLE() 	__HAL_RCC_GPIOB_CLK_ENABLE()
#define IRD_GPIO			GPIOB
#define IRD_PIN				GPIO_PIN_8

/* PB8/TIM4_CH3 捕获脉宽 */
#define	IRD_TIMx_IRQHandler		TIM4_IRQHandler
#define TIMx					TIM4
#define TIMx_CHANNEL			TIM_CHANNEL_3
#define TIMx_ACTIVE_CHANNEL 	HAL_TIM_ACTIVE_CHANNEL_3

/* Definition for TIMx's NVIC */
#define TIMx_IRQn				TIM4_IRQn

#define TIMx_CLK_ENABLE()		__HAL_RCC_TIM4_CLK_ENABLE()
#define TIMx_GPIO_AF_TIMx  		GPIO_AF2_TIM4

static TIM_HandleTypeDef    TimHandle_IR;
IRD_T g_tIR;

static void IRD_DecodeNec(uint16_t _width);

/*
*********************************************************************************************************
*	函 数 名: bsp_InitIRD
*	功能说明: 配置STM32的GPIO,用于红外遥控器解码
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitIRD(void)
{
	GPIO_InitTypeDef gpio_init;
	
	IRD_CLK_ENABLE();
	
	/* 配置DQ为开漏输出 */
	gpio_init.Mode = GPIO_MODE_AF_PP;		/* 设置开漏输出 */
	gpio_init.Pull = GPIO_NOPULL;			/* 上下拉电阻不使能 */
	gpio_init.Speed = GPIO_SPEED_FREQ_LOW;  /* GPIO速度等级 */	
	gpio_init.Pin = IRD_PIN;	
	HAL_GPIO_Init(IRD_GPIO, &gpio_init);	
}

/*
*********************************************************************************************************
*	函 数 名: IRD_StartWork
*	功能说明: 配置TIM，开始解码
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void IRD_StartWork(void)
{
	{
		GPIO_InitTypeDef gpio_init;
		
		IRD_CLK_ENABLE();
		
		/* 配置DQ为开漏输出 */
		gpio_init.Mode = GPIO_MODE_AF_PP;		/* 设置开漏输出 */
		gpio_init.Pull = GPIO_NOPULL;			/* 上下拉电阻不使能 */
		gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;  /* GPIO速度等级 */
		gpio_init.Alternate = TIMx_GPIO_AF_TIMx;
		gpio_init.Pin = IRD_PIN;	
		HAL_GPIO_Init(IRD_GPIO, &gpio_init);	
	}
  
	{
		TIM_IC_InitTypeDef     sICConfig;
		  
		/* Set TIMx instance */
		TimHandle_IR.Instance = TIMx;

		/* 设置分频为 1680/2， 捕获计数器值的单位正好是 10us, 方便脉宽比较 
			SystemCoreClock 是主频. 常用值: 168000000, 180000000,192000000
		*/

		TimHandle_IR.Init.Period            = 0xFFFF;
		TimHandle_IR.Init.Prescaler         = (SystemCoreClock / 100000) / 2;
		TimHandle_IR.Init.ClockDivision     = 0;
		TimHandle_IR.Init.CounterMode       = TIM_COUNTERMODE_UP;
		TimHandle_IR.Init.RepetitionCounter = 0;
		if(HAL_TIM_IC_Init(&TimHandle_IR) != HAL_OK)
		{
			Error_Handler(__FILE__, __LINE__);
		}

		/*##-2- Configure the Input Capture channel ################################*/ 
		/* Configure the Input Capture of channel 2 */
		sICConfig.ICPolarity  = TIM_ICPOLARITY_BOTHEDGE;
		sICConfig.ICSelection = TIM_ICSELECTION_DIRECTTI;
		sICConfig.ICPrescaler = TIM_ICPSC_DIV1;
		sICConfig.ICFilter    = 0;   
		if (HAL_TIM_IC_ConfigChannel(&TimHandle_IR, &sICConfig, TIMx_CHANNEL) != HAL_OK)
		{
			Error_Handler(__FILE__, __LINE__);
		}

		/*##-3- Start the Input Capture in interrupt mode ##########################*/
		if (HAL_TIM_IC_Start_IT(&TimHandle_IR, TIMx_CHANNEL) != HAL_OK)
		{
			Error_Handler(__FILE__, __LINE__);
		}	  
	}
	
	{
		/* TIMx Peripheral clock enable */
		TIMx_CLK_ENABLE();

		HAL_NVIC_SetPriority(TIMx_IRQn, 0, 1);

		/* Enable the TIMx global Interrupt */
		HAL_NVIC_EnableIRQ(TIMx_IRQn);	
	}
	
	g_tIR.LastCapture = 0;	
	g_tIR.Status = 0;
}


/*
*********************************************************************************************************
*	函 数 名: IRD_TIMx_IRQHandler
*	功能说明: TIM中断服务程序
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void IRD_TIMx_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&TimHandle_IR);
}

/*
*********************************************************************************************************
*	函 数 名: HAL_TIM_IC_CaptureCallback
*	功能说明: TIM中断服务程序中会调用本函数进行捕获事件处理
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	uint16_t NowCapture;
	uint16_t Width;
	
	if (htim->Channel == TIMx_ACTIVE_CHANNEL)
	{
		NowCapture = HAL_TIM_ReadCapturedValue(htim, TIMx_CHANNEL);

		if (NowCapture >= g_tIR.LastCapture)
		{
			Width = NowCapture - g_tIR.LastCapture;
		}
		else if (NowCapture < g_tIR.LastCapture)	/* 计数器抵达最大并翻转 */
		{
			Width = ((0xFFFF - g_tIR.LastCapture) + NowCapture);
		}			
		
		if ((g_tIR.Status == 0) && (g_tIR.LastCapture == 0))
		{
			g_tIR.LastCapture = NowCapture;
			return;
		}
				
		g_tIR.LastCapture = NowCapture;	/* 保存当前计数器，用于下次计算差值 */
		
		IRD_DecodeNec(Width);		/* 解码 */		
	}
}

/*
*********************************************************************************************************
*	函 数 名: IRD_StopWork
*	功能说明: 停止红外解码
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void IRD_StopWork(void)
{
	HAL_TIM_IC_DeInit(&TimHandle_IR);		
	
	HAL_TIM_IC_Stop_IT(&TimHandle_IR, TIMx_CHANNEL);
}

/*
*********************************************************************************************************
*	函 数 名: IRD_DecodeNec
*	功能说明: 按照NEC编码格式实时解码
*	形    参: _width 脉冲宽度，单位 10us
*	返 回 值: 无
*********************************************************************************************************
*/
static void IRD_DecodeNec(uint16_t _width)
{
	static uint16_t s_LowWidth;
	static uint8_t s_Byte;
	static uint8_t s_Bit;
	uint16_t TotalWitdh;
	
	/* NEC 格式 （5段）
		1、引导码  9ms低 + 4.5ms高
		2、低8位地址码  0=1.125ms  1=2.25ms    bit0先传
		3、高8位地址码  0=1.125ms  1=2.25ms
		4、8位数据      0=1.125ms  1=2.25ms
		5、8为数码反码  0=1.125ms  1=2.25ms
	*/

loop1:	
	switch (g_tIR.Status)
	{
		case 0:			/* 929 等待引导码低信号  7ms - 11ms */
			if ((_width > 700) && (_width < 1100))
			{
				g_tIR.Status = 1;
				s_Byte = 0;
				s_Bit = 0;
			}
			break;

		case 1:			/* 413 判断引导码高信号  3ms - 6ms */
			if ((_width > 313) && (_width < 600))	/* 引导码 4.5ms */
			{
				g_tIR.Status = 2;
			}
			else if ((_width > 150) && (_width < 250))	/* 2.25ms */
			{
				#ifdef IR_REPEAT_SEND_EN				
					if (g_tIR.RepeatCount >= IR_REPEAT_FILTER)
					{
						bsp_PutKey(g_tIR.RxBuf[2] + IR_KEY_STRAT);	/* 连发码 */
					}
					else
					{
						g_tIR.RepeatCount++;
					}
				#endif
				g_tIR.Status = 0;	/* 复位解码状态 */
			}
			else
			{
				/* 异常脉宽 */
				g_tIR.Status = 0;	/* 复位解码状态 */
			}
			break;
		
		case 2:			/* 低电平期间 0.56ms */
			if ((_width > 10) && (_width < 100))
			{		
				g_tIR.Status = 3;
				s_LowWidth = _width;	/* 保存低电平宽度 */
			}
			else	/* 异常脉宽 */
			{
				/* 异常脉宽 */
				g_tIR.Status = 0;	/* 复位解码器状态 */	
				goto loop1;		/* 继续判断同步信号 */
			}
			break;

		case 3:			/* 85+25, 64+157 开始连续解码32bit */						
			TotalWitdh = s_LowWidth + _width;
			/* 0的宽度为1.125ms，1的宽度为2.25ms */				
			s_Byte >>= 1;
			if ((TotalWitdh > 92) && (TotalWitdh < 132))
			{
				;					/* bit = 0 */
			}
			else if ((TotalWitdh > 205) && (TotalWitdh < 245))
			{
				s_Byte += 0x80;		/* bit = 1 */
			}	
			else
			{
				/* 异常脉宽 */
				g_tIR.Status = 0;	/* 复位解码器状态 */	
				goto loop1;		/* 继续判断同步信号 */
			}
			
			s_Bit++;
			if (s_Bit == 8)	/* 收齐8位 */
			{
				g_tIR.RxBuf[0] = s_Byte;
				s_Byte = 0;
			}
			else if (s_Bit == 16)	/* 收齐16位 */
			{
				g_tIR.RxBuf[1] = s_Byte;
				s_Byte = 0;
			}
			else if (s_Bit == 24)	/* 收齐24位 */
			{
				g_tIR.RxBuf[2] = s_Byte;
				s_Byte = 0;
			}
			else if (s_Bit == 32)	/* 收齐32位 */
			{
				g_tIR.RxBuf[3] = s_Byte;
								
				if (g_tIR.RxBuf[2] + g_tIR.RxBuf[3] == 255)	/* 检查校验 */
				{
					bsp_PutKey(g_tIR.RxBuf[2] + IR_KEY_STRAT);	/* 将键值放入KEY FIFO */
					
					g_tIR.RepeatCount = 0;	/* 重发计数器 */										
				}
				
				g_tIR.Status = 0;	/* 等待下一组编码 */
				break;
			}
			g_tIR.Status = 2;	/* 继续下一个bit */
			break;						
	}
}

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
