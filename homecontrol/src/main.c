/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
* main.c
shell-init: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
chdir: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
* Copyright (C) 2014 gsj0791 <gsj0791@163.com>
*
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "udpbroadcast.h"
#include "tcpserver.h"
#include "taskmanager.h"
#include "hardware.h"
#include "radionetwork.h"

#define UDP_PORT 21680

#define SERVER_PORT 2088
#define DATA_SERVER_PORT 8089

#define DEVICE_NAME "gs_box"
#define DEVICE_ID "1001180615"

#ifndef OS_UBUNTU

#include <ctype.h>


static void getNetworkValue(char *szBuf, uint8_t *value)
{
    int i,j;
	uint8_t len;

	for(i =0;i < strlen(szBuf);i++)
    {              
		if(*(szBuf+i)==0x27)
		{			
			memset(value,'\0',18);
			for(j=0;j<18;j++)
			{
				i++;				
				if(*(szBuf+i)==0x27)
				{
					return;
				}
				*value++=*(szBuf+i);
			}
		}
    }
}

static void getNetworkInfo(NETWORK_ADAPTER *adapter,char *interface)
{
    char szBuf[64];
	uint8_t interface_flag=0;
	FILE *fp = NULL;

    if((fp=fopen("/etc/config/network", "r"))==NULL)            
    {
        printf( "Can 't   open   file!\n"); 
        return;
    }
    while(fgets(szBuf,128,fp))                    
    {                         
		if(interface_flag==0)  //jyc20150813add
		{		
			if(strstr(szBuf,interface) != NULL)  
				interface_flag=1;
			continue;
		}  
		if(strstr(szBuf, "ipaddr") != NULL)
        {
			if((++interface_flag)>=3)break; //jyc20151028add	
			getNetworkValue(szBuf, adapter->ipaddr);			
        }
		if(strstr(szBuf, "netmask") != NULL)
        {
			getNetworkValue(szBuf, adapter->netmask);
        }
		if(strstr(szBuf, "gateway") != NULL)
        {
			getNetworkValue(szBuf, adapter->gateway);
        }
		if(strstr(szBuf, "dns") != NULL)
        {
			getNetworkValue(szBuf, adapter->dns);
        }
		if(strstr(szBuf, "macaddr") != NULL)
        {
			getNetworkValue(szBuf, adapter->mac);
        }
    }			
    fclose(fp);
}
#endif

static uint8_t get_three(char *addr)
{
	uint8_t i,j=0;
	for(i=0;i<strlen(addr);i++)
	{
		if(addr[i]=='.')
			j++;
		if(j==3)
			return i;
	}
}

static uint8_t cmp_route_addr(char *addr1,char *addr2)
{
	uint8_t len;
	len = get_three(addr1);
	if(memcmp(addr1,addr2,len)==0)
		return 0;
	else
		return 1;
}

