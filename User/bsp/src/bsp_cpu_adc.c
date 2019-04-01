/*
*********************************************************************************************************
*
*	模块名称 : 示波器模块ADC底层的驱动
*	文件名称 : bsp_adc_dso.c
*	版    本 : V1.0
*	说    明 : 使用STM32内部ADC，同步采集两路波形。占用了部分GPIO控制示波器模块的增益和耦合方式。
*				使用 ADC_EXTERNALTRIG_T3_TRGO 作为ADC触发源
*
*	修改记录 :
*		版本号  日期        作者     说明
*		V1.0    2018-10-25  armfly  正式发布
*
*	Copyright (C), 2018-2030, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#include "bsp.h"

/*
	STM32-V7开发板，示波器模块GPIO定义
			
	---------------------------------------------------------
	D112-2 版本的示波器模块  （新版）
		示波器通道1
			PC3  = ADC123_IN13
			AC1  = PB7    控制AC-DC切换, 1表示DC, 0表示AC
			G1A  = PC6    控制衰减比
			G1B  = PC7    控制衰减比

		示波器通道2
			PC0  = ADC123_IN10
			AC2  = PA6   控制AC-DC切换, 1表示DC, 0表示AC
			G2A  = PG10  控制衰减比
			G2B  = PA5   控制衰减比	
			
		DAC = PA4
*/	
#define AC1_CLK_ENABLE()	__HAL_RCC_GPIOB_CLK_ENABLE()
#define AC1_GPIO			GPIOB
#define AC1_PIN				GPIO_PIN_7
#define AC1_0()				AC1_GPIO->BSRRH = AC1_PIN
#define AC1_1()				AC1_GPIO->BSRRL = AC1_PIN

#define AC2_CLK_ENABLE()	__HAL_RCC_GPIOA_CLK_ENABLE()
#define AC2_GPIO			GPIOA
#define AC2_PIN				GPIO_PIN_6
#define AC2_0()				AC2_GPIO->BSRRH = AC2_PIN
#define AC2_1()				AC2_GPIO->BSRRL = AC2_PIN

#define G1A_CLK_ENABLE()	__HAL_RCC_GPIOC_CLK_ENABLE()
#define G1A_GPIO			GPIOC
#define G1A_PIN				GPIO_PIN_6
#define G1A_0()				G1A_GPIO->BSRRH = G1A_PIN
#define G1A_1()				G1A_GPIO->BSRRL = G1A_PIN

#define G1B_CLK_ENABLE()	__HAL_RCC_GPIOC_CLK_ENABLE()
#define G1B_GPIO			GPIOC
#define G1B_PIN				GPIO_PIN_7
#define G1B_0()				G1B_GPIO->BSRRH = G1B_PIN
#define G1B_1()				G1B_GPIO->BSRRL = G1B_PIN

#define G2A_CLK_ENABLE()	__HAL_RCC_GPIOG_CLK_ENABLE()
#define G2A_GPIO			GPIOG
#define G2A_PIN				GPIO_PIN_10
#define G2A_0()				G2A_GPIO->BSRRH = G2A_PIN
#define G2A_1()				G2A_GPIO->BSRRL = G2A_PIN

#define G2B_CLK_ENABLE()	__HAL_RCC_GPIOA_CLK_ENABLE()
#define G2B_GPIO			GPIOA
#define G2B_PIN				GPIO_PIN_5
#define G2B_0()				G2B_GPIO->BSRRH = G2B_PIN
#define G2B_1()				G2B_GPIO->BSRRL = G2B_PIN

    
/* ADC CH1通道GPIO, ADC通道，DMA定义 */
#define ADCH1                            ADC1
#define ADCH1_CLK_ENABLE()               __HAL_RCC_ADC12_CLK_ENABLE()

#define ADCH1_FORCE_RESET()              __HAL_RCC_ADC12_FORCE_RESET()
#define ADCH1_RELEASE_RESET()            __HAL_RCC_ADC12_RELEASE_RESET()

#define ADCH1_CHANNEL_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOC_CLK_ENABLE()
#define ADCH1_CHANNEL_PIN                GPIO_PIN_3
#define ADCH1_CHANNEL_GPIO_PORT          GPIOC

