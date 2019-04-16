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

/**
 * Log default configuration for EasyLogger.
 * NOTE: Must defined before including the <elog.h>
 */
#if !defined(LOG_TAG)
#define LOG_TAG                    "main_test_tag:"
#endif
#undef LOG_LVL
#if defined(XX_LOG_LVL)
    #define LOG_LVL                    XX_LOG_LVL
#endif

#include "elog.h"

#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "netif_port.h"

#include "tcp_client.h"

#include "lwip/apps/lwiperf.h"

static void vTaskLED (void *pvParameters);
static void vTaskLwip(void *pvParameters);

 TaskHandle_t xHandleTaskLED  = NULL;
 TaskHandle_t xHandleTaskLwip = NULL;

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
	
	/* initialize EasyLogger */
	if (elog_init() == ELOG_NO_ERR)
	{
			/* set enabled format */
			elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL & ~ELOG_FMT_P_INFO);
			elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_ALL );
			elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
			elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_TAG | ELOG_FMT_TIME);
			elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_ALL & ~(ELOG_FMT_FUNC | ELOG_FMT_P_INFO));
			elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_ALL & ~(ELOG_FMT_FUNC | ELOG_FMT_P_INFO));
		
			elog_set_text_color_enabled( true );
		
			elog_buf_enabled( false );
		
			/* start EasyLogger */
			elog_start();
	}
	
	xTaskCreate( vTaskLED, "vTaskLED", 512, NULL, 3, &xHandleTaskLED );
	xTaskCreate( vTaskLwip,"Lwip"     ,512, NULL, 2, &xHandleTaskLwip );
	
	/* 启动调度，开始执行任务 */
	vTaskStartScheduler();
}


struct netif gnetif; /* network interface structure */

static void netif_config(void)
{
  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;

  IP_ADDR4(&ipaddr,192,168,0,11);
  IP_ADDR4(&netmask,255,255,255,0);
  IP_ADDR4(&gw,192,168,0,1);

  /* add the network interface */ 
  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);
  
  /*  Registers the default network interface. */
  netif_set_default(&gnetif);
	
	/* Set the link callback function, this function is called on change of link status*/
  netif_set_link_callback(&gnetif, eth_link_callback);

}

/*
*********************************************************************************************************
*	函 数 名: vTaskLwip
*	功能说明: 初始化 ETH,MAC,DMA,GPIO,LWIP,并创建线程用于处理以太网消息
*	形    参: pvParameters 是在创建该任务时传递的形参
*	返 回 值: 无
* 优 先 级: 2  
*********************************************************************************************************
*/
static void vTaskLwip(void *pvParameters)
{

  /* Create tcp_ip stack thread */
  tcpip_init(NULL, NULL);

  /* Initilaize the netif */
  netif_config();

	for(;;)
	{
	
		ip_addr_t  perf_server_ip;
		
		IP_ADDR4( &perf_server_ip, 192, 168, 0, 11 ); //初始化 perf_server_ip  

		lwiperf_start_tcp_server( &perf_server_ip, 9527, NULL, NULL );

		vTaskDelay( 2000 / portTICK_PERIOD_MS );
	}
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

//		log_i( "SystemCoreClock:%u,system heap:%u.", SystemCoreClock,xPortGetFreeHeapSize() );

	}
}

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
