/*
*********************************************************************************************************
*
*	模块名称 : STM32H7内部LCD驱动程序
*	文件名称 : bsp_tft_h7.c
*	版    本 : V1.0
*	说    明 : STM32F429 内部LCD接口的硬件配置程序。
*	修改记录 :
*		版本号  日期       作者    说明
*		V1.0    2014-05-05 armfly 增加 STM32F429 内部LCD接口； 基于ST的例子更改，不要背景层和前景层定义，直接
*							      用 LTDC_Layer1 、 LTDC_Layer2, 这是2个结构体指针
*		V1.1	2015-11-19 armfly 
*						1. 绘图函数替换为DMA2D硬件驱动，提高绘图效率
*						2. 统一多种面板的配置函数，自动识别面板类型
*
*	Copyright (C), 2015-2020, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#include "bsp.h"
#include "fonts.h"

typedef uint32_t LCD_COLOR;

/* 偏移地址计算公式:
   Maximum width x Maximum Length x Maximum Pixel size (RGB565)，单位字节
   => 800 x 480 x 2 =  768000 */
#define BUFFER_OFFSET			SDRAM_LCD_SIZE	// (uint32_t)(g_LcdHeight * g_LcdWidth * 2)

/* 宏定义 */
#define LCDH7_FRAME_BUFFER		SDRAM_LCD_BUF1

/* 变量 */
static LTDC_HandleTypeDef   hltdc_F;
static DMA2D_HandleTypeDef  hdma2d;
uint32_t s_CurrentFrameBuffer;
uint8_t s_CurrentLayer;

/* 函数 */
static void LCDH7_ConfigLTDC(void);

void LCDH7_LayerInit(void);
void LCDH7_SetPixelFormat(uint32_t PixelFormat);

static void LCDH7_InitDMA2D(void);
static inline uint32_t LCD_LL_GetPixelformat(uint32_t LayerIndex);
void DMA2D_CopyBuffer(uint32_t LayerIndex, void * pSrc, void * pDst, uint32_t xSize, uint32_t ySize, uint32_t OffLineSrc, uint32_t OffLineDst);
static void DMA2D_FillBuffer(uint32_t LayerIndex, void * pDst, uint32_t xSize, uint32_t ySize, uint32_t OffLine, uint32_t ColorIndex);

/*
*********************************************************************************************************
*	函 数 名: LCDH7_InitHard
*	功能说明: 初始化LCD
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void LCDH7_InitHard(void)
{
	LCDH7_ConfigLTDC();			/* 配置429 CPU内部LTDC */
	LCDH7_InitDMA2D();          /* 使能DMA2D */
	LCDH7_SetLayer(LCD_LAYER_1);/* 使用的图层1 */	
	LCDH7_QuitWinMode();
}