static void onUDPCallback(void *arg, uint8_t *data, int size, struct sockaddr_in *addr)
{
	uint8_t temp[260] = {0};
	int read_size;
	TASK_SOCK *sock = arg;
    packet pkt = {0};
    read_size = packet_decode(&pkt, (uint8_t *)data, size);
	if(	read_size > 0){
		switch(pkt.command){
			case CMD_BROADCAST_INFO:
			{
				NETWORK_ADAPTER *adp;				
				uint8_t *enc = temp;
				char *route_addr;
				route_addr=inet_ntoa(addr->sin_addr);
				if((cmp_route_addr(route_addr,sock->adapter2->ipaddr)==0)&&cmp_route_addr(route_addr,sock->adapter->ipaddr))
					adp = sock->adapter2;
				else
					adp = sock->adapter;
				enc = packet_encodeString(enc, (const char *)adp->ipaddr); //ipaddress, replace with a real one
				enc = packet_encodeString(enc, (const char *)adp->netmask);//mask, replace with a real one
				enc = packet_encodeString(enc, (const char *)adp->gateway);//gateway, replace with a real one
				enc = packet_encodeString(enc, (const char *)adp->dns);//DNS1, replace with a real one
				enc = packet_encodeString(enc, "");//DNS2, replace with a real one
				enc = packet_encodeInt16(enc, 80); //WEB PORT,replace with a real one
				enc = packet_encodeInt16(enc, DATA_SERVER_PORT); //COM PORT,replace with a real one
				enc = packet_encodeInt16(enc, SERVER_PORT); //DATA PORT
				enc = packet_encodeString(enc, (const char *)adp->mac); //MAC
				enc = packet_encodeString(enc, DEVICE_NAME); //DEVICE NAME
				enc = packet_encodeBoolean(enc, 0); //USE DHCP,replace with a real one
				enc = packet_encodeString(enc, DEVICE_ID); //DEVICE ID
				enc = packet_encodeString(enc, (const char *)sock->version); //VERSION VERSION_SOFT 
				//enc = packet_encodeString(enc, VERSION_SOFT);

				pkt.ack = ACK_REPLY;
				pkt.data = temp;
				pkt.length = enc - temp;
				
				size = packet_readBuffer(&pkt, data);
				if(size > 0){
					udpbroadcast_send(sock->socket_fd, addr, (char *)data, size);
				}
				break;
			}
		}
	}
}

static void createTcpService(int port,TASK *task, SERVICE *serv)
{
	struct evconnlistener *listener;
	struct sockaddr_in sin;	
	//TCP Server
	memset(&sin, 0, sizeof(sin));
	/* This is an INET address */
	sin.sin_family = AF_INET;
	/* Listen on 0.0.0.0 */
	sin.sin_addr.s_addr = htonl(0);
	/* Listen on the given port. */
	sin.sin_port = htons(port);

	listener = evconnlistener_new_bind(task->evbase, connection_accept, serv,
		LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
		(struct sockaddr*)&sin, sizeof(sin));

	if (!listener) {
		perror("Couldn't create listener");
		return;
	}

	evconnlistener_set_error_cb(listener, connection_accept_error);
}


int main(void)
{
	SERVICE tcp_service = {0}, serial_service= {0};
	serial_packet prev_packet = {0};
	TASK task = {0};
	BOARD _board = {0};
	NETWORK_ADAPTER adapter,adapter2;
	uint8_t version[20] = {0};
	RADIO_NETWORK rd;

	#ifndef OS_UBUNTU
	getNetworkInfo(&adapter,"'lan'");
	getNetworkInfo(&adapter2,"'wwan'");
	hardware_soft_version(&version);
    #else
	memcpy(version, VERSION_SOFT, sizeof(VERSION_SOFT));
    #endif
	
	tcp_service.logged = 0;
	tcp_service.service_type = 0;
	tcp_service.task = &task;
	tcp_service.radio = &rd;

	serial_service.logged = 0;
	serial_service.service_type = 1;
	serial_service.task = &task;

	_board.prve_packet = &prev_packet;
	_board.task = &task;
	_board.radio = &rd;
	
	taskmanager_init(&task);

	if(hardware_init(&_board)!=0){
		return 0;
	}
	
	int udp_socket = udpbroadcast_init(UDP_PORT);
	if(udp_socket != -1){
	   task.sock.version = version;
	   task.sock.adapter = &adapter;
	   task.sock.adapter2 = &adapter2;
	   taskmanager_add_socket(&task, udp_socket, onUDPCallback);
	}

	createTcpService(SERVER_PORT, &task, &tcp_service);
	createTcpService(DATA_SERVER_PORT, &task, &serial_service);

	radionetwork_init(&task, _board.radio);

	taskmanager_run(&task); //loop until error
	taskmanager_close(&task);

	radionetwork_free(&task, _board.radio);
	hardware_close(&_board);
	udpbroadcast_close(udp_socket);

	return (0);
}
