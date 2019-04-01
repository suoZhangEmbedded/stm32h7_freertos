/*
*********************************************************************************************************
*
*	模块名称 : SPI总线驱动
*	文件名称 : bsp_spi_bus1.h
*	版    本 : V1.2
*	说    明 : SPI总线底层驱动。提供SPI配置、收发数据、多设备共享SPI功能.
*	修改记录 :
*		版本号  日期        作者    说明
*       v1.0    2014-10-24 armfly  首版。将串行FLASH、TSC2046、VS1053、AD7705、ADS1256等SPI设备的配置
*									和收发数据的函数进行汇总分类。并解决不同速度的设备间的共享问题。
*		V1.1	2015-02-25 armfly  硬件SPI时，没有开启GPIOB时钟，已解决。
*		V1.2	2015-07-23 armfly  修改 bsp_SPI_Init() 函数，增加开关SPI时钟的语句。规范硬件SPI和软件SPI的宏定义.
*
*	Copyright (C), 2015-2016, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#include "bsp.h"

/*
	安富莱STM32-V7开发板口线分配
	PB3/SPI3_SCK/SPI1_SCK
	PB4/SPI3_MISO/SPI1_MISO
	PB5/SPI3_MOSI/SPI1_MOSI	
*/

/* 
	armfly ： 2018-10-03  SPI 遗留问题待查 (估计HAL库存在缺陷）
	1. DMA模式，发送缓冲区必须是 const 定位在flash才能正常传输。定位在SRAM，则DMA传输异常。
	2. INT模式，32字节一下连续发送正常。48字节以上异常，只能发送一次。第2次会中途终止。
	3. 目前采用普通轮询模式，未发现问题。
	
	测试代码如下：
	bsp_InitSPIBus();
	bsp_InitSPIParam(SPI_BAUDRATEPRESCALER_8, SPI_PHASE_1EDGE, SPI_POLARITY_LOW);
	while (1)
	{
		g_spiLen = 32;		// 数据长度
		bsp_spiTransfer();
		
		bsp_DelayUS(1000);
	}
*/
//#define USE_SPI_DMA
//#define USE_SPI_INT
#define USE_SPI_POLL

#define SPIx						SPI1

#define SPIx_CLK_ENABLE()			__HAL_RCC_SPI1_CLK_ENABLE()

#define DMAx_CLK_ENABLE()			__HAL_RCC_DMA2_CLK_ENABLE()

#define SPIx_FORCE_RESET()			__HAL_RCC_SPI1_FORCE_RESET()
#define SPIx_RELEASE_RESET()		__HAL_RCC_SPI1_RELEASE_RESET()

#define SPIx_SCK_CLK_ENABLE()		__HAL_RCC_GPIOB_CLK_ENABLE()
#define SPIx_SCK_GPIO				GPIOB
#define SPIx_SCK_PIN				GPIO_PIN_3
#define SPIx_SCK_AF					GPIO_AF5_SPI1

#define SPIx_MISO_CLK_ENABLE()		__HAL_RCC_GPIOB_CLK_ENABLE()
#define SPIx_MISO_GPIO				GPIOB
#define SPIx_MISO_PIN 				GPIO_PIN_4
#define SPIx_MISO_AF				GPIO_AF5_SPI1

#define SPIx_MOSI_CLK_ENABLE()		__HAL_RCC_GPIOB_CLK_ENABLE()
#define SPIx_MOSI_GPIO				GPIOB
#define SPIx_MOSI_PIN 				GPIO_PIN_5
#define SPIx_MOSI_AF				GPIO_AF5_SPI1

/* Definition for SPIx's DMA */
#define SPIx_TX_DMA_STREAM               DMA2_Stream3
#define SPIx_RX_DMA_STREAM               DMA2_Stream2

#define SPIx_TX_DMA_REQUEST              DMA_REQUEST_SPI1_TX
#define SPIx_RX_DMA_REQUEST              DMA_REQUEST_SPI1_RX

/* Definition for SPIx's NVIC */
#define SPIx_DMA_TX_IRQn                 DMA2_Stream3_IRQn
#define SPIx_DMA_RX_IRQn                 DMA2_Stream2_IRQn

#define SPIx_DMA_TX_IRQHandler           DMA2_Stream3_IRQHandler
#define SPIx_DMA_RX_IRQHandler           DMA2_Stream2_IRQHandler

#define SPIx_IRQn                        SPI1_IRQn
#define SPIx_IRQHandler                  SPI1_IRQHandler

