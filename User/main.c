/*
*********************************************************************************************************
*
*	模块名称 : 主程序模块
*	文件名称 : main.c
*	版    本 : V1.1
*	说    明 : 
*
*	修改记录 :
*		版本号   日期         作者        说明
*		V1.0    2018-12-12   Eric2013     1. CMSIS软包版本 V5.4.0
*                                     2. HAL库版本 V1.3.0
*
*   V1.1    2019-04-01   suozhang     1. add FreeRTOS V10.20
*
*	Copyright (C), 2018-2030, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/	
#include "bsp.h"			/* 底层硬件驱动 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "croutine.h"
#include "semphr.h"
#include "event_groups.h"


static void vTaskLED (void *pvParameters);
static TaskHandle_t xHandleTaskLED  = NULL;

/*
*********************************************************************************************************
*	函 数 名: main
*	功能说明: c程序入口
*	形    参: 无
*	返 回 值: 错误代码(无需处理)
*********************************************************************************************************
*/
int main(void)
{

	bsp_Init();		/* 硬件初始化 */
	
	xTaskCreate( vTaskLED, "vTaskLED", 512, NULL, 3, &xHandleTaskLED );
	
	/* 启动调度，开始执行任务 */
	vTaskStartScheduler();
}


/*
*********************************************************************************************************
*	函 数 名: vTaskLED
*	功能说明: KED闪烁	
*	形    参: pvParameters 是在创建该任务时传递的形参
*	返 回 值: 无
* 优 先 级: 2  
*********************************************************************************************************
*/
static void vTaskLED(void *pvParameters)
{
	
	uint32_t ulNotifiedValue     = 0;
	uint32_t ledToggleIntervalMs = 1000;

	for(;;)
	{
		
		/*
			* 参数 0x00      表示使用通知前不清除任务的通知值位，
			* 参数 ULONG_MAX 表示函数xTaskNotifyWait()退出前将任务通知值设置为0
			*/
	 if( pdPASS == xTaskNotifyWait( 0x00, 0xffffffffUL, &ulNotifiedValue, ledToggleIntervalMs ) )
	 {
		 if( ulNotifiedValue < 2000 )
			ledToggleIntervalMs  = ulNotifiedValue;
		 else
			 ledToggleIntervalMs = 1000 / portTICK_PERIOD_MS;
	 }

		bsp_LedToggle(1);

		printf( "SystemCoreClock:%u,systick:%u,system heap:%u.\r\n", SystemCoreClock, xTaskGetTickCount(), xPortGetFreeHeapSize() );

	}
}

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
