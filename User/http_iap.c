/*
*********************************************************************************************************
*
*	模块名称 : http_iap
*	文件名称 : http_iap.c
*	版    本 : V1.0
*	说    明 : http 客户端 连接服务器 下载文件 进行 IAP 升级
*
*	修改记录 :
*						版本号    日期        作者       说明
*						V1.0  2019年04月12日  suozhang   首次发布
*
*********************************************************************************************************
*/


#include "http_iap.h"

/**
 * Log default configuration for EasyLogger.
 * NOTE: Must defined before including the <elog.h>
 */
#if !defined(LOG_TAG)
#define LOG_TAG                    "http_iap_tag:"
#endif
#undef LOG_LVL
#if defined(XX_LOG_LVL)
    #define LOG_LVL                    XX_LOG_LVL
#endif

#include "elog.h"

#include "lwip/apps/http_client.h"

#include "lwip/altcp_tcp.h"
#include "lwip/dns.h"
#include "lwip/debug.h"
#include "lwip/mem.h"
#include "lwip/altcp_tls.h"
#include "lwip/init.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "croutine.h"
#include "semphr.h"
#include "event_groups.h"

#include "lwip/sys.h"
#include "lwip/api.h"

#include "lwip/tcp.h"
#include "lwip/ip.h"

TaskHandle_t happ_iap_task_handle = NULL;

uint8_t buffer[2048] = {0};

uint32_t file_len = 0 ;
uint8_t  file_buffer[250*1024] = {0};

#if LWIP_TCP && LWIP_CALLBACK_API

/** Function prototype for tcp receive callback functions. Called when data has
 * been received.
 *
 * @param arg Additional argument to pass to the callback function (@see tcp_arg())
 * @param tpcb The connection pcb which received data
 * @param p The received data (or NULL when the connection has been closed!)
 * @param err An error code if there has been an error receiving
 *            Only return ERR_ABRT if you have called tcp_abort from within the
 *            callback function!
 */
err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
	
  struct pbuf *data =NULL;
	
  for(data = p; data != NULL; data = data->next)
  {
    if( file_len >= sizeof(file_buffer) )
		{
			log_e("buffer overflow.");
			
			goto err_process;
		}
		
		memcpy( &file_buffer[file_len], data->payload, data->len );

    file_len=file_len+data->len;
  }
	
	log_i("tcp_recv_callback total file_len:%d, err:%d.", file_len, err );
	
	elog_hexdump( "httpc data:", 8, file_buffer, file_len );
	
	pbuf_free(p);
	return ERR_OK;
	
err_process:
		
	pbuf_free(p);
	return ERR_IF;
	
}

/**
 * @ingroup httpc 
 * Prototype of a http client callback function
 *
 * @param arg argument specified when initiating the request
 * @param httpc_result result of the http transfer (see enum httpc_result_t)
 * @param rx_content_len number of bytes received (without headers)
 * @param srv_res this contains the http status code received (if any)
 * @param err an error returned by internal lwip functions, can help to specify
 *            the source of the error but must not necessarily be != ERR_OK
 */
void httpc_result_callback(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err)
{
	log_i("httpc_result_callback err:%d.", err );
	log_i("httpc_result_callback httpc_result_t:%d.", httpc_result );
	log_i("httpc_result_callback http status:%d.", srv_res );
	log_i("httpc_result_callback rx_content_len:%d.", rx_content_len );
	
	if( httpc_result != HTTPC_RESULT_OK )
	{
		log_e("httpc_result:%d.", httpc_result );
		
		/* HTTP 下载出错，重新开始下载文件 */
    xTaskNotifyGive( happ_iap_task_handle );
	}
	
}

/**
 * @ingroup httpc 
 * Prototype of http client callback: called when the headers are received
 *
 * @param connection http client connection
 * @param arg argument specified when initiating the request
 * @param hdr header pbuf(s) (may contain data also)
 * @param hdr_len length of the heders in 'hdr'
 * @param content_len content length as received in the headers (-1 if not received)
 * @return if != ERR_OK is returned, the connection is aborted
 */
err_t httpc_headers_done_callback(httpc_state_t *connection, void *arg, struct pbuf *hdr, u16_t hdr_len, u32_t content_len)
{
	uint32_t len=0;
  struct pbuf *data =NULL;
	
	log_i("httpc_headers_done_callback heders len:%d.", hdr_len );

  for(data = hdr; data != NULL; data = data->next)
  {
    if( len >= sizeof(buffer) )
		{
			log_e("buffer overflow.");
			return ERR_IF;
		}
		
		memcpy( &buffer[len], data->payload, data->len );

    len=len+data->len;
  }
	
//	elog_hexdump( "httpc headers data:", 8, buffer, hdr_len );
	
	log_i("httpc heders:\r\n%s.", buffer );
	
//	pbuf_free(hdr);
	return ERR_OK;
}

httpc_connection_t httpc_connection_iap;

httpc_state_t *httpc_state_iap;
 
void http_iap_task(void *pvParameters)
{
	err_t err;
	u16_t http_server_port = 80 ;
	
	httpc_connection_iap.result_fn       = httpc_result_callback;
	httpc_connection_iap.headers_done_fn = httpc_headers_done_callback;
	
	for( ;; )
	{

		/* Block to wait notify this task. */
    ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
		
		log_i("http_iap_task running.");
		
		/*	
		  * 下载例子,需要开启 lwip DNS 功能
			* http://download.savannah.gnu.org/releases/lwip/drivers/1.3.0/MCF5223X/mcf5223xif.c
			* suozhang,2019年4月15日15:32:58
			*/
		err = httpc_get_file_dns( "download.savannah.gnu.org", 
															http_server_port, 
															"/releases/lwip/drivers/1.3.0/MCF5223X/mcf5223xif.c",
															&httpc_connection_iap,
															tcp_recv_callback, 
															NULL,
															&httpc_state_iap);
		if( err != ERR_OK )
		{
			log_e("http_iap_task err:%d.", err );
			while(1);
		}

	}
	
}

#endif /* LWIP_TCP && LWIP_CALLBACK_API */