/*
*********************************************************************************************************
*	函 数 名: LCDH7_ConfigLTDC
*	功能说明: 配置LTDC
*	形    参: 无
*	返 回 值: 无
*   笔    记:
*       LCD_TFT 同步时序配置（整理自官方做的一个截图，言简意赅）：
*       ----------------------------------------------------------------------------
*    
*                                                 Total Width
*                             <--------------------------------------------------->
*                       Hsync width HBP             Active Width                HFP
*                             <---><--><--------------------------------------><-->
*                         ____    ____|_______________________________________|____ 
*                             |___|   |                                       |    |
*                                     |                                       |    |
*                         __|         |                                       |    |
*            /|\    /|\  |            |                                       |    |
*             | VSYNC|   |            |                                       |    |
*             |Width\|/  |__          |                                       |    |
*             |     /|\     |         |                                       |    |
*             |  VBP |      |         |                                       |    |
*             |     \|/_____|_________|_______________________________________|    |
*             |     /|\     |         | / / / / / / / / / / / / / / / / / / / |    |
*             |      |      |         |/ / / / / / / / / / / / / / / / / / / /|    |
*    Total    |      |      |         |/ / / / / / / / / / / / / / / / / / / /|    |
*    Heigh    |      |      |         |/ / / / / / / / / / / / / / / / / / / /|    |
*             |Active|      |         |/ / / / / / / / / / / / / / / / / / / /|    |
*             |Heigh |      |         |/ / / / / / Active Display Area / / / /|    |
*             |      |      |         |/ / / / / / / / / / / / / / / / / / / /|    |
*             |      |      |         |/ / / / / / / / / / / / / / / / / / / /|    |
*             |      |      |         |/ / / / / / / / / / / / / / / / / / / /|    |
*             |      |      |         |/ / / / / / / / / / / / / / / / / / / /|    |
*             |      |      |         |/ / / / / / / / / / / / / / / / / / / /|    |
*             |     \|/_____|_________|_______________________________________|    |
*             |     /|\     |                                                      |
*             |  VFP |      |                                                      |
*            \|/    \|/_____|______________________________________________________|
*            
*     
*     每个LCD设备都有自己的同步时序值：
*     Horizontal Synchronization (Hsync) 
*     Horizontal Back Porch (HBP)       
*     Active Width                      
*     Horizontal Front Porch (HFP)     
*   
*     Vertical Synchronization (Vsync)  
*     Vertical Back Porch (VBP)         
*     Active Heigh                       
*     Vertical Front Porch (VFP)         
*     
*     LCD_TFT 窗口水平和垂直的起始以及结束位置 :
*     ----------------------------------------------------------------
*   
*     HorizontalStart = (Offset_X + Hsync + HBP);
*     HorizontalStop  = (Offset_X + Hsync + HBP + Window_Width - 1); 
*     VarticalStart   = (Offset_Y + Vsync + VBP);
*     VerticalStop    = (Offset_Y + Vsync + VBP + Window_Heigh - 1);
*
*********************************************************************************************************
*/
static void LCDH7_ConfigLTDC(void)
{
	/* 配置LCD相关的GPIO */
	{
		/* GPIOs Configuration */
		/*
		+------------------------+-----------------------+----------------------------+
		+                       LCD pins assignment                                   +
		+------------------------+-----------------------+----------------------------+
		|  LCDH7_TFT R0 <-> PI.15  |  LCDH7_TFT G0 <-> PJ.07 |  LCDH7_TFT B0 <-> PJ.12      |
		|  LCDH7_TFT R1 <-> PJ.00  |  LCDH7_TFT G1 <-> PJ.08 |  LCDH7_TFT B1 <-> PJ.13      |
		|  LCDH7_TFT R2 <-> PJ.01  |  LCDH7_TFT G2 <-> PJ.09 |  LCDH7_TFT B2 <-> PJ.14      |
		|  LCDH7_TFT R3 <-> PJ.02  |  LCDH7_TFT G3 <-> PJ.10 |  LCDH7_TFT B3 <-> PJ.15      |
		|  LCDH7_TFT R4 <-> PJ.03  |  LCDH7_TFT G4 <-> PJ.11 |  LCDH7_TFT B4 <-> PK.03      |
		|  LCDH7_TFT R5 <-> PJ.04  |  LCDH7_TFT G5 <-> PK.00 |  LCDH7_TFT B5 <-> PK.04      |
		|  LCDH7_TFT R6 <-> PJ.05  |  LCDH7_TFT G6 <-> PK.01 |  LCDH7_TFT B6 <-> PK.05      |
		|  LCDH7_TFT R7 <-> PJ.06  |  LCDH7_TFT G7 <-> PK.02 |  LCDH7_TFT B7 <-> PK.06      |
		-------------------------------------------------------------------------------
		|  LCDH7_TFT HSYNC <-> PI.12  | LCDTFT VSYNC <->  PI.13 |
		|  LCDH7_TFT CLK   <-> PI.14  | LCDH7_TFT DE   <->  PK.07 |
		-----------------------------------------------------
		*/		
		GPIO_InitTypeDef GPIO_Init_Structure;

		/*##-1- Enable peripherals and GPIO Clocks #################################*/  
		/* 使能LTDC时钟 */
		__HAL_RCC_LTDC_CLK_ENABLE();
		/* 使能GPIO时钟 */
		__HAL_RCC_GPIOI_CLK_ENABLE();
		__HAL_RCC_GPIOJ_CLK_ENABLE();
		__HAL_RCC_GPIOK_CLK_ENABLE();

		/* GPIOI 配置 */
		GPIO_Init_Structure.Pin       = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15; 
		GPIO_Init_Structure.Mode      = GPIO_MODE_AF_PP;
		GPIO_Init_Structure.Pull      = GPIO_NOPULL;
		GPIO_Init_Structure.Speed     = GPIO_SPEED_FREQ_HIGH;
		GPIO_Init_Structure.Alternate = GPIO_AF14_LTDC;  
		HAL_GPIO_Init(GPIOI, &GPIO_Init_Structure);

		/* GPIOJ 配置 */  
		GPIO_Init_Structure.Pin       = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | \
									  GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | \
									  GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | \
									  GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15; 
		GPIO_Init_Structure.Mode      = GPIO_MODE_AF_PP;
		GPIO_Init_Structure.Pull      = GPIO_NOPULL;
		GPIO_Init_Structure.Speed     = GPIO_SPEED_FREQ_HIGH;
		GPIO_Init_Structure.Alternate = GPIO_AF14_LTDC;  
		HAL_GPIO_Init(GPIOJ, &GPIO_Init_Structure);  

		/* GPIOK 配置 */  
		GPIO_Init_Structure.Pin       = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | \
									  GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7; 
		GPIO_Init_Structure.Mode      = GPIO_MODE_AF_PP;
		GPIO_Init_Structure.Pull      = GPIO_NOPULL;
		GPIO_Init_Structure.Speed     = GPIO_SPEED_FREQ_HIGH;
		GPIO_Init_Structure.Alternate = GPIO_AF14_LTDC;  
		HAL_GPIO_Init(GPIOK, &GPIO_Init_Structure);  	
	}
	
	/*##-2- LTDC初始化 #############################################################*/  
	{	
		LTDC_LayerCfgTypeDef      pLayerCfg;
		uint16_t Width, Height, HSYNC_W, HBP, HFP, VSYNC_W, VBP, VFP;
		RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;

		/* 支持6种面板 */
		switch (g_LcdType)
		{
			case LCD_35_480X320:	/* 3.5寸 480 * 320 */	
				Width = 480;
				Height = 272;
				HSYNC_W = 10;
				HBP = 20;
				HFP = 20;
				VSYNC_W = 20;
				VBP = 20;
				VFP = 20;
				break;
			
			case LCD_43_480X272:		/* 4.3寸 480 * 272 */			
				Width = 480;
				Height = 272;

				HSYNC_W = 40;
				HBP = 2;
				HFP = 2;
				VSYNC_W = 9;
				VBP = 2;
				VFP = 2;
		
				/* LCD 时钟配置 */
				/* PLL3_VCO Input = HSE_VALUE/PLL3M = 25MHz/5 = 5MHz */
				/* PLL3_VCO Output = PLL3_VCO Input * PLL3N = 5MHz * 48 = 240MHz */
				/* PLLLCDCLK = PLL3_VCO Output/PLL3R = 240 / 10 = 24MHz */
				/* LTDC clock frequency = PLLLCDCLK = 24MHz */
				/*
					刷新率 = 24MHz /((Width + HSYNC_W  + HBP  + HFP)*(Height + VSYNC_W +  VBP  + VFP))
                   		   = 24000000/((480 + 40  + 2  + 2)*(272 + 9 +  2  + 2)) 
			               = 24000000/(524*285)
                           = 160Hz	

					当前这个配置方便用户使用PLL3Q输出的48MHz时钟供USB使用。
			    */
				PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
				PeriphClkInitStruct.PLL3.PLL3M = 5;
				PeriphClkInitStruct.PLL3.PLL3N = 48;
				PeriphClkInitStruct.PLL3.PLL3P = 2;
				PeriphClkInitStruct.PLL3.PLL3Q = 5;
				PeriphClkInitStruct.PLL3.PLL3R = 10;				
				HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);     			
				break;
			
			case LCD_50_480X272:		/* 5.0寸 480 * 272 */
				Width = 480;
				Height = 272;
			
				HSYNC_W = 40;
				HBP = 2;
				HFP = 2;
				VSYNC_W = 9;
				VBP = 2;
				VFP = 2;			
				break;
			
			case LCD_50_800X480:		/* 5.0寸 800 * 480 */
			case LCD_70_800X480:		/* 7.0寸 800 * 480 */					
				Width = 800;
				Height = 480;

				HSYNC_W = 96;	/* =10时，显示错位，20时部分屏可以的,80时全部OK */
				HBP = 10;
				HFP = 10;
				VSYNC_W = 2;
				VBP = 10;
				VFP = 10;			

				/* LCD 时钟配置 */
				/* PLL3_VCO Input = HSE_VALUE/PLL3M = 25MHz/5 = 5MHz */
				/* PLL3_VCO Output = PLL3_VCO Input * PLL3N = 5MHz * 48 = 240MHz */
				/* PLLLCDCLK = PLL3_VCO Output/PLL3R = 240 / 10 = 24MHz */
				/* LTDC clock frequency = PLLLCDCLK = 24MHz */
				/*
					刷新率 = 24MHz /((Width + HSYNC_W  + HBP  + HFP)*(Height + VSYNC_W +  VBP  + VFP))
                   		   = 24000000/((800 + 96  + 10  + 10)*(480 + 2 +  10  + 10)) 
			               = 24000000/(916*502)
                           = 52Hz	
			
					根据需要可以加大，100Hz刷新率完全没问题，设置PeriphClkInitStruct.PLL3.PLL3N = 100即可
					此时的LTDC时钟是50MHz
					刷新率 = 50MHz /(（Width + HSYNC_W  + HBP  + HFP ）*(Height + VSYNC_W +  VBP  +VFP  )) 
					       = 5000000/(916*502) 
					       = 108.7Hz

					当前这个配置方便用户使用PLL3Q输出的48MHz时钟供USB使用。
			    */ 
				PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
				PeriphClkInitStruct.PLL3.PLL3M = 5;
				PeriphClkInitStruct.PLL3.PLL3N = 48;
				PeriphClkInitStruct.PLL3.PLL3P = 2;
				PeriphClkInitStruct.PLL3.PLL3Q = 5;
				PeriphClkInitStruct.PLL3.PLL3R = 10; 
				HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);     			
				break;
			
			case LCD_70_1024X600:		/* 7.0寸 1024 * 600 */
				/* 实测像素时钟 = 53.7M */
				Width = 1024;
				Height = 600;

				HSYNC_W = 2;	/* =10时，显示错位，20时部分屏可以的,80时全部OK */
				HBP = 157;
				HFP = 160;
				VSYNC_W = 2;
				VBP = 20;
				VFP = 12;		
			
				PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
				PeriphClkInitStruct.PLL3.PLL3M = 5;
				PeriphClkInitStruct.PLL3.PLL3N = 48;
				PeriphClkInitStruct.PLL3.PLL3P = 2;
				PeriphClkInitStruct.PLL3.PLL3Q = 5;
				PeriphClkInitStruct.PLL3.PLL3R = 10;
				HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct); 			
				break;
			
			default:
				Width = 800;
				Height = 480;

				HSYNC_W = 80;	/* =10时，显示错位，20时部分屏可以的,80时全部OK */
				HBP = 10;
				HFP = 10;
				VSYNC_W = 10;
				VBP = 10;
				VFP = 10;		
			
				/* LCD 时钟配置 */
				/* PLL3_VCO Input = HSE_VALUE/PLL3M = 25MHz/5 = 5MHz */
				/* PLL3_VCO Output = PLL3_VCO Input * PLL3N = 5MHz * 48 = 240MHz */
				/* PLLLCDCLK = PLL3_VCO Output/PLL3R = 240 / 10 = 24MHz */
				/* LTDC clock frequency = PLLLCDCLK = 24MHz */
				/*
					刷新率 = 24MHz /((Width + HSYNC_W  + HBP  + HFP)*(Height + VSYNC_W +  VBP  + VFP))
                   		   = 24000000/((800 + 96  + 10  + 10)*(480 + 2 +  10  + 10)) 
			               = 24000000/(916*502)
                           = 52Hz

					根据需要可以加大，100Hz刷新率完全没问题，设置PeriphClkInitStruct.PLL3.PLL3N = 100即可
					此时的LTDC时钟是50MHz
					刷新率 = 50MHz /(（Width + HSYNC_W  + HBP  + HFP ）*(Height + VSYNC_W +  VBP  +VFP  )) 
					       = 5000000/(916*502) 
					       = 108.7Hz

					当前这个配置方便用户使用PLL3Q输出的48MHz时钟供USB使用。
			    */ 
				PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
				PeriphClkInitStruct.PLL3.PLL3M = 5;
				PeriphClkInitStruct.PLL3.PLL3N = 48;
				PeriphClkInitStruct.PLL3.PLL3P = 2;
				PeriphClkInitStruct.PLL3.PLL3Q = 5;
				PeriphClkInitStruct.PLL3.PLL3R = 10;  
				HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct); 			
				break;
		}		

		g_LcdHeight = Height;
		g_LcdWidth = Width;
		
		/* 配置信号极性 */	
		hltdc_F.Init.HSPolarity = LTDC_HSPOLARITY_AL;	/* HSYNC 低电平有效 */
		hltdc_F.Init.VSPolarity = LTDC_VSPOLARITY_AL; 	/* VSYNC 低电平有效 */
		hltdc_F.Init.DEPolarity = LTDC_DEPOLARITY_AL; 	/* DE 低电平有效 */
		hltdc_F.Init.PCPolarity = LTDC_PCPOLARITY_IPC;

		/* 时序配置 */    
		hltdc_F.Init.HorizontalSync = (HSYNC_W - 1);
		hltdc_F.Init.VerticalSync = (VSYNC_W - 1);
		hltdc_F.Init.AccumulatedHBP = (HSYNC_W + HBP - 1);
		hltdc_F.Init.AccumulatedVBP = (VSYNC_W + VBP - 1);  
		hltdc_F.Init.AccumulatedActiveH = (Height + VSYNC_W + VBP - 1);
		hltdc_F.Init.AccumulatedActiveW = (Width + HSYNC_W + HBP - 1);
		hltdc_F.Init.TotalHeigh = (Height + VSYNC_W + VBP + VFP - 1);
		hltdc_F.Init.TotalWidth = (Width + HSYNC_W + HBP + HFP - 1); 

		/* 配置背景层颜色 */
		hltdc_F.Init.Backcolor.Blue = 0;
		hltdc_F.Init.Backcolor.Green = 0;
		hltdc_F.Init.Backcolor.Red = 0;

		hltdc_F.Instance = LTDC;

		/* 开始配置图层 ------------------------------------------------------*/
		/* 窗口显示区设置 */ 
		pLayerCfg.WindowX0 = 0;
		pLayerCfg.WindowX1 = Width;
		pLayerCfg.WindowY0 = 0;
		pLayerCfg.WindowY1 = Height;

		/* 配置颜色格式 */ 
		pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;

		/* 显存地址 */
		pLayerCfg.FBStartAdress = LCDH7_FRAME_BUFFER;	

		/* Alpha常数 (255 表示完全不透明) */
		pLayerCfg.Alpha = 255;

		/* 无背景色 */
		pLayerCfg.Alpha0 = 0; 	/* 完全透明 */
		pLayerCfg.Backcolor.Blue = 0;
		pLayerCfg.Backcolor.Green = 0;
		pLayerCfg.Backcolor.Red = 0;

		/* 配置图层混合因数 */
		pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
		pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;

		/* 配置行列大小 */
		pLayerCfg.ImageWidth  = Width;
		pLayerCfg.ImageHeight = Height;

		/* 配置LTDC  */  
		if (HAL_LTDC_Init(&hltdc_F) != HAL_OK)
		{
			/* 初始化错误 */
			Error_Handler(__FILE__, __LINE__);
		}

		/* 配置图层1 */
		if (HAL_LTDC_ConfigLayer(&hltdc_F, &pLayerCfg, LTDC_LAYER_1) != HAL_OK)
		{
			/* 初始化错误 */
			Error_Handler(__FILE__, __LINE__);
		}  
		
		#if 0
		/* 配置图层2 */
		if (HAL_LTDC_ConfigLayer(&hltdc_F, &pLayerCfg, LTDC_LAYER_2) != HAL_OK)
		{
			/* 初始化错误 */
			Error_Handler(__FILE__, __LINE__);
		} 
        #endif 		
	}  