#define ADCH1_CHANNEL                    ADC_CHANNEL_13

#define CH1_DMA_CLK_ENABLE()           __HAL_RCC_DMA1_CLK_ENABLE()
#define CH1_DMA_Stream					 DMA1_Stream1
#define CH1_DMA_Stream_IRQn				 DMA1_Stream1_IRQn
#define CH1_DMA_Stream_IRQHandle         DMA1_Stream1_IRQHandler
#define CH1_DMA_REQUEST_ADC				 DMA_REQUEST_ADC1

/* ADC CH2通道GPIO, ADC通道，DMA定义 */
#if 1
	#define ADCH2                            ADC2
	#define ADCH2_CLK_ENABLE()               __HAL_RCC_ADC12_CLK_ENABLE()

	#define ADCH2_FORCE_RESET()              __HAL_RCC_ADC12_FORCE_RESET()
	#define ADCH2_RELEASE_RESET()            __HAL_RCC_ADC12_RELEASE_RESET()

	#define ADCH2_CHANNEL_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOC_CLK_ENABLE()
	#define ADCH2_CHANNEL_PIN                GPIO_PIN_0
	#define ADCH2_CHANNEL_GPIO_PORT          GPIOC

	#define ADCH2_CHANNEL                    ADC_CHANNEL_10


	#define CH2_DMA_CLK_ENABLE()           __HAL_RCC_DMA2_CLK_ENABLE()
	#define CH2_DMA_Stream					 DMA2_Stream1
	#define CH2_DMA_Stream_IRQn				 DMA2_Stream1_IRQn
	#define CH2_DMA_Stream_IRQHandle         DMA2_Stream1_IRQHandler
	#define CH2_DMA_REQUEST_ADC				 DMA_REQUEST_ADC2
#else
	#define ADCH2                            ADC3
	#define ADCH2_CLK_ENABLE()               __HAL_RCC_ADC3_CLK_ENABLE()

	#define ADCH2_FORCE_RESET()              __HAL_RCC_ADC3_FORCE_RESET()
	#define ADCH2_RELEASE_RESET()            __HAL_RCC_ADC3_RELEASE_RESET()

	#define ADCH2_CHANNEL_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOC_CLK_ENABLE()
	#define ADCH2_CHANNEL_PIN                GPIO_PIN_3
	#define ADCH2_CHANNEL_GPIO_PORT          GPIOC

	#define ADCH2_CHANNEL                    ADC_CHANNEL_13


	#define CH2_DMA_CLK_ENABLE()           __HAL_RCC_DMA2_CLK_ENABLE()
	#define CH2_DMA_Stream					 DMA2_Stream1
	#define CH2_DMA_Stream_IRQn				 DMA2_Stream1_IRQn
	#define CH2_DMA_Stream_IRQHandle         DMA2_Stream1_IRQHandler
	#define CH2_DMA_REQUEST_ADC				 DMA_REQUEST_ADC3
#endif

/* Definition of ADCH1 conversions data table size */
#define ADC_BUFFER_SIZE   ((uint32_t)  1024)   /* Size of array aADCH1ConvertedData[], Aligned on cache line size */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* ADC handle declaration */
ADC_HandleTypeDef             AdcHandle1 = {0};
ADC_HandleTypeDef             AdcHandle2 = {0};

/* ADC channel configuration structure declaration */
ADC_ChannelConfTypeDef        sConfig1 = {0};
ADC_ChannelConfTypeDef        sConfig2 = {0};

/* Variable containing ADC conversions data */
ALIGN_32BYTES (uint16_t   aADCH1ConvertedData[ADC_BUFFER_SIZE]);

ALIGN_32BYTES (uint16_t   aADCH2ConvertedData[ADC_BUFFER_SIZE]);

static void TIM3_Config(uint32_t _freq);

