/*
*********************************************************************************************************
*
*	模块名称 : DAC波形发生器
*	文件名称 : bsp_dac_wave.c
*	版    本 : V1.0
*	说    明 : 使用STM32内部DAC输出波形。支持DAC1和DAC2输出不同的波形。
*
*	修改记录 :
*		版本号  日期        作者     说明
*		V1.0    2015-10-06  armfly  正式发布
*
*	Copyright (C), 2015-2020, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#include "bsp.h"

/*
	PA4用作 DAC_OUT1
	PA5用作 DAC_OUT2   （不用第2路，该引脚控制G2B衰减）

	DAC1使用了TIM6作为定时触发， DMA通道: DMA1_Stream5
//	DAC2使用了TIM7作为定时触发， DMA通道: DMA2_Stream6	
	
	DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable 开启了DAC输出缓冲，增加驱动能力,
	开了缓冲之后，靠近0V和参考电源时，失真厉害，最低50mV
	不开缓冲波形较好，到0V目测不出明显失真。
	
	功能：
	1、输出正弦波，幅度和频率可调节
	2、输出方波，幅度偏移可调节，频率可调节，占空比可以调节
	3、输出三角波，幅度可调节，频率可调节，上升沿占比可调节
	4、基本的DAC输出直流电平的函数
*/

#define DACx                            DAC1
#define DACx_CHANNEL_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()
#define DMAx_CLK_ENABLE()               __HAL_RCC_DMA1_CLK_ENABLE()

#define DACx_CLK_ENABLE()               __HAL_RCC_DAC12_CLK_ENABLE()
#define DACx_FORCE_RESET()              __HAL_RCC_DAC12_FORCE_RESET()
#define DACx_RELEASE_RESET()            __HAL_RCC_DAC12_RELEASE_RESET()
					  
					  /* Definition for DACx Channel Pin */
#define DACx_CHANNEL_PIN                GPIO_PIN_4
#define DACx_CHANNEL_GPIO_PORT          GPIOA
					  
/* Definition for DACx's DMA_STREAM */
#define DACx_CHANNEL                    DAC_CHANNEL_1

/* Definition for DACx's DMA_STREAM */
#define DACx_DMA_INSTANCE               DMA1_Stream5

/* Definition for DACx's NVIC */
#define DACx_DMA_IRQn                   DMA1_Stream5_IRQn
#define DACx_DMA_IRQHandler             DMA1_Stream5_IRQHandler


/*  正弦波数据，12bit，1个周期128个点, 0-4095之间变化 */
const uint16_t g_SineWave128[] = {
	2047 ,
	2147 ,
	2248 ,
	2347 ,
	2446 ,
	2544 ,
	2641 ,
	2737 ,
	2830 ,
	2922 ,
	3012 ,
	3099 ,
	3184 ,
	3266 ,
	3346 ,
	3422 ,
	3494 ,
	3564 ,
	3629 ,
	3691 ,
	3749 ,
	3803 ,
	3852 ,
	3897 ,
	3938 ,
	3974 ,
	4006 ,
	4033 ,
	4055 ,
	4072 ,
	4084 ,
	4092 ,
	4094 ,
	4092 ,
	4084 ,
	4072 ,
	4055 ,
	4033 ,
	4006 ,
	3974 ,
	3938 ,
	3897 ,
	3852 ,
	3803 ,
	3749 ,
	3691 ,
	3629 ,
	3564 ,
	3494 ,
	3422 ,
	3346 ,
	3266 ,
	3184 ,
	3099 ,
	3012 ,
	2922 ,
	2830 ,
	2737 ,
	2641 ,
	2544 ,
	2446 ,
	2347 ,
	2248 ,
	2147 ,
	2047 ,
	1947 ,
	1846 ,
	1747 ,
	1648 ,
	1550 ,
	1453 ,
	1357 ,
	1264 ,
	1172 ,
	1082 ,
	995  ,
	910  ,
	828  ,
	748  ,
	672  ,
	600  ,
	530  ,
	465  ,
	403  ,
	345  ,
	291  ,
	242  ,
	197  ,
	156  ,
	120  ,
	88   ,
	61   ,
	39   ,
	22   ,
	10   ,
	2    ,
	0    ,
	2    ,
	10   ,
	22   ,
	39   ,
	61   ,
	88   ,
	120  ,
	156  ,
	197  ,
	242  ,
	291  ,
	345  ,
	403  ,
	465  ,
	530  ,
	600  ,
	672  ,
	748  ,
	828  ,
	910  ,
	995  ,
	1082 ,
	1172 ,
	1264 ,
	1357 ,
	1453 ,
	1550 ,
	1648 ,
	1747 ,
	1846 ,
	1947
};