#if	1
	HAL_NVIC_SetPriority(LTDC_IRQn, 0xE, 0);
	HAL_NVIC_EnableIRQ(LTDC_IRQn);
#endif	
}

/*
*********************************************************************************************************
*	函 数 名: LCDH7_GetChipDescribe
*	功能说明: 读取LCD驱动芯片的描述符号，用于显示
*	形    参: char *_str : 描述符字符串填入此缓冲区
*	返 回 值: 无
*********************************************************************************************************
*/
void LCDH7_GetChipDescribe(char *_str)
{
	strcpy(_str, "STM32H743");
}

/*
*********************************************************************************************************
*	函 数 名: LCDH7_SetLayer
*	功能说明: 切换层。只是更改程序变量，以便于后面的代码更改相关寄存器。硬件支持2层。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void LCDH7_SetLayer(uint8_t _ucLayer)
{
	if (_ucLayer == LCD_LAYER_1)
	{
		s_CurrentFrameBuffer = LCDH7_FRAME_BUFFER;
		s_CurrentLayer = LCD_LAYER_1;
	}
	else if (_ucLayer == LCD_LAYER_2)
	{
		s_CurrentFrameBuffer = LCDH7_FRAME_BUFFER + BUFFER_OFFSET;
		s_CurrentLayer = LCD_LAYER_2;
	}
}

/*
*********************************************************************************************************
*	函 数 名: LCDH7_SetTransparency
*	功能说明: 配置当前层的透明属性
*	形    参: 透明度， 值域： 0x00 - 0xFF
*	返 回 值: 无
*********************************************************************************************************
*/
void LCDH7_SetTransparency(uint8_t transparency)
{
	HAL_LTDC_SetAlpha(&hltdc_F, transparency, s_CurrentLayer);	/* 立即刷新 */
}