/*
*********************************************************************************************************
*	函 数 名: DSO_InitHard
*	功能说明: 配置控制用通道耦合和增益的GPIO, 配置ADC
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void DSO_InitHard(void)
{
	DSO_ConfigCtrlGPIO();
}

/*
*********************************************************************************************************
*	函 数 名: DSO_ConfigCtrlGPIO
*	功能说明: 配置控制用通道耦合和增益的GPIO
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void DSO_ConfigCtrlGPIO(void)
{
	/* 配置控制增益和耦合的GPIO */
	GPIO_InitTypeDef gpio_init;

	/* 打开GPIO时钟 */
	AC1_CLK_ENABLE();
	AC2_CLK_ENABLE();
	G1A_CLK_ENABLE();
	G1B_CLK_ENABLE();
	G2A_CLK_ENABLE();
	G2B_CLK_ENABLE();
	
	gpio_init.Mode = GPIO_MODE_OUTPUT_PP;		/* 设置推挽输出 */
	gpio_init.Pull = GPIO_NOPULL;				/* 上下拉电阻不使能 */
	gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;  	/* GPIO速度等级 */	
	
	gpio_init.Pin = AC1_PIN;	
	HAL_GPIO_Init(AC1_GPIO, &gpio_init);	
	
	gpio_init.Pin = AC2_PIN;	
	HAL_GPIO_Init(AC2_GPIO, &gpio_init);	

	gpio_init.Pin = G1A_PIN;	
	HAL_GPIO_Init(G1A_GPIO, &gpio_init);	
	
	gpio_init.Pin = G1B_PIN;	
	HAL_GPIO_Init(G1B_GPIO, &gpio_init);	
	
	gpio_init.Pin = G2A_PIN;	
	HAL_GPIO_Init(G2A_GPIO, &gpio_init);	
	
	gpio_init.Pin = G2B_PIN;	
	HAL_GPIO_Init(G2B_GPIO, &gpio_init);	

	DSO_SetDC(1, 0);	/* CH1选择AC耦合 */
	DSO_SetDC(2, 0);	/* CH1选择AC耦合 */
	DSO_SetGain(1, 0);	/* CH1选择小增益 衰减1/5, 第2个参数1表示不衰减1;1 */
	DSO_SetGain(2, 0);	/* CH2选择小增益 衰减1/5, 第2个参数1表示不衰减1;1 */		
}

/*
*********************************************************************************************************
*	函 数 名: DSO_SetDC
*	功能说明: 控制AC DC耦合模式
*	形    参: _ch : 通道1或2
*			  _mode : 0或1.  1表示DC耦合 0表示AC耦合
*	返 回 值: 无
*********************************************************************************************************
*/
void DSO_SetDC(uint8_t _ch, uint8_t _mode)
{	
	if (_ch == 1)
	{
		if (_mode == 1)
		{
			AC1_1();
		}
		else
		{
			AC1_0();
		}
	}
	else
	{
		if (_mode == 1)
		{
			AC2_1();
		}
		else
		{
			AC2_0();
		}
	}	
}

/*
*********************************************************************************************************
*	函 数 名: SetGainHigh
*	功能说明: 控制增益模式
*	形    参: _ch : 通道1或2
*			  _gain : 0或1.  1表示1:1， 0表示衰减1/5
*	返 回 值: 无
*********************************************************************************************************
*/
void DSO_SetGain(uint8_t _ch, uint8_t _gain)
{	
	if (_ch == 1)
	{
		if (_gain == 0)
		{
			G1A_0();
			G1B_0();
		}
		else if (_gain == 1)
		{
			G1A_1();
			G1B_0();
		}		
		else if (_gain == 2)
		{
			G1A_0();
			G1B_1();
		}
		else if (_gain == 3)
		{
			G1A_1();
			G1B_1();
		}			
	}
	else
	{
		if (_gain == 0)
		{
			G2A_0();
			G2B_0();
		}
		else if (_gain == 1)
		{
			G2A_1();
			G2B_0();
		}		
		else if (_gain == 2)
		{
			G2A_0();
			G2B_1();
		}
		else if (_gain == 3)
		{
			G2A_1();
			G2B_1();
		}	
	}
}