/* 正弦波 (32样本，适合高频） */
const uint16_t g_SineWave32[32] = {
                      2047, 2447, 2831, 3185, 3498, 3750, 3939, 4056, 4095, 4056,
                      3939, 3750, 3495, 3185, 2831, 2447, 2047, 1647, 1263, 909,
                      599, 344, 155, 38, 0, 38, 155, 344, 599, 909, 1263, 1647};

/* DMA波形缓冲区 */
uint16_t g_Wave1[128];
//uint16_t g_Wave2[128];
					  
DAC_HandleTypeDef    DacHandle;
static DAC_ChannelConfTypeDef sConfig;

static void TIM6_Config(uint32_t _freq);
	
/*
*********************************************************************************************************
*	函 数 名: bsp_InitDAC1
*	功能说明: 配置PA4/DAC1。 不启用DMA。调用 bsp_SetDAC1()函数修改输出DAC值
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitDAC1(void)
{	
	/* 配置DAC, 无触发，不用DMA */
	{		
		DacHandle.Instance = DACx;

		HAL_DAC_DeInit(&DacHandle);
		
		  /*##-1- Initialize the DAC peripheral ######################################*/
		if (HAL_DAC_Init(&DacHandle) != HAL_OK)
		{
			Error_Handler(__FILE__, __LINE__);
		}

		/*##-1- DAC channel1 Configuration #########################################*/
		sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
		sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;

		if (HAL_DAC_ConfigChannel(&DacHandle, &sConfig, DAC_CHANNEL_1) != HAL_OK)
		{
			Error_Handler(__FILE__, __LINE__);
		}

		/*##-2- Enable DAC selected channel and associated DMA #############################*/
		if (HAL_DAC_Start(&DacHandle, DAC_CHANNEL_1) != HAL_OK)  
		{
			Error_Handler(__FILE__, __LINE__);
		}
	}
}

/*
*********************************************************************************************************
*	函 数 名: bsp_SetDAC1
*	功能说明: 设置DAC1输出数据寄存器，改变输出电压
*	形    参: _dac : DAC数据，0-4095
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_SetDAC1(uint16_t _dac)
{
	DAC1->DHR12R1 = _dac;
}

/*
*********************************************************************************************************
*	函 数 名: bsp_SetDAC2
*	功能说明: 设置DAC2输出数据寄存器，改变输出电压
*	形    参: _dac : DAC数据，0-4095
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_SetDAC2(uint16_t _dac)
{
	DAC1->DHR12R2 = _dac;
}

/*
*********************************************************************************************************
*	函 数 名: HAL_DAC_MspInit
*	功能说明: 配置DAC用到的时钟，引脚和DMA通道
*	形    参: hdac  DAC_HandleTypeDef类型结构体指针变量
*	返 回 值: 无
*********************************************************************************************************
*/
void HAL_DAC_MspInit(DAC_HandleTypeDef *hdac)
{
	GPIO_InitTypeDef          GPIO_InitStruct;

	/*##-1- 使能时钟 #################################*/
	/* 使能GPIO时钟 */
	DACx_CHANNEL_GPIO_CLK_ENABLE();
	/* 使能DAC外设时钟 */
	DACx_CLK_ENABLE();

	/*##-2- 配置GPIO ##########################################*/
	/* DAC Channel1 GPIO 配置 */
	GPIO_InitStruct.Pin = DACx_CHANNEL_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(DACx_CHANNEL_GPIO_PORT, &GPIO_InitStruct);
}

/*
*********************************************************************************************************
*	函 数 名: HAL_DAC_MspDeInit
*	功能说明: 复位DAC
*	形    参: hdac  DAC_HandleTypeDef类型结构体指针变量
*	返 回 值: 无
*********************************************************************************************************
*/
void HAL_DAC_MspDeInit(DAC_HandleTypeDef *hdac)
{
	/*##-1- 复位DAC外设 ##################################################*/
	DACx_FORCE_RESET();
	DACx_RELEASE_RESET();

	/*##-2- 复位DAC对应GPIO ################################*/
	HAL_GPIO_DeInit(DACx_CHANNEL_GPIO_PORT, DACx_CHANNEL_PIN);

	/*##-3- 关闭DAC用的DMA Stream ############################################*/
	HAL_DMA_DeInit(hdac->DMA_Handle1);

	/*##-4- 关闭DMA中断 ###########################################*/
	HAL_NVIC_DisableIRQ(DACx_DMA_IRQn);
}