/*
*********************************************************************************************************
*	函 数 名: LCDH7_SetPixelFormat
*	功能说明: 配置当前层的像素格式
*	形    参: 像素格式:
*                      LTDC_PIXEL_FORMAT_ARGB8888
*                      LTDC_PIXEL_FORMAT_ARGB8888_RGB888
*                      LTDC_PIXEL_FORMAT_ARGB8888_RGB565
*                      LTDC_PIXEL_FORMAT_ARGB8888_ARGB1555
*                      LTDC_PIXEL_FORMAT_ARGB8888_ARGB4444
*                      LTDC_PIXEL_FORMAT_ARGB8888_L8
*                      LTDC_PIXEL_FORMAT_ARGB8888_AL44
*                      LTDC_PIXEL_FORMAT_ARGB8888_AL88
*	返 回 值: 无
*********************************************************************************************************
*/
void LCDH7_SetPixelFormat(uint32_t PixelFormat)
{
	HAL_LTDC_SetPixelFormat(&hltdc_F, PixelFormat, s_CurrentLayer);
}

/*
*********************************************************************************************************
*	函 数 名: LCDH7_SetDispWin
*	功能说明: 设置显示窗口，进入窗口显示模式。
*	形    参:  
*		_usX : 水平坐标
*		_usY : 垂直坐标
*		_usHeight: 窗口高度
*		_usWidth : 窗口宽度
*	返 回 值: 无
*********************************************************************************************************
*/
void LCDH7_SetDispWin(uint16_t _usX, uint16_t _usY, uint16_t _usHeight, uint16_t _usWidth)
{
	HAL_LTDC_SetWindowSize_NoReload(&hltdc_F, _usWidth, _usHeight, s_CurrentLayer);	/* 不立即更新 */
	HAL_LTDC_SetWindowPosition(&hltdc_F, _usX, _usY, s_CurrentLayer);		/* 立即更新 */
}