/*
*********************************************************************************************************
*	函 数 名: bsp_StartAdcCH1
*	功能说明: 配置CH1通道的GPIO, ADC, DMA
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_StartAdcCH1(void)
{
	/* ### - 1 - Initialize ADC peripheral #################################### */
	AdcHandle1.Instance          = ADCH1;
	if (HAL_ADC_DeInit(&AdcHandle1) != HAL_OK)
	{
		/* ADC de-initialization Error */
		Error_Handler(__FILE__, __LINE__);
	}

	AdcHandle1.Init.ClockPrescaler           = ADC_CLOCK_SYNC_PCLK_DIV4;        /* Synchronous clock mode, input ADC clock divided by 4*/
	AdcHandle1.Init.Resolution               = ADC_RESOLUTION_12B;              /* 16-bit resolution for converted data */
	AdcHandle1.Init.ScanConvMode             = DISABLE;                         /* Sequencer disabled (ADC conversion on only 1 channel: channel set on rank 1) */
	AdcHandle1.Init.EOCSelection             = ADC_EOC_SINGLE_CONV;             /* EOC flag picked-up to indicate conversion end */
	AdcHandle1.Init.LowPowerAutoWait         = DISABLE;                         /* Auto-delayed conversion feature disabled */
	AdcHandle1.Init.ContinuousConvMode       = DISABLE;                          /* Continuous mode enabled (automatic conversion restart after each conversion) */
	AdcHandle1.Init.NbrOfConversion          = 1;                               /* Parameter discarded because sequencer is disabled */
	AdcHandle1.Init.DiscontinuousConvMode    = DISABLE;                         /* Parameter discarded because sequencer is disabled */
	AdcHandle1.Init.NbrOfDiscConversion      = 1;                               /* Parameter discarded because sequencer is disabled */
	AdcHandle1.Init.ExternalTrigConv         = ADC_EXTERNALTRIG_T3_TRGO;              /* Software start to trig the 1st conversion manually, without external event */
	AdcHandle1.Init.ExternalTrigConvEdge     = ADC_EXTERNALTRIGCONVEDGE_RISING;   /* Parameter discarded because software trigger chosen */
	AdcHandle1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR; /* ADC DMA circular requested */
	AdcHandle1.Init.Overrun                  = ADC_OVR_DATA_OVERWRITTEN;        /* DR register is overwritten with the last conversion result in case of overrun */
	AdcHandle1.Init.OversamplingMode         = DISABLE;                         /* No oversampling */
	AdcHandle1.Init.BoostMode                = ENABLE;                          /* Enable Boost mode as ADC clock frequency is bigger than 20 MHz */
	/* Initialize ADC peripheral according to the passed parameters */
	if (HAL_ADC_Init(&AdcHandle1) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}


	/* ### - 2 - Start calibration ############################################ */
	if (HAL_ADCEx_Calibration_Start(&AdcHandle1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}

	/* ### - 3 - Channel configuration ######################################## */
	sConfig1.Channel      = ADCH1_CHANNEL;                /* Sampled channel number */
	sConfig1.Rank         = ADC_REGULAR_RANK_1;          /* Rank of sampled channel number ADCH1_CHANNEL */
	sConfig1.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;   /* Sampling time (number of clock cycles unit) */
	sConfig1.SingleDiff   = ADC_SINGLE_ENDED;            /* Single-ended input channel */
	sConfig1.OffsetNumber = ADC_OFFSET_NONE;             /* No offset subtraction */ 
	sConfig1.Offset = 0;                                 /* Parameter discarded because offset correction is disabled */
	if (HAL_ADC_ConfigChannel(&AdcHandle1, &sConfig1) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}

	/* ### - 4 - Start conversion in DMA mode ################################# */
	if (HAL_ADC_Start_DMA(&AdcHandle1,
						(uint32_t *)aADCH1ConvertedData,
						ADC_BUFFER_SIZE
					   ) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}
}

