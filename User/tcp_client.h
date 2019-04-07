/*
*********************************************************************************************************
*
*	模块名称 : tcp_client 
*	文件名称 : tcp_client.h
*	版    本 : V1.0
*	说    明 : 
*
*	修改记录 :
*						版本号    日期        作者       说明
*						V1.0  2019年04月04日  suozhang   首次发布
*
*********************************************************************************************************
*/

#ifndef  __TCP_CLIENT_H__
#define  __TCP_CLIENT_H__

#include "lwip/opt.h"
	
void tcp_client_conn_server_task( void );

int send_server_data( uint8_t *data, uint16_t len );
int received_server_data_process( uint8_t *data, uint16_t len );

#endif