/*
*********************************************************************************************************
*	函 数 名: LCDH7_QuitWinMode
*	功能说明: 退出窗口显示模式，变为全屏显示模式
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void LCDH7_QuitWinMode(void)
{
	HAL_LTDC_SetWindowSize_NoReload(&hltdc_F, g_LcdWidth, g_LcdHeight, s_CurrentLayer);	/* 不立即更新 */
	HAL_LTDC_SetWindowPosition(&hltdc_F, 0, 0, s_CurrentLayer);		/* 立即更新 */	
}

/*
*********************************************************************************************************
*	函 数 名: LCDH7_DispOn
*	功能说明: 打开显示
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void LCDH7_DispOn(void)
{
}

/*
*********************************************************************************************************
*	函 数 名: LCDH7_DispOff
*	功能说明: 关闭显示
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void LCDH7_DispOff(void)
{
}

/*
*********************************************************************************************************
*	函 数 名: LCDH7_ClrScr
*	功能说明: 根据输入的颜色值清屏
*	形    参: _usColor : 背景色
*	返 回 值: 无
*********************************************************************************************************
*/
void LCDH7_ClrScr(uint16_t _usColor)
{
	LCDH7_FillRect(0, 0, g_LcdHeight, g_LcdWidth, _usColor);
}

/*
*********************************************************************************************************
*	函 数 名: LCDH7_PutPixel
*	功能说明: 画1个像素
*	形    参:
*			_usX,_usY : 像素坐标
*			_usColor  : 像素颜色 ( RGB = 565 格式)
*	返 回 值: 无
*********************************************************************************************************
*/
void LCDH7_PutPixel(uint16_t _usX, uint16_t _usY, uint16_t _usColor)
{
	uint16_t *p;
	uint32_t index = 0;
	
	p = (uint16_t *)s_CurrentFrameBuffer;
		
	if (g_LcdDirection == 0)		/* 横屏 */
	{
		index = (uint32_t)_usY * g_LcdWidth + _usX;
	}
	else if (g_LcdDirection == 1)	/* 横屏180°*/
	{
		index = (uint32_t)(g_LcdHeight - _usY - 1) * g_LcdWidth + (g_LcdWidth - _usX - 1);
	}
	else if (g_LcdDirection == 2)	/* 竖屏 */
	{
		index = (uint32_t)(g_LcdWidth - _usX - 1) * g_LcdHeight + _usY;
	}
	else if (g_LcdDirection == 3)	/* 竖屏180° */
	{
		index = (uint32_t)_usX * g_LcdHeight + g_LcdHeight - _usY - 1;
	}	
	
	if (index < g_LcdHeight * g_LcdWidth)
	{
		p[index] = _usColor;
	}
}

/*
*********************************************************************************************************
*	函 数 名: LCDH7_GetPixel
*	功能说明: 读取1个像素
*	形    参:
*			_usX,_usY : 像素坐标
*			_usColor  : 像素颜色
*	返 回 值: RGB颜色值
*********************************************************************************************************
*/
uint16_t LCDH7_GetPixel(uint16_t _usX, uint16_t _usY)
{
	uint16_t usRGB;
	uint16_t *p;
	uint32_t index = 0;
	
	p = (uint16_t *)s_CurrentFrameBuffer;

	if (g_LcdDirection == 0)		/* 横屏 */
	{
		index = (uint32_t)_usY * g_LcdWidth + _usX;
	}
	else if (g_LcdDirection == 1)	/* 横屏180°*/
	{
		index = (uint32_t)(g_LcdHeight - _usY - 1) * g_LcdWidth + (g_LcdWidth - _usX - 1);
	}
	else if (g_LcdDirection == 2)	/* 竖屏 */
	{
		index = (uint32_t)(g_LcdWidth - _usX - 1) * g_LcdHeight + _usY;
	}
	else if (g_LcdDirection == 3)	/* 竖屏180° */
	{
		index = (uint32_t)_usX * g_LcdHeight + g_LcdHeight - _usY - 1;
	}
	
	usRGB = p[index];

	return usRGB;
}


/*
*********************************************************************************************************
*	函 数 名: LCDH7_DrawLine
*	功能说明: 采用 Bresenham 算法，在2点间画一条直线。
*	形    参:
*			_usX1, _usY1 : 起始点坐标
*			_usX2, _usY2 : 终止点Y坐标
*			_usColor     : 颜色
*	返 回 值: 无
*********************************************************************************************************
*/
void LCDH7_DrawLine(uint16_t _usX1 , uint16_t _usY1 , uint16_t _usX2 , uint16_t _usY2 , uint16_t _usColor)
{
	int32_t dx , dy ;
	int32_t tx , ty ;
	int32_t inc1 , inc2 ;
	int32_t d , iTag ;
	int32_t x , y ;

	/* 采用 Bresenham 算法，在2点间画一条直线 */

	LCDH7_PutPixel(_usX1 , _usY1 , _usColor);

	/* 如果两点重合，结束后面的动作。*/
	if ( _usX1 == _usX2 && _usY1 == _usY2 )
	{
		return;
	}

	iTag = 0 ;
	/* dx = abs ( _usX2 - _usX1 ); */
	if (_usX2 >= _usX1)
	{
		dx = _usX2 - _usX1;
	}
	else
	{
		dx = _usX1 - _usX2;
	}

	/* dy = abs ( _usY2 - _usY1 ); */
	if (_usY2 >= _usY1)
	{
		dy = _usY2 - _usY1;
	}
	else
	{
		dy = _usY1 - _usY2;
	}

	if ( dx < dy )   /*如果dy为计长方向，则交换纵横坐标。*/
	{
		uint16_t temp;

		iTag = 1 ;
		temp = _usX1; _usX1 = _usY1; _usY1 = temp;
		temp = _usX2; _usX2 = _usY2; _usY2 = temp;
		temp = dx; dx = dy; dy = temp;
	}
	tx = _usX2 > _usX1 ? 1 : -1 ;    /* 确定是增1还是减1 */
	ty = _usY2 > _usY1 ? 1 : -1 ;
	x = _usX1 ;
	y = _usY1 ;
	inc1 = 2 * dy ;
	inc2 = 2 * ( dy - dx );
	d = inc1 - dx ;
	while ( x != _usX2 )     /* 循环画点 */
	{
		if ( d < 0 )
		{
			d += inc1 ;
		}
		else
		{
			y += ty ;
			d += inc2 ;
		}
		if ( iTag )
		{
			LCDH7_PutPixel ( y , x , _usColor) ;
		}
		else
		{
			LCDH7_PutPixel ( x , y , _usColor) ;
		}
		x += tx ;
	}	
}

