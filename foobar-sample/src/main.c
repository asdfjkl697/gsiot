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
#include <errno.h>
#include <unistd.h>     /* */

#include <sys/types.h>
#include <sys/stat.h>  

#include <version.h>
//#include <pthread.h>

#include <fcntl.h>      /* */
#include <termios.h>    /* */

#include <sys/socket.h>  //20130823  
#include <arpa/inet.h>
#include <netinet/in.h>


/*Add 20150527 JYC*/
int actual,actual2=0; //used to find out how many bytes were written
unsigned char send_data[8];
unsigned char rec_data[64]={0,0x0e,0x4c,0x5a,0x30,0x30,0x30,0x32,8,1,1,2,2
		,0x7b,0x61,0x7d};
unsigned char usb_rec_data[128];
unsigned char usb_rec_flag=0;

#define DEST_IP "112.124.115.184"
#define DEST_PORT 10002

#define FALSE  -1
#define TRUE   0

unsigned int Time_Cnt=0;

const uint8_t upkey_buf[9]={0x33,0x03,0x28,0x19,0x82,0x07,0x14,0x02,0x18};
const uint8_t reboot_buf[9]={0x33,0x03,0x28,0x19,0x83,0x09,0x01,0x02,0x18};

void update_gs(void)
{
	printf("update ...\n");	
	system("cd /root");
	system("rm firmware.bin");
	system("wget -c http://api.gsss.cn/box_update/firmware.bin");	
	system("chmod 777 /root/test.sh");
	system("/root/test.sh");
}

int main(void)
{	
	int r = 1,i;
	ssize_t cnt;
	uint16_t crc16=0;

	unsigned char rxsockbuff[256]={0};
    unsigned char tmpbuff[20];
	unsigned char rxbuff[128]={0};
	int len;
	int sockfd;
	struct sockaddr_in dest_addr;
	int dest_len = sizeof(struct sockaddr_in);
	int rxsock_len=0;

	int serialfd;
    int nread,count=0;
    char *dev_sr  = "/dev/ttyUSB0"; 

	/************serial init******************/	

	serialfd = OpenDev(dev_sr);
        set_speed(serialfd,115200); //failed 20140916
	if (set_Parity(serialfd,8,1,'N') == FALSE)  {
                  printf("Set Parity Error\n");
                  //return -1;
      	}
	write(serialfd,rxsockbuff,rxsock_len);


	/************socket init******************/
	sockfd=socket(AF_INET,SOCK_STREAM,0);
	int flags = fcntl(sockfd,F_GETFL,0);
	fcntl(sockfd,F_SETFL,flags|O_NONBLOCK); //noblock
    printf("connect...\n");
	dest_addr.sin_family=AF_INET;
	dest_addr.sin_port=htons(DEST_PORT);
    dest_addr.sin_addr.s_addr=inet_addr(DEST_IP);
    bzero(&(dest_addr.sin_zero),8);
    connect(sockfd,(struct sockaddr *)&dest_addr,sizeof(dest_addr));
    printf("connect ok\n");

	while(1)
	{
		Time_Cnt++;
		if((Time_Cnt&0xff)==0)
		{
			send(sockfd,rec_data,16,0);  //socket send
		}

		rxsock_len = recv(sockfd,rxsockbuff,256,0); //socket rec
		if(rxsock_len>0) //
		{
			//write(serialfd,rxsockbuff,rxsock_len);
			i=0;
			bzero(rxsockbuff,sizeof(rxsockbuff));
			printf("%d\n",rxsock_len);
			rxsock_len=0;
			//break;		
		}
		
		usleep(10000);		
	}
	
	//device_satus(dev_handle);
	return 0;

}