/*
*********************************************************************************************************
*	函 数 名: bsp_StartAdcCH2
*	功能说明: 配置CH2通道的GPIO, ADC, DMA
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_StartAdcCH2(void)
{
	/* ### - 1 - Initialize ADC peripheral #################################### */
	AdcHandle2.Instance          = ADCH2;
	if (HAL_ADC_DeInit(&AdcHandle2) != HAL_OK)
	{
		/* ADC de-initialization Error */
		Error_Handler(__FILE__, __LINE__);
	}

	AdcHandle2.Init.ClockPrescaler           = ADC_CLOCK_SYNC_PCLK_DIV4;        /* Synchronous clock mode, input ADC clock divided by 4*/
	AdcHandle2.Init.Resolution               = ADC_RESOLUTION_12B;              /* 16-bit resolution for converted data */
	AdcHandle2.Init.ScanConvMode             = DISABLE;                         /* Sequencer disabled (ADC conversion on only 1 channel: channel set on rank 1) */
	AdcHandle2.Init.EOCSelection             = ADC_EOC_SINGLE_CONV;             /* EOC flag picked-up to indicate conversion end */
	AdcHandle2.Init.LowPowerAutoWait         = DISABLE;                         /* Auto-delayed conversion feature disabled */
	AdcHandle2.Init.ContinuousConvMode       = DISABLE;                          /* Continuous mode enabled (automatic conversion restart after each conversion) */
	AdcHandle2.Init.NbrOfConversion          = 1;                               /* Parameter discarded because sequencer is disabled */
	AdcHandle2.Init.DiscontinuousConvMode    = DISABLE;                         /* Parameter discarded because sequencer is disabled */
	AdcHandle2.Init.NbrOfDiscConversion      = 1;                               /* Parameter discarded because sequencer is disabled */
	AdcHandle2.Init.ExternalTrigConv         = ADC_EXTERNALTRIG_T3_TRGO;              /* Software start to trig the 1st conversion manually, without external event */
	AdcHandle2.Init.ExternalTrigConvEdge     = ADC_EXTERNALTRIGCONVEDGE_RISING;   /* Parameter discarded because software trigger chosen */
	AdcHandle2.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR; /* ADC DMA circular requested */
	AdcHandle2.Init.Overrun                  = ADC_OVR_DATA_OVERWRITTEN;        /* DR register is overwritten with the last conversion result in case of overrun */
	AdcHandle2.Init.OversamplingMode         = DISABLE;                         /* No oversampling */
	AdcHandle2.Init.BoostMode                = ENABLE;                          /* Enable Boost mode as ADC clock frequency is bigger than 20 MHz */
	/* Initialize ADC peripheral according to the passed parameters */
	if (HAL_ADC_Init(&AdcHandle2) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}


	/* ### - 2 - Start calibration ############################################ */
	if (HAL_ADCEx_Calibration_Start(&AdcHandle2, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}

	/* ### - 3 - Channel configuration ######################################## */
	sConfig2.Channel      = ADCH2_CHANNEL;                /* Sampled channel number */
	sConfig2.Rank         = ADC_REGULAR_RANK_1;          /* Rank of sampled channel number ADCH1_CHANNEL */
	sConfig2.SamplingTime = ADC_SAMPLETIME_1CYCLE_5  ;   /* Sampling time (number of clock cycles unit) */
	sConfig2.SingleDiff   = ADC_SINGLE_ENDED;            /* Single-ended input channel */
	sConfig2.OffsetNumber = ADC_OFFSET_NONE;             /* No offset subtraction */ 
	sConfig2.Offset = 0;                                 /* Parameter discarded because offset correction is disabled */
	if (HAL_ADC_ConfigChannel(&AdcHandle2, &sConfig2) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}

	/* ### - 4 - Start conversion in DMA mode ################################# */
	if (HAL_ADC_Start_DMA(&AdcHandle2,
						(uint32_t *)aADCH2ConvertedData,
						ADC_BUFFER_SIZE
					   ) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}
}

/*
*********************************************************************************************************
*	函 数 名: bsp_StopAdcCH1
*	功能说明: 复位CH1通道配置，停止ADC采集.
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_StopAdcCH1(void)
{
	/* ### - 1 - Initialize ADC peripheral #################################### */
	AdcHandle1.Instance          = ADCH2;
	if (HAL_ADC_DeInit(&AdcHandle2) != HAL_OK)
	{
		/* ADC de-initialization Error */
		Error_Handler(__FILE__, __LINE__);
	}
}

/*
*********************************************************************************************************
*	函 数 名: bsp_StopAdcCH2
*	功能说明: 复位CH2通道配置，停止ADC采集.
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_StopAdcCH2(void)
{
	/* ### - 1 - Initialize ADC peripheral #################################### */
	AdcHandle2.Instance          = ADCH2;
	if (HAL_ADC_DeInit(&AdcHandle2) != HAL_OK)
	{
		/* ADC de-initialization Error */
		Error_Handler(__FILE__, __LINE__);
	}
}