/*
*********************************************************************************************************
*	函 数 名: LCDH7_DrawHLine
*	功能说明: 绘制一条水平线. 使用STM32F429 DMA2D硬件绘制.
*	形    参:
*			_usX1, _usY1 : 起始点坐标
*			_usLen       : 线的长度
*			_usColor     : 颜色
*	返 回 值: 无
*********************************************************************************************************
*/
void LCDH7_DrawHLine(uint16_t _usX, uint16_t _usY, uint16_t _usLen , uint16_t _usColor)
{
#if 0
	LCDH7_FillRect(_usX, _usY, 1, _usLen, _usColor);
#else	
	uint16_t i;
	
	for (i = 0; i < _usLen; i++)
	{	
		LCDH7_PutPixel(_usX + i , _usY , _usColor);
	}
#endif	
}

/*
*********************************************************************************************************
*	函 数 名: LCDH7_DrawVLine
*	功能说明: 绘制一条垂直线。 使用STM32F429 DMA2D硬件绘制.
*	形    参:
*			_usX, _usY : 起始点坐标
*			_usLen       : 线的长度
*			_usColor     : 颜色
*	返 回 值: 无
*********************************************************************************************************
*/
void LCDH7_DrawVLine(uint16_t _usX , uint16_t _usY , uint16_t _usLen , uint16_t _usColor)
{
#if 0
	LCDH7_FillRect(_usX, _usY, _usLen, 1, _usColor);
#else	
	uint16_t i;
	
	for (i = 0; i < _usLen; i++)
	{	
		LCDH7_PutPixel(_usX, _usY + i, _usColor);
	}
#endif	
}
/*
*********************************************************************************************************
*	函 数 名: LCDH7_DrawPoints
*	功能说明: 采用 Bresenham 算法，绘制一组点，并将这些点连接起来。可用于波形显示。
*	形    参:
*			x, y     : 坐标数组
*			_usColor : 颜色
*	返 回 值: 无
*********************************************************************************************************
*/
void LCDH7_DrawPoints(uint16_t *x, uint16_t *y, uint16_t _usSize, uint16_t _usColor)
{
	uint16_t i;

	for (i = 0 ; i < _usSize - 1; i++)
	{
		LCDH7_DrawLine(x[i], y[i], x[i + 1], y[i + 1], _usColor);
	}
}

/*
*********************************************************************************************************
*	函 数 名: LCDH7_DrawRect
*	功能说明: 绘制水平放置的矩形。
*	形    参:
*			_usX,_usY: 矩形左上角的坐标
*			_usHeight : 矩形的高度
*			_usWidth  : 矩形的宽度
*	返 回 值: 无
*********************************************************************************************************
*/
void LCDH7_DrawRect(uint16_t _usX, uint16_t _usY, uint16_t _usHeight, uint16_t _usWidth, uint16_t _usColor)
{
	/*
	 ---------------->---
	|(_usX，_usY)        |
	V                    V  _usHeight
	|                    |
	 ---------------->---
		  _usWidth
	*/
	LCDH7_DrawHLine(_usX, _usY, _usWidth, _usColor);
	LCDH7_DrawVLine(_usX +_usWidth - 1, _usY, _usHeight, _usColor);
	LCDH7_DrawHLine(_usX, _usY + _usHeight - 1, _usWidth, _usColor);
	LCDH7_DrawVLine(_usX, _usY, _usHeight, _usColor);
}

/*
*********************************************************************************************************
*	函 数 名: LCDH7_FillRect
*	功能说明: 用一个颜色值填充一个矩形。使用STM32F429内部DMA2D硬件绘制。
*	形    参:
*			_usX,_usY: 矩形左上角的坐标
*			_usHeight : 矩形的高度
*			_usWidth  : 矩形的宽度
*			_usColor  : 颜色代码
*	返 回 值: 无
*********************************************************************************************************
*/
void LCDH7_FillRect(uint16_t _usX, uint16_t _usY, uint16_t _usHeight, uint16_t _usWidth, uint16_t _usColor)
{
	/* 使用DMA2D硬件填充矩形 */
	uint32_t  Xaddress = 0;
	uint16_t  OutputOffset;
	uint16_t  NumberOfLine;
	uint16_t  PixelPerLine;	
		
	if (g_LcdDirection == 0)		/* 横屏 */
	{
		Xaddress = s_CurrentFrameBuffer + 2 * (g_LcdWidth * _usY + _usX);	
		OutputOffset = g_LcdWidth - _usWidth;
		NumberOfLine = _usHeight;
		PixelPerLine = _usWidth;
	}
	else if (g_LcdDirection == 1)	/* 横屏180°*/
	{
		Xaddress = s_CurrentFrameBuffer + 2 * (g_LcdWidth * (g_LcdHeight - _usHeight - _usY) + g_LcdWidth - _usX - _usWidth);	
		OutputOffset = g_LcdWidth - _usWidth;
		NumberOfLine = _usHeight;
		PixelPerLine = _usWidth;
	}
	else if (g_LcdDirection == 2)	/* 竖屏 */
	{
		Xaddress = s_CurrentFrameBuffer + 2 * (g_LcdHeight * (g_LcdWidth - _usX -  _usWidth) + _usY);	
		OutputOffset = g_LcdHeight - _usHeight;
		NumberOfLine = _usWidth;
		PixelPerLine = _usHeight;
	}
	else if (g_LcdDirection == 3)	/* 竖屏180° */
	{
		Xaddress = s_CurrentFrameBuffer + 2 * (g_LcdHeight * _usX + g_LcdHeight - _usHeight - _usY);	
		OutputOffset = g_LcdHeight - _usHeight;
		NumberOfLine = _usWidth;
		PixelPerLine = _usHeight;		
	}	

	//DMA2D_FillBuffer(uint32_t LayerIndex, void * pDst, uint32_t xSize, uint32_t ySize, uint32_t OffLine, uint32_t ColorIndex) 
	DMA2D_FillBuffer(s_CurrentLayer, (void *)Xaddress, PixelPerLine, NumberOfLine, OutputOffset, _usColor);
}