enum {
	TRANSFER_WAIT,
	TRANSFER_COMPLETE,
	TRANSFER_ERROR
};

static SPI_HandleTypeDef hspi = {0};
static DMA_HandleTypeDef hdma_tx;
static DMA_HandleTypeDef hdma_rx;


//const uint8_t g_spiTxBuf[] = "****SPI - Two Boards communication based on DMA **** SPI Message ********* SPI Message *********";
uint8_t g_spiTxBuf[SPI_BUFFER_SIZE];

ALIGN_32BYTES(uint8_t g_spiRxBuf[SPI_BUFFER_SIZE]);	/* 必须32字节对齐 */
uint32_t g_spiLen;		/* 收发的数据长度 */

/* transfer state */
__IO uint32_t wTransferState = TRANSFER_WAIT;

/* 备份SPI几个关键传输参数，波特率，相位，极性. 如果不同外设切换，需要重新Init SPI参数 */
static uint32_t s_BaudRatePrescaler;
static uint32_t s_CLKPhase;
static uint32_t s_CLKPolarity;

uint8_t g_spi_busy;

/*
*********************************************************************************************************
*	函 数 名: bsp_InitSPIBus1
*	功能说明: 配置SPI总线。 只包括 SCK、 MOSI、 MISO口线的配置。不包括片选CS，也不包括外设芯片特有的INT、BUSY等
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitSPIBus(void)
{	
	g_spi_busy = 0;
	
	bsp_InitSPIParam(SPI_BAUDRATEPRESCALER_8, SPI_PHASE_1EDGE, SPI_POLARITY_LOW);
}

/*
*********************************************************************************************************
*	函 数 名: bsp_InitSPIParam
*	功能说明: 配置SPI总线参数，波特率、
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitSPIParam(uint32_t _BaudRatePrescaler, uint32_t _CLKPhase, uint32_t _CLKPolarity)
{
	/* 提高执行效率，只有在SPI硬件参数发生变化时，才执行HAL_Init */
	if (s_BaudRatePrescaler == _BaudRatePrescaler && s_CLKPhase == _CLKPhase && s_CLKPolarity == _CLKPolarity)
	{		
		return;
	}

	s_BaudRatePrescaler = _BaudRatePrescaler;	
	s_CLKPhase = _CLKPhase;
	s_CLKPolarity = _CLKPolarity;
	
	/*##-1- Configure the SPI peripheral #######################################*/
	/* Set the SPI parameters */
	hspi.Instance               = SPIx;
	hspi.Init.BaudRatePrescaler = _BaudRatePrescaler;
	hspi.Init.Direction         = SPI_DIRECTION_2LINES;
	hspi.Init.CLKPhase          = _CLKPhase;
	hspi.Init.CLKPolarity       = _CLKPolarity;
	hspi.Init.DataSize          = SPI_DATASIZE_8BIT;
	hspi.Init.FirstBit          = SPI_FIRSTBIT_MSB;
	hspi.Init.TIMode            = SPI_TIMODE_DISABLE;
	hspi.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
	hspi.Init.CRCPolynomial     = 7;
	hspi.Init.CRCLength         = SPI_CRC_LENGTH_8BIT;
	hspi.Init.NSS               = SPI_NSS_SOFT;
	hspi.Init.FifoThreshold     = SPI_FIFO_THRESHOLD_01DATA;
	hspi.Init.NSSPMode          = SPI_NSS_PULSE_DISABLE;
	hspi.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;  /* Recommanded setting to avoid glitches */
	hspi.Init.Mode 			 = SPI_MODE_MASTER;

	if (HAL_SPI_Init(&hspi) != HAL_OK)
	{
		/* Initialization Error */
		Error_Handler(__FILE__, __LINE__);
	}	
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *_hspi)
{
	/* 配置 SPI总线GPIO : SCK MOSI MISO */
	{
		GPIO_InitTypeDef  GPIO_InitStruct;
			
		/*##-1- Enable peripherals and GPIO Clocks #################################*/
		SPIx_SCK_CLK_ENABLE();
		SPIx_MISO_CLK_ENABLE();
		SPIx_MOSI_CLK_ENABLE();

		/* Enable SPI1 clock */
		SPIx_CLK_ENABLE();

		/*##-2- Configure peripheral GPIO ##########################################*/  
		/* SPI SCK GPIO pin configuration  */
		GPIO_InitStruct.Pin       = SPIx_SCK_PIN;
		GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull      = GPIO_PULLDOWN;		/* 下拉 */
		GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_MEDIUM;
		GPIO_InitStruct.Alternate = SPIx_SCK_AF;
		HAL_GPIO_Init(SPIx_SCK_GPIO, &GPIO_InitStruct);

		/* SPI MISO GPIO pin configuration  */
		GPIO_InitStruct.Pin = SPIx_MISO_PIN;
		GPIO_InitStruct.Alternate = SPIx_MISO_AF;
		HAL_GPIO_Init(SPIx_MISO_GPIO, &GPIO_InitStruct);

		/* SPI MOSI GPIO pin configuration  */
		GPIO_InitStruct.Pin = SPIx_MOSI_PIN;
		GPIO_InitStruct.Alternate = SPIx_MOSI_AF;
		HAL_GPIO_Init(SPIx_MOSI_GPIO, &GPIO_InitStruct);
	}

	/* 配置DMA和NVIC */
	#ifdef USE_SPI_DMA
	{
		/* Enable DMA clock */
		DMAx_CLK_ENABLE();
		
		/*##-3- Configure the DMA ##################################################*/
		/* Configure the DMA handler for Transmission process */
		hdma_tx.Instance                 = SPIx_TX_DMA_STREAM;
		hdma_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
		hdma_tx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
		hdma_tx.Init.MemBurst            = DMA_MBURST_INC4;	// DMA_MBURST_INC4; DMA_MBURST_SINGLE
		hdma_tx.Init.PeriphBurst         = DMA_PBURST_INC4;	// DMA_PBURST_INC4; DMA_PBURST_SINGLE
		hdma_tx.Init.Request             = SPIx_TX_DMA_REQUEST;
		hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
		hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
		hdma_tx.Init.MemInc              = DMA_MINC_ENABLE;
		hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
		hdma_tx.Init.Mode                = DMA_NORMAL;
		hdma_tx.Init.Priority            = DMA_PRIORITY_LOW;
		HAL_DMA_Init(&hdma_tx);
		__HAL_LINKDMA(_hspi, hdmatx, hdma_tx);	 /* Associate the initialized DMA handle to the the SPI handle */

		/* Configure the DMA handler for Transmission process */
		hdma_rx.Instance                 = SPIx_RX_DMA_STREAM;
		hdma_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
		hdma_rx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
		hdma_rx.Init.MemBurst            = DMA_MBURST_INC4;	// DMA_MBURST_INC4;  DMA_MBURST_SINGLE
		hdma_rx.Init.PeriphBurst         = DMA_PBURST_INC4;	// DMA_PBURST_INC4;  DMA_PBURST_SINGLE
		hdma_rx.Init.Request             = SPIx_RX_DMA_REQUEST;
		hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
		hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
		hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
		hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
		hdma_rx.Init.Mode                = DMA_NORMAL;
		hdma_rx.Init.Priority            = DMA_PRIORITY_HIGH;
		HAL_DMA_Init(&hdma_rx);    
	   __HAL_LINKDMA(_hspi, hdmarx, hdma_rx);	/* Associate the initialized DMA handle to the the SPI handle */

		/* NVIC configuration for DMA transfer complete interrupt (SPI1_TX) */
		HAL_NVIC_SetPriority(SPIx_DMA_TX_IRQn, 1, 1);
		HAL_NVIC_EnableIRQ(SPIx_DMA_TX_IRQn);
		
		/* NVIC configuration for DMA transfer complete interrupt (SPI1_RX) */
		HAL_NVIC_SetPriority(SPIx_DMA_RX_IRQn, 1, 0);
		HAL_NVIC_EnableIRQ(SPIx_DMA_RX_IRQn);
		
		/* NVIC configuration for SPI transfer complete interrupt (SPI1) */
		HAL_NVIC_SetPriority(SPIx_IRQn, 1, 0);
		HAL_NVIC_EnableIRQ(SPIx_IRQn);
	}
	#endif

	#ifdef USE_SPI_INT
		/* NVIC configuration for SPI transfer complete interrupt (SPI1) */
		HAL_NVIC_SetPriority(SPIx_IRQn, 1, 0);
		HAL_NVIC_EnableIRQ(SPIx_IRQn);
	#endif
}
	