/*
*********************************************************************************************************
*	函 数 名: TIM3_Config
*	功能说明: 配置TIM3作为ADC触发源
*	形    参: _freq : 采样频率，单位Hz
*	返 回 值: 无
*********************************************************************************************************
*/
static void TIM3_Config(uint32_t _freq)
{
	static TIM_HandleTypeDef  htim = {0};
	TIM_MasterConfigTypeDef sMasterConfig = {0};

	htim.Instance = TIM3;
	if (_freq == 0)
	{
		__HAL_RCC_TIM3_CLK_DISABLE();
		HAL_TIM_Base_Stop(&htim);
	}
	else
	{
		__HAL_RCC_TIM3_CLK_ENABLE();
		
		/*##-1- Configure the TIM peripheral #######################################*/
		/* Time base configuration */

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
}

/*
*********************************************************************************************************
*	函 数 名: DSO_StartADC
*	功能说明: 主要完成模拟量GPIO的配置、ADC的配置、定时器的配置以及DMA的配置。
*			- 两个ADC工作在独立模式
*			- 具有相同的外部触发， ADC_EXTERNALTRIG_T4_CC4
*			- TIM1的CC1频率的决定了采样频率
*
*	形    参: _uiBufAddr1 : DMA目标地址，CH1数据存放的缓冲区地址
*			  _uiBufAddr2 : DMA目标地址，CH2数据存放的缓冲区地址
*			  _uiCount : 缓冲区的样本个数 (不是字节数)，两通道同步采集.
*			  _uiFreq : 采样频率， Hz
*	返 回 值: 无
*********************************************************************************************************
*/
void DSO_StartADC(uint16_t **_AdcBuf1, uint16_t **_AdcBuf2, uint32_t _uiFreq)
{			
	*(uint32_t *)_AdcBuf1 = (uint32_t)aADCH1ConvertedData;
	*(uint32_t *)_AdcBuf2 = (uint32_t)aADCH2ConvertedData;
	
	bsp_StartAdcCH1();
	bsp_StartAdcCH2();
	
	/* 配置采样触发定时器，使用TIM1 CC1 */
	DSO_SetSampRate(_uiFreq);	/* 修改采样频率，并启动TIM */
}

/*
*********************************************************************************************************
*	函 数 名: DSO_StopADC
*	功能说明: 关闭ADC采样所有的外设。ADC, DMA, TIM
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void DSO_StopADC(void)
{	
	TIM3_Config(0);
	
	bsp_StopAdcCH1();
	bsp_StopAdcCH1();
}

/*
*********************************************************************************************************
*	函 数 名: PauseADC
*	功能说明: 暂停ADC采样。准备处理数据。保证下次DMA正常启动。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void DSO_PauseADC(void)
{
	TIM3_Config(0);
}

/**
* @brief  ADC MSP Init
* @param  hadc : ADC handle
* @retval None
*/
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
	if (hadc->Instance == ADCH1)
	{	
	  GPIO_InitTypeDef          GPIO_InitStruct;
	  static DMA_HandleTypeDef  DmaHandle1 = {0};
	  
	  /*##-1- Enable peripherals and GPIO Clocks #################################*/
	  /* Enable GPIO clock ****************************************/
	  ADCH1_CHANNEL_GPIO_CLK_ENABLE();
	  /* ADC Periph clock enable */
	  ADCH1_CLK_ENABLE();
	  /* ADC Periph interface clock configuration */
	  __HAL_RCC_ADC_CONFIG(RCC_ADCCLKSOURCE_CLKP);
	  /* Enable DMA clock */
	  CH1_DMA_CLK_ENABLE();
	  
	  /*##- 2- Configure peripheral GPIO #########################################*/
	  /* ADC Channel GPIO pin configuration */
	  GPIO_InitStruct.Pin = ADCH1_CHANNEL_PIN;
	  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	  GPIO_InitStruct.Pull = GPIO_NOPULL;
	  HAL_GPIO_Init(ADCH1_CHANNEL_GPIO_PORT, &GPIO_InitStruct);
	  /*##- 3- Configure DMA #####################################################*/ 

	  /*********************** Configure DMA parameters ***************************/
	  DmaHandle1.Instance                 = CH1_DMA_Stream;
	  DmaHandle1.Init.Request             = CH1_DMA_REQUEST_ADC;
	  DmaHandle1.Init.Direction           = DMA_PERIPH_TO_MEMORY;
	  DmaHandle1.Init.PeriphInc           = DMA_PINC_DISABLE;
	  DmaHandle1.Init.MemInc              = DMA_MINC_ENABLE;
	  DmaHandle1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
	  DmaHandle1.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
	  DmaHandle1.Init.Mode                = DMA_NORMAL;	// DMA_CIRCULAR;
	  DmaHandle1.Init.Priority            = DMA_PRIORITY_MEDIUM;
	  /* Deinitialize  & Initialize the DMA for new transfer */
	  HAL_DMA_DeInit(&DmaHandle1);
	  HAL_DMA_Init(&DmaHandle1);
	  
	  /* Associate the DMA handle */
	  __HAL_LINKDMA(hadc, DMA_Handle, DmaHandle1);

	  /* NVIC configuration for DMA Input data interrupt */
	  HAL_NVIC_SetPriority(CH1_DMA_Stream_IRQn, 1, 0);
	  HAL_NVIC_EnableIRQ(CH1_DMA_Stream_IRQn);  
  }
 else if (hadc->Instance == ADCH2)
 {
	  GPIO_InitTypeDef          GPIO_InitStruct;
	  static DMA_HandleTypeDef  DmaHandle2 = {0};
	  
	  /*##-1- Enable peripherals and GPIO Clocks #################################*/
	  /* Enable GPIO clock ****************************************/
	  ADCH2_CHANNEL_GPIO_CLK_ENABLE();
	  /* ADC Periph clock enable */
	  ADCH2_CLK_ENABLE();
	  /* ADC Periph interface clock configuration */
	  __HAL_RCC_ADC_CONFIG(RCC_ADCCLKSOURCE_CLKP);
	  /* Enable DMA clock */
	  CH2_DMA_CLK_ENABLE();
	  
	  /*##- 2- Configure peripheral GPIO #########################################*/
	  /* ADC Channel GPIO pin configuration */
	  GPIO_InitStruct.Pin = ADCH2_CHANNEL_PIN;
	  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	  GPIO_InitStruct.Pull = GPIO_NOPULL;
	  HAL_GPIO_Init(ADCH2_CHANNEL_GPIO_PORT, &GPIO_InitStruct);
	  /*##- 3- Configure DMA #####################################################*/ 

	  /*********************** Configure DMA parameters ***************************/
	  DmaHandle2.Instance                 = CH2_DMA_Stream;
	  DmaHandle2.Init.Request             = CH2_DMA_REQUEST_ADC;
	  DmaHandle2.Init.Direction           = DMA_PERIPH_TO_MEMORY;
	  DmaHandle2.Init.PeriphInc           = DMA_PINC_DISABLE;
	  DmaHandle2.Init.MemInc              = DMA_MINC_ENABLE;
	  DmaHandle2.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
	  DmaHandle2.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
	  DmaHandle2.Init.Mode                = DMA_NORMAL; // DMA_CIRCULAR;
	  DmaHandle2.Init.Priority            = DMA_PRIORITY_MEDIUM;
	  /* Deinitialize  & Initialize the DMA for new transfer */
	  HAL_DMA_DeInit(&DmaHandle2);
	  HAL_DMA_Init(&DmaHandle2);
	  
	  /* Associate the DMA handle */
	  __HAL_LINKDMA(hadc, DMA_Handle, DmaHandle2);

	  /* NVIC configuration for DMA Input data interrupt */
	  HAL_NVIC_SetPriority(CH2_DMA_Stream_IRQn, 1, 0);
	  HAL_NVIC_EnableIRQ(CH2_DMA_Stream_IRQn);  	 
 }		
}