/*
*********************************************************************************************************
*	函 数 名: LCDH7_DrawCircle
*	功能说明: 绘制一个圆，笔宽为1个像素
*	形    参:
*			_usX,_usY  : 圆心的坐标
*			_usRadius  : 圆的半径
*	返 回 值: 无
*********************************************************************************************************
*/
void LCDH7_DrawCircle(uint16_t _usX, uint16_t _usY, uint16_t _usRadius, uint16_t _usColor)
{
	int32_t  D;			/* Decision Variable */
	uint32_t  CurX;		/* 当前 X 值 */
	uint32_t  CurY;		/* 当前 Y 值 */

	D = 3 - (_usRadius << 1);
	CurX = 0;
	CurY = _usRadius;

	while (CurX <= CurY)
	{
		LCDH7_PutPixel(_usX + CurX, _usY + CurY, _usColor);
		LCDH7_PutPixel(_usX + CurX, _usY - CurY, _usColor);
		LCDH7_PutPixel(_usX - CurX, _usY + CurY, _usColor);
		LCDH7_PutPixel(_usX - CurX, _usY - CurY, _usColor);
		LCDH7_PutPixel(_usX + CurY, _usY + CurX, _usColor);
		LCDH7_PutPixel(_usX + CurY, _usY - CurX, _usColor);
		LCDH7_PutPixel(_usX - CurY, _usY + CurX, _usColor);
		LCDH7_PutPixel(_usX - CurY, _usY - CurX, _usColor);

		if (D < 0)
		{
			D += (CurX << 2) + 6;
		}
		else
		{
			D += ((CurX - CurY) << 2) + 10;
			CurY--;
		}
		CurX++;
	}
}

/*
*********************************************************************************************************
*	函 数 名: LCDH7_DrawBMP
*	功能说明: 在LCD上显示一个BMP位图，位图点阵扫描次序：从左到右，从上到下
*	形    参:  
*			_usX, _usY : 图片的坐标
*			_usHeight  ：图片高度
*			_usWidth   ：图片宽度
*			_ptr       ：图片点阵指针
*	返 回 值: 无
*********************************************************************************************************
*/
void LCDH7_DrawBMP(uint16_t _usX, uint16_t _usY, uint16_t _usHeight, uint16_t _usWidth, uint16_t *_ptr)
{
	uint16_t i, k, y;
	const uint16_t *p;

	p = _ptr;
	y = _usY;
	for (i = 0; i < _usHeight; i++)
	{
		for (k = 0; k < _usWidth; k++)
		{
			LCDH7_PutPixel(_usX + k, y, *p++);
		}
		
		y++;
	}
}

/*
*********************************************************************************************************
*	函 数 名: LCDH7_SetDirection
*	功能说明: 设置显示屏显示方向（横屏 竖屏）
*	形    参: 显示方向代码 0 横屏正常, 1=横屏180度翻转, 2=竖屏, 3=竖屏180度翻转
*	返 回 值: 无
*********************************************************************************************************
*/
void LCDH7_SetDirection(uint8_t _dir)
{
	uint16_t temp;
	
	if (_dir == 0 || _dir == 1)		/* 横屏， 横屏180度 */
	{
		if (g_LcdWidth < g_LcdHeight)
		{
			temp = g_LcdWidth;
			g_LcdWidth = g_LcdHeight;
			g_LcdHeight = temp;			
		}
	}
	else if (_dir == 2 || _dir == 3)	/* 竖屏, 竖屏180°*/
	{
		if (g_LcdWidth > g_LcdHeight)
		{
			temp = g_LcdWidth;
			g_LcdWidth = g_LcdHeight;
			g_LcdHeight = temp;			
		}
	}
}

/*
*********************************************************************************************************
*	函 数 名: LCDH7_BeginDraw
*	功能说明: 双缓冲区工作模式。开始绘图。将当前显示缓冲区的数据完整复制到另外一个缓冲区。
*			 必须和 LCDH7_EndDraw函数成对使用。 实际效果并不好。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void LCDH7_BeginDraw(void)
{
//	uint16_t *src;
//	uint16_t *dst;
//		
//	if (s_CurrentFrameBuffer == LCDH7_FRAME_BUFFER)
//	{
//		src = (uint16_t *)LCDH7_FRAME_BUFFER;
//		dst =  (uint16_t *)(LCDH7_FRAME_BUFFER + BUFFER_OFFSET);
//		
//		s_CurrentFrameBuffer = LCDH7_FRAME_BUFFER + BUFFER_OFFSET;
//	}
//	else
//	{
//		src = (uint16_t *)(LCDH7_FRAME_BUFFER + BUFFER_OFFSET);
//		dst = (uint16_t *)LCDH7_FRAME_BUFFER;
//		
//		s_CurrentFrameBuffer = LCDH7_FRAME_BUFFER;
//	}
//	
//	_DMA_Copy(src, dst, g_LcdHeight, g_LcdWidth, 0, 0);
}

/*
*********************************************************************************************************
*	函 数 名: LCDH7_EndDraw
*	功能说明: APP结束了缓冲区绘图工作，切换硬件显示。
*			 必须和 LCDH7_BeginDraw函数成对使用。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void LCDH7_EndDraw(void)
{
	//LTDC_LayerAddress(LTDC_Layer1, s_CurrentFrameBuffer);
}

/*
*********************************************************************************************************
*	函 数 名: _GetBufferSize
*	功能说明: 无
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
uint32_t _GetBufferSize(uint8_t LayerIndex)
{
	return g_LcdWidth * g_LcdHeight;
}

/*
*********************************************************************************************************
*	函 数 名: LCDH7_InitDMA2D
*	功能说明: 配置DMA2D
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void LCDH7_InitDMA2D(void)
{
	/* 使能DMA2D时钟 */
	__HAL_RCC_DMA2D_CLK_ENABLE();   
	
	/* 配置默认模式 */ 
	hdma2d.Init.Mode         = DMA2D_R2M;
	hdma2d.Init.ColorMode    = DMA2D_INPUT_RGB565;
	hdma2d.Init.OutputOffset = 0x0;     

	hdma2d.Instance          = DMA2D; 

	if (HAL_DMA2D_Init(&hdma2d) != HAL_OK)
	{
		Error_Handler(__FILE__, __LINE__);
	}
 }