/*
*********************************************************************************************************
*	函 数 名: bsp_spiTransfer
*	功能说明: 向SPI总线发送1个或多个字节
*	形    参:  g_spi_TxBuf : 发送的数据
*		      g_spi_RxBuf  ：收发的数据长度
*			  _TxRxLen ：接收的数据缓冲区
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_spiTransfer(void)
{
	if (g_spiLen > SPI_BUFFER_SIZE)
	{
		return;
	}
	
#ifdef USE_SPI_DMA
	wTransferState == TRANSFER_WAIT;
	
	while (hspi.State != HAL_SPI_STATE_READY);

	if(HAL_SPI_TransmitReceive_DMA(&hspi, (uint8_t*)g_spiTxBuf, (uint8_t *)g_spiRxBuf, g_spiLen) != HAL_OK)	
	{
		Error_Handler(__FILE__, __LINE__);
	}
	
	while (wTransferState == TRANSFER_WAIT)
	{
		;
	}
	
	/* Invalidate cache prior to access by CPU */
	SCB_InvalidateDCache_by_Addr ((uint32_t *)g_spiRxBuf, SPI_BUFFER_SIZE);
#endif
	
#ifdef USE_SPI_INT
	wTransferState = TRANSFER_WAIT;
	
	while (hspi.State != HAL_SPI_STATE_READY);

	if(HAL_SPI_TransmitReceive_IT(&hspi, (uint8_t*)g_spiTxBuf, (uint8_t *)g_spiRxBuf, g_spiLen) != HAL_OK)	
	{
		Error_Handler(__FILE__, __LINE__);
	}
	
	while (wTransferState == TRANSFER_WAIT)
	{
		;
	}