/**
  * @brief ADC MSP De-Initialization
  *        This function frees the hardware resources used in this example:
  *          - Disable the Peripheral's clock
  *          - Revert GPIO to their default state
  * @param hadc: ADC handle pointer
  * @retval None
  */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef *hadc)
{

	if (hadc->Instance == ADCH1)
	{
	  /*##-1- Reset peripherals ##################################################*/
	  ADCH1_FORCE_RESET();
	  ADCH1_RELEASE_RESET();
	  /* ADC Periph clock disable
	   (automatically reset all ADC instances of the ADC common group) */
	  __HAL_RCC_ADC12_CLK_DISABLE();

	  /*##-2- Disable peripherals and GPIO Clocks ################################*/
	  /* De-initialize the ADC Channel GPIO pin */
	  HAL_GPIO_DeInit(ADCH1_CHANNEL_GPIO_PORT, ADCH1_CHANNEL_PIN);
	}
	else if (hadc->Instance == ADCH2)
	{
	  /*##-1- Reset peripherals ##################################################*/
//	  ADCH2_FORCE_RESET();
//	  ADCH2_RELEASE_RESET();
	  /* ADC Periph clock disable
	   (automatically reset all ADC instances of the ADC common group) */
//	  __HAL_RCC_ADC12_CLK_DISABLE();

	  /*##-2- Disable peripherals and GPIO Clocks ################################*/
	  /* De-initialize the ADC Channel GPIO pin */
	  HAL_GPIO_DeInit(ADCH2_CHANNEL_GPIO_PORT, ADCH2_CHANNEL_PIN);
	}
}