/*
*********************************************************************************************************
*	函 数 名: LCD_LL_GetPixelformat
*	功能说明: 获取图层的颜色格式
*	形    参: 无
*	返 回 值: 返回相应图层的颜色格式
*********************************************************************************************************
*/
static inline uint32_t LCD_LL_GetPixelformat(uint32_t LayerIndex)
{
	if (LayerIndex == 0)
	{
		return LTDC_PIXEL_FORMAT_RGB565;
	} 
	else
	{
		return LTDC_PIXEL_FORMAT_RGB565;  // LTDC_PIXEL_FORMAT_ARGB1555;
	} 
}

/*
*********************************************************************************************************
*	函 数 名: DMA2D_CopyBuffer
*	功能说明: 通过DMA2D从前景层复制指定区域的颜色数据到目标区域
*	形    参: LayerIndex    图层
*             pSrc          颜色数据源地址
*             pDst          颜色数据目的地址
*             xSize         要复制区域的X轴大小，即每行像素数
*             ySize         要复制区域的Y轴大小，即行数
*             OffLineSrc    前景层图像的行偏移
*             OffLineDst    输出的行偏移
*	返 回 值: 无
*********************************************************************************************************
*/
void DMA2D_CopyBuffer(uint32_t LayerIndex, void * pSrc, void * pDst, uint32_t xSize, uint32_t ySize, uint32_t OffLineSrc, uint32_t OffLineDst)
{
	uint32_t PixelFormat;

	PixelFormat = LCD_LL_GetPixelformat(LayerIndex);
	DMA2D->CR      = 0x00000000UL | (1 << 9);  

	/* 设置基本参数 */
	DMA2D->FGMAR   = (uint32_t)pSrc;                       
	DMA2D->OMAR    = (uint32_t)pDst;                       
	DMA2D->FGOR    = OffLineSrc;                      
	DMA2D->OOR     = OffLineDst; 

	/* 设置颜色格式 */  
	DMA2D->FGPFCCR = PixelFormat;  

	/*  设置传输大小 */
	DMA2D->NLR     = (uint32_t)(xSize << 16) | (uint16_t)ySize; 

	DMA2D->CR     |= DMA2D_CR_START;   

	/* 等待传输完成 */
	while (DMA2D->CR & DMA2D_CR_START) 
	{
	}
}

/*
*********************************************************************************************************
*	函 数 名: _LCD_DrawBitmap16bpp
*	功能说明: 16bpp位图绘制，专用于摄像头
*	形    参: --
*	返 回 值: 无
*********************************************************************************************************
*/
void _LCD_DrawCamera16bpp(int x, int y, uint16_t * p, int xSize, int ySize, int SrcOffLine) 
{
	uint32_t  AddrDst;
	int OffLineSrc, OffLineDst;

	AddrDst =s_CurrentFrameBuffer + (y * g_LcdWidth + x) * 2;
	OffLineSrc = SrcOffLine; 		/* 源图形的偏移，这里是全部都是用了 */
	OffLineDst = g_LcdWidth - xSize;/* 目标图形的便宜 */
	DMA2D_CopyBuffer(LCD_LAYER_1, (void *)p, (void *)AddrDst, xSize, ySize, OffLineSrc, OffLineDst);	
}

/*
*********************************************************************************************************
*	函 数 名: _DMA_Fill
*	功能说明: 通过DMA2D对于指定区域进行颜色填充
*	形    参: LayerIndex    图层
*             pDst          颜色数据目的地址
*             xSize         要复制区域的X轴大小，即每行像素数
*             ySize         要复制区域的Y轴大小，即行数
*             OffLine       前景层图像的行偏移
*             ColorIndex    要填充的颜色值
*	返 回 值: 无
*********************************************************************************************************
*/
static void DMA2D_FillBuffer(uint32_t LayerIndex, void * pDst, uint32_t xSize, uint32_t ySize, uint32_t OffLine, uint32_t ColorIndex) 
{
	uint32_t PixelFormat;

	PixelFormat = LCD_LL_GetPixelformat(LayerIndex);

	/* 颜色填充 */
	DMA2D->CR      = 0x00030000UL | (1 << 9);        
	DMA2D->OCOLR   = ColorIndex;                     

	/* 设置填充的颜色目的地址 */
	DMA2D->OMAR    = (uint32_t)pDst;                      

	/* 目的行偏移地址 */
	DMA2D->OOR     = OffLine;                        

	/* 设置颜色格式 */
	DMA2D->OPFCCR  = PixelFormat;                    

	/* 设置填充大小 */
	DMA2D->NLR     = (uint32_t)(xSize << 16) | (uint16_t)ySize;

	DMA2D->CR     |= DMA2D_CR_START; 

	/* 等待DMA2D传输完成 */
	while (DMA2D->CR & DMA2D_CR_START) 
	{
	}
}

/*
*********************************************************************************************************
*	函 数 名: LTDC_IRQHandler
*	功能说明: LTDC中断服务程序
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void LTDC_IRQHandler(void)
{
	HAL_LTDC_IRQHandler(&hltdc_F);
}

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