#endif
	
#ifdef USE_SPI_POLL
	if(HAL_SPI_TransmitReceive(&hspi, (uint8_t*)g_spiTxBuf, (uint8_t *)g_spiRxBuf, g_spiLen, 1000000) != HAL_OK)	
	{
		Error_Handler(__FILE__, __LINE__);
	}	
#endif
}

/**
  * @brief  TxRx Transfer completed callback.
  * @param  hspi: SPI handle
  * @note   This example shows a simple way to report end of DMA TxRx transfer, and 
  *         you can add your own implementation. 
  * @retval None
  */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
	wTransferState = TRANSFER_COMPLETE;
}

/**
  * @brief  SPI error callbacks.
  * @param  hspi: SPI handle
  * @note   This example shows a simple way to report transfer error, and you can
  *         add your own implementation.
  * @retval None
  */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
	wTransferState = TRANSFER_ERROR;
}

/*
*********************************************************************************************************
*	函 数 名: bsp_SpiBusEnter
*	功能说明: 占用SPI总线
*	形    参: 无
*	返 回 值: 0 表示不忙  1表示忙
*********************************************************************************************************
*/
void bsp_SpiBusEnter(void)
{
	g_spi_busy = 1;
}

/*
*********************************************************************************************************
*	函 数 名: bsp_SpiBusExit
*	功能说明: 释放占用的SPI总线
*	形    参: 无
*	返 回 值: 0 表示不忙  1表示忙
*********************************************************************************************************
*/
void bsp_SpiBusExit(void)
{
	g_spi_busy = 0;
}

/*
*********************************************************************************************************
*	函 数 名: bsp_SpiBusBusy
*	功能说明: 判断SPI总线忙。方法是检测其他SPI芯片的片选信号是否为1
*	形    参: 无
*	返 回 值: 0 表示不忙  1表示忙
*********************************************************************************************************
*/
uint8_t bsp_SpiBusBusy(void)
{
	return g_spi_busy;
}


#ifdef USE_SPI_INT
	/**
	  * @brief  This function handles SPIx interrupt request.
	  * @param  None
	  * @retval None
	  */
	void SPIx_IRQHandler(void)
	{
		HAL_SPI_IRQHandler(&hspi);
	}	
#endif

#ifdef USE_SPI_DMA
	/**
	  * @brief  This function handles DMA Rx interrupt request.
	  * @param  None
	  * @retval None
	  */
	void SPIx_DMA_RX_IRQHandler(void)
	{
		HAL_DMA_IRQHandler(hspi.hdmarx);
	}

	/**
	  * @brief  This function handles DMA Tx interrupt request.
	  * @param  None
	  * @retval None
	  */
	void SPIx_DMA_TX_IRQHandler(void)
	{
		HAL_DMA_IRQHandler(hspi.hdmatx);
	}
	
	/**
	  * @brief  This function handles SPIx interrupt request.
	  * @param  None
	  * @retval None
	  */
	void SPIx_IRQHandler(void)
	{
		HAL_SPI_IRQHandler(&hspi);
	}	
#endif
	
/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