/*
*********************************************************************************************************
*	函 数 名: DSO_SetSampRate
*	功能说明: 修改采样频率. 使用TIM4_CC4触发
*	形    参: freq : 采样频率 单位Hz
*	返 回 值: 无
*********************************************************************************************************
*/
void DSO_SetSampRate(uint32_t _ulFreq)
{
	TIM3_Config(_ulFreq);
}

/*
*********************************************************************************************************
*	函 数 名: CH1_DMA_STREAM_IRQHANDLER
*	功能说明: CH1 DAM中断服务程序
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void CH1_DMA_Stream_IRQHandle(void)
{
	HAL_DMA_IRQHandler(AdcHandle1.DMA_Handle);
}

/*
*********************************************************************************************************
*	函 数 名: CH1_DMA_STREAM_IRQHANDLER
*	功能说明: CH2 DAM中断服务程序,
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void CH2_DMA_Stream_IRQHandle(void)
{
	HAL_DMA_IRQHandler(AdcHandle2.DMA_Handle);
}

/*
*********************************************************************************************************
*	函 数 名: HAL_ADC_ConvHalfCpltCallback
*	功能说明: DAM中断服务程序回调函数，用于cache数据刷新到内存
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
	if (hadc->Instance == ADCH1)
	{
		/* Invalidate Data Cache to get the updated content of the SRAM on the first half of the ADC converted data buffer: 32 bytes */ 
		SCB_InvalidateDCache_by_Addr((uint32_t *)aADCH1ConvertedData,  ADC_BUFFER_SIZE);
	}
	else if (hadc->Instance == ADCH2)
	{
		/* Invalidate Data Cache to get the updated content of the SRAM on the first half of the ADC converted data buffer: 32 bytes */ 
		SCB_InvalidateDCache_by_Addr((uint32_t *)aADCH2ConvertedData,  ADC_BUFFER_SIZE);
	}	
}

/*
*********************************************************************************************************
*	函 数 名: HAL_ADC_ConvCpltCallback
*	功能说明: DAM中断服务程序回调函数，用于cache数据刷新到内存
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
	if (hadc->Instance == ADCH1)
	{
		/* Invalidate Data Cache to get the updated content of the SRAM on the first half of the ADC converted data buffer: 32 bytes */ 
		SCB_InvalidateDCache_by_Addr((uint32_t *)aADCH1ConvertedData + ADC_BUFFER_SIZE,  ADC_BUFFER_SIZE);
	}
	else if (hadc->Instance == ADCH2)
	{
		/* Invalidate Data Cache to get the updated content of the SRAM on the first half of the ADC converted data buffer: 32 bytes */ 
		SCB_InvalidateDCache_by_Addr((uint32_t *)(aADCH2ConvertedData + ADC_BUFFER_SIZE),  ADC_BUFFER_SIZE);
	}	
}

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