/*
*********************************************************************************************************
*	函 数 名: HAL_TIM_Base_MspInit
*	功能说明: 初始化定时器时钟
*	形    参: htim  TIM_HandleTypeDef类型结构体指针变量
*	返 回 值: 无
*********************************************************************************************************
*/
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
	/* TIM6 时钟使能 */
	__HAL_RCC_TIM6_CLK_ENABLE();
}

/*
*********************************************************************************************************
*	函 数 名: HAL_TIM_Base_MspDeInit
*	功能说明: 复位定时器时钟
*	形    参: htim  TIM_HandleTypeDef类型结构体指针变量
*	返 回 值: 无
*********************************************************************************************************
*/
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *htim)
{
	/*##-1- 复位外设 ##################################################*/
	__HAL_RCC_TIM6_FORCE_RESET();
	__HAL_RCC_TIM6_RELEASE_RESET();
}

/*
*********************************************************************************************************
*	函 数 名: TIM6_Config
*	功能说明: 配置TIM6作为DAC触发源
*	形    参: _freq : 采样频率，单位Hz
*	返 回 值: 无
*********************************************************************************************************
*/
static void TIM6_Config(uint32_t _freq)
{
	static TIM_HandleTypeDef  htim;
	TIM_MasterConfigTypeDef sMasterConfig;

	/*##-1- Configure the TIM peripheral #######################################*/
	/* Time base configuration */
	htim.Instance = TIM6;

	htim.Init.Period            = (SystemCoreClock / 2) / _freq - 1;
	htim.Init.Prescaler         = 0;
	htim.Init.ClockDivision     = 0;
	htim.Init.CounterMode       = TIM_COUNTERMODE_UP;
	htim.Init.RepetitionCounter = 0;
	HAL_TIM_Base_Init(&htim);

	/* TIM6 TRGO selection */
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;

	HAL_TIMEx_MasterConfigSynchronization(&htim, &sMasterConfig);

	/*##-2- Enable TIM peripheral counter ######################################*/
	HAL_TIM_Base_Start(&htim);
}

/*
*********************************************************************************************************
*	函 数 名: bsp_StartDAC1_DMA
*	功能说明: 配置PA4 为DAC_OUT1, 启用DMA, 开始持续输出波形
*	形    参: _BufAddr : DMA数据缓冲区地址。 必须定位在0x24000000 RAM，或flash常量
*			  _Count  : 缓冲区样本个数
*			 _DacFreq : DAC样本更新频率, Hz,芯片规格书最高1MHz。 实测可以到10MHz.
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_StartDAC1_DMA(uint32_t _BufAddr, uint32_t _Count, uint32_t _DacFreq)
{		
	TIM6_Config(_DacFreq);  /* DAC转换频率最高1M */
		
	/* 配置DAC, TIM6触发，DMA启用 */
	{		
		DacHandle.Instance = DACx;

		HAL_DAC_DeInit(&DacHandle);
		
		  /*##-1- Initialize the DAC peripheral ######################################*/
		if (HAL_DAC_Init(&DacHandle) != HAL_OK)
		{
			Error_Handler(__FILE__, __LINE__);
		}

		/* 配置DMA */
		{
			static DMA_HandleTypeDef  hdma_dac1;
			
			/* 使能DMA1时钟 */
			DMAx_CLK_ENABLE();
			
			/* 配置 DACx_DMA_STREAM  */
			hdma_dac1.Instance = DACx_DMA_INSTANCE;

			hdma_dac1.Init.Request  = DMA_REQUEST_DAC1;

			hdma_dac1.Init.Direction = DMA_MEMORY_TO_PERIPH;
			hdma_dac1.Init.PeriphInc = DMA_PINC_DISABLE;
			hdma_dac1.Init.MemInc = DMA_MINC_ENABLE;
			hdma_dac1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
			hdma_dac1.Init.MemDataAlignment = DMA_PDATAALIGN_HALFWORD;
			hdma_dac1.Init.Mode = DMA_CIRCULAR;
			hdma_dac1.Init.Priority = DMA_PRIORITY_HIGH;

			HAL_DMA_Init(&hdma_dac1);

			/* 关联DMA句柄到DAC句柄下 */
			__HAL_LINKDMA(&DacHandle, DMA_Handle1, hdma_dac1);
		}		

		/*##-1- DAC channel1 Configuration #########################################*/
		sConfig.DAC_Trigger = DAC_TRIGGER_T6_TRGO;
		sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;

		if (HAL_DAC_ConfigChannel(&DacHandle, &sConfig, DAC_CHANNEL_1) != HAL_OK)
		{
			Error_Handler(__FILE__, __LINE__);
		}

		/*##-2- Enable DAC selected channel and associated DMA #############################*/
		if (HAL_DAC_Start_DMA(&DacHandle, DAC_CHANNEL_1, (uint32_t *)_BufAddr, _Count, DAC_ALIGN_12B_R) != HAL_OK)  
		{
			Error_Handler(__FILE__, __LINE__);
		}
	}
}

