/*
*********************************************************************************************************
*
*	模块名称 : tcp_client
*	文件名称 : tcp_client.c
*	版    本 : V1.0
*	说    明 : TCP 客户端 用于连接后台服务器
*
*	修改记录 :
*						版本号    日期        作者       说明
*						V1.0  2019年04月04日  suozhang   首次发布
*
*********************************************************************************************************
*/


#include "tcp_client.h"


/**
 * Log default configuration for EasyLogger.
 * NOTE: Must defined before including the <elog.h>
 */
#if !defined(LOG_TAG)
#define LOG_TAG                    "tcp_client_tag:"
#endif
#undef LOG_LVL
#if defined(XX_LOG_LVL)
    #define LOG_LVL                    XX_LOG_LVL
#endif

#include "elog.h"

#if LWIP_NETCONN

#include "lwip/sys.h"
#include "lwip/api.h"

#include "lwip/tcp.h"
#include "lwip/ip.h"

extern TaskHandle_t xHandleTaskLED;

#define TCP_SERVER_IP   "192.168.0.22"
#define TCP_SERVER_PORT 9527

/* 服务器通信使用权信号量 */
SemaphoreHandle_t xServerCommunicationLockSemaphore = NULL;

static struct netconn *tcp_client_server_conn;

void tcp_client_conn_server_task( void )
{
  struct netbuf *buf;
  void *data;
  u16_t len;
  err_t err;
	
	ip_addr_t server_ip;
	

	
	u16_t server_port = TCP_SERVER_PORT;				     // 服务器端口号初始化

	ip4addr_aton( TCP_SERVER_IP, &server_ip ); 			 // 服务器IP地址初始化

	xServerCommunicationLockSemaphore = xSemaphoreCreateBinary();

	if( NULL == xServerCommunicationLockSemaphore )
	{
			log_e("err:xServerCommunicationLockSemaphore == NULL,while(1).");
			while(1);
	}
	
	for( ;; )
	{
				
		log_i("tcp server connecting %s:%d......", ipaddr_ntoa(&server_ip), server_port );
		
		
		xTaskNotify( xHandleTaskLED, 200, eSetValueWithOverwrite );/* 服务器断开连接状态，LED闪烁为200mS一次. */
		
		/* Create a new connection identifier. */
		tcp_client_server_conn = netconn_new( NETCONN_TCP );
				
		if( tcp_client_server_conn != NULL )
		{		
			//打开TCP 的保活功能 （客户端不默认打开），2018年12月6日10:00:41，SuoZhang
			tcp_client_server_conn->pcb.tcp->so_options |= SOF_KEEPALIVE;

			err = netconn_connect( tcp_client_server_conn, &server_ip, server_port );
					
			if(err == ERR_OK)
			{
				log_i("TCP Server %s:%d connected sucess.", ipaddr_ntoa(&server_ip), server_port );
						
				xSemaphoreGive( xServerCommunicationLockSemaphore ); /* 释放服务器通信使用权 */

				xTaskNotify( xHandleTaskLED, 1000, eSetValueWithOverwrite );/* 服务器连接状态，LED闪烁为1000mS一次. */
				
				for( ;; )
				{
					/* receive data until the other host closes the connection */
					if((err = netconn_recv(tcp_client_server_conn, &buf)) == ERR_OK) 
					{
								 //获取一个指向netbuf 结构中的数据的指针
								 if((err = netbuf_data(buf, &data, &len)) == ERR_OK)
								 {
									  received_server_data_process( data, len );
										netbuf_delete(buf);
								 }
								 else
								 {
									 log_e("err:netbuf_data(buf, &data, &len):%d.",err);
								 }
								
					}
					else//if((err = netconn_recv(conn, &buf)) == ERR_OK)
					{
						log_e("err:netconn_recv(conn, &buf):%d.",err);
						netbuf_delete(buf);	
						break; //连接发生错误，退出死等数据的循环，重新建立链接
					}
				 }
			 }
			
			 log_e("err:TCP Server %s:%d connect fail,err:%d.", ipaddr_ntoa(&server_ip), server_port, err );
			 netconn_close  ( tcp_client_server_conn );
			 netconn_delete ( tcp_client_server_conn );		
		   vTaskDelay(1000);
		}
		else//(conn!=NULL)
		{
			log_e("err:Can not create tcp_client_server_conn.");
			vTaskDelay(1000);
		}
	}
}

int received_server_data_process( uint8_t *data, uint16_t len )
{
	 return send_server_data( data, len );
}

int send_server_data( uint8_t *data, uint16_t len )
{
												 
	if( tcp_client_server_conn )
	{
		return netconn_write( tcp_client_server_conn, data, len, NETCONN_COPY);
	}
	else
		return ERR_CONN; 

}

#endif /* LWIP_NETCONN */
