/*
*********************************************************************************************************
*	函 数 名: dac1_SetSinWave
*	功能说明: DAC1输出正弦波
*	形    参: _vpp : 幅度 0-2047;
*			  _freq : 频率
*	返 回 值: 无
*********************************************************************************************************
*/
void dac1_SetSinWave(uint16_t _vpp, uint32_t _freq)
{	
	uint32_t i;
	uint32_t dac;
		
	/* 调整正弦波幅度 */		
	for (i = 0; i < 128; i++)
	{
		dac = (g_SineWave128[i] * _vpp) / 2047;
		if (dac > 4095)
		{
			dac = 4095;	
		}
		g_Wave1[i] = dac;
	}
	
	bsp_StartDAC1_DMA((uint32_t)&g_Wave1, 128, _freq * 128);
}

/*
*********************************************************************************************************
*	函 数 名: dac1_SetRectWave
*	功能说明: DAC1输出方波
*	形    参: _low : 低电平时DAC, 
*			  _high : 高电平时DAC
*			  _freq : 频率 Hz
*			  _duty : 占空比 2% - 98%, 调节步数 1%
*	返 回 值: 无
*********************************************************************************************************
*/
void dac1_SetRectWave(uint16_t _low, uint16_t _high, uint32_t _freq, uint16_t _duty)
{	
	uint16_t i;
	
	for (i = 0; i < (_duty * 128) / 100; i++)
	{
		g_Wave1[i] = _high;
	}
	for (; i < 128; i++)
	{
		g_Wave1[i] = _low;
	}
	
	bsp_StartDAC1_DMA((uint32_t)&g_Wave1, 128, _freq * 128);
}

/*
*********************************************************************************************************
*	函 数 名: dac1_SetTriWave
*	功能说明: DAC1输出三角波
*	形    参: _low : 低电平时DAC, 
*			  _high : 高电平时DAC
*			  _freq : 频率 Hz
*			  _duty : 占空比
*	返 回 值: 无
*********************************************************************************************************
*/
void dac1_SetTriWave(uint16_t _low, uint16_t _high, uint32_t _freq, uint16_t _duty)
{	
	uint32_t i;
	uint16_t dac;
	uint16_t m;
		
	/* 构造三角波数组，128个样本，从 _low 到 _high */		
	m = (_duty * 128) / 100;
	
	if (m == 0)
	{
		m = 1;
	}
	
	if (m > 127)
	{
		m = 127;
	}
	for (i = 0; i < m; i++)
	{
		dac = _low + ((_high - _low) * i) / m;
		g_Wave1[i] = dac;
	}
	for (; i < 128; i++)
	{
		dac = _high - ((_high - _low) * (i - m)) / (128 - m);
		g_Wave1[i] = dac;
	}	
	
	bsp_StartDAC1_DMA((uint32_t)&g_Wave1, 128, _freq * 128);
}

/*
*********************************************************************************************************
*	函 数 名: dac1_StopWave
*	功能说明: 停止DAC1输出
*	形    参: 无
*			  _freq : 频率 0-5Hz
*	返 回 值: 无
*********************************************************************************************************
*/
void dac1_StopWave(void)
{	
	__HAL_RCC_DAC12_FORCE_RESET();
	__HAL_RCC_DAC12_RELEASE_RESET();
	
	HAL_DMA_DeInit(DacHandle.DMA_Handle1);
}

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
