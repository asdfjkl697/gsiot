#include     	<stdio.h>      /*  */
#include	<stdio.h>
#include     	<unistd.h>     /* */
#include     	<sys/types.h> 
#include     	<sys/stat.h>  
#include     	<fcntl.h>      /* */
#include     	<termios.h>    /* */
#include     	<errno.h>      /* */
#include        <string.h>
#include	<sys/socket.h>  //20140823  
#include     	<arpa/inet.h>
#include     	<netinet/in.h>

#define DEST_IP "192.168.0.73"
#define DEST_PORT 5012

#define FALSE  -1
#define TRUE   0

/**
*/
int speed_arr[] = { B38400, B19200, B9600, B4800, B2400, B1200, B300,};
                  // B38400, B19200, B9600, B4800, B2400, B1200, B300, };
int name_arr[] = {38400,  19200,  9600,  4800,  2400,  1200,  300,}; 
		//38400, 19200,  9600, 4800, 2400, 1200,  300, };
void set_speed(int fd, int speed)
{
	 int   i;
         int   status;
         struct termios   Opt;
         tcgetattr(fd, &Opt);
         for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++) 
         {
        	 if  (speed == name_arr[i])
		 {    
                            tcflush(fd, TCIOFLUSH);    
                            cfsetispeed(&Opt, speed_arr[i]); 
                            cfsetospeed(&Opt, speed_arr[i]);  
                            status = tcsetattr(fd, TCSANOW, &Opt); 
                            if  (status != 0) 
			    {       
                                     perror("tcsetattr fd"); 
                                     return;    
                            }   
                            tcflush(fd,TCIOFLUSH);  
                   } 
         }
}
void set_btl(int fd)
{
	int   i,BTL=B9600;
        int   status;
        struct termios   Opt;
        tcgetattr(fd, &Opt);
       	tcflush(fd, TCIOFLUSH);    
        cfsetispeed(&Opt, BTL); 
        cfsetospeed(&Opt, BTL);  
        status = tcsetattr(fd, TCSANOW, &Opt); 
	if  (status != 0) 
	{     
        	perror("tcsetattr fd"); 
                return;    
        }   
        tcflush(fd,TCIOFLUSH);  
}
/**
*/
int set_Parity(int fd,int databits,int stopbits,int parity)
{
         struct termios options;
         if  ( tcgetattr( fd,&options)  !=  0) {
                   perror("SetupSerial 1");    
                   return(FALSE); 
         }
         options.c_cflag &= ~CSIZE;
         options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
         options.c_oflag  &= ~OPOST;   /*Output*/
 
         switch (databits) /*  */
         {   
         case 7:                
                   options.c_cflag |= CS7;
                   break;
         case 8:    
                   options.c_cflag |= CS8;
                   break;  
         default:   
                   fprintf(stderr,"Unsupported data size\n"); return (FALSE); 
         }
	switch (parity)
	{  
         case 'n':
         case 'N':   
                   options.c_cflag &= ~PARENB;   /* Clear parity enable */
                   options.c_iflag &= ~INPCK;     /* Enable parity checking */
                   break; 
         case 'o':  
         case 'O':    
                   options.c_cflag |= (PARODD | PARENB); /* ÉèÖÃÎªÆæÐ§Ñé */ 
                   options.c_iflag |= INPCK;             /* Disnable parity checking */
                   break; 
         case 'e': 
         case 'E':  
                   options.c_cflag |= PARENB;     /* Enable parity */   
                   options.c_cflag &= ~PARODD;   /* ×ª»»ÎªÅŒÐ§Ñé */    
                   options.c_iflag |= INPCK;       /* Disnable parity checking */
                   break;
         case 'S':
         case 's':  /*as no parity*/  
             options.c_cflag &= ~PARENB;
                   options.c_cflag &= ~CSTOPB;break; 
         default:  
                   fprintf(stderr,"Unsupported parity\n");   
                   return (FALSE); 
         } 
	/*stop  */ 
	switch (stopbits)
	{  
         case 1:   
                   options.c_cflag &= ~CSTOPB; 
                   break; 
         case 2:   
                   options.c_cflag |= CSTOPB; 
            break;
         default:   
                    fprintf(stderr,"Unsupported stop bits\n"); 
                    return (FALSE);
	}
	/* Set input parity option */
	if (parity != 'n')  
         options.c_iflag |= INPCK;
	tcflush(fd,TCIFLUSH);
	options.c_cc[VTIME] = 0; /* ÉèÖÃ³¬Ê± 15 seconds*/  
	options.c_cc[VMIN] = 13; /* define the minimum bytes data to be readed*/
	if (tcsetattr(fd,TCSANOW,&options) != 0)  
	{
        	perror("SetupSerial 3");  
         	return (FALSE); 
	}
	return (TRUE); 
}
/**********************************************************************
**********************************************************************/
 
/*********************************************************************/
int OpenDev(char *Dev)
{
         int     fd = open( Dev, O_RDWR|O_NDELAY);
        //| O_NOCTTY | O_NDELAY         
         if (-1 == fd)        
         {                        
                   perror("Can't Open Serial Port");
                   return -1;            
         }      
         else  
                   return fd;
}

int main(void)
{
	unsigned int  testcnt=0;
	unsigned char rxsockbuff[100]={0xca,0,0,0x20,0xbb,0,1,0,0,0xdc};
        unsigned char tmpbuff[20];
	unsigned char rxbuff[100]={0};
	int len;
	int sockfd;
	struct sockaddr_in dest_addr;
	int dest_len = sizeof(struct sockaddr_in);
	int rxsock_len=10;

	int serialfd;
        int nread,count=0;
        char *dev  = "/dev/ttyATH0"; 

	serialfd = OpenDev(dev);
        set_speed(serialfd,115200); //failed 20140916
	if (set_Parity(serialfd,8,1,'N') == FALSE)  {
                  printf("Set Parity Error\n");
                  //return -1;
      	}
	write(serialfd,rxsockbuff,rxsock_len);

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
		if(++testcnt>99999)
			testcnt=0;
		if((testcnt&0x0000003f)==0) //64*50ms=3s
		{
			//printf("testcnt=%d\n",testcnt);		 				
			if((testcnt&0x0000007f)==0)			
				system("echo 1 >/sys/class/gpio/gpio18/value");
			else
				system("echo 0 >/sys/class/gpio/gpio18/value");
			//memcpy(testcnt,&rxbuff,4); 
			rxbuff[0]=0x30+testcnt/10000;
			rxbuff[1]=0x30+(testcnt%10000)/1000;
			rxbuff[2]=0x30+(testcnt%1000)/100; 
			rxbuff[3]=0x30+(testcnt%100)/10; 
			rxbuff[4]=0x30+testcnt%10; 
			rxbuff[5]=0xd;
			rxbuff[6]=0xa;			
			send(sockfd,rxbuff,7,0);
		}
	
		rxsock_len = recv(sockfd,rxsockbuff,100,0);
		if(rxsock_len>0 && rxsockbuff[rxsock_len-1]==0x38)
		{
			write(serialfd,rxsockbuff,rxsock_len);
			printf("\n %s",rxsockbuff); //forward
			printf("\n recvlen: %d\n",rxsock_len);	
			bzero(rxsockbuff,sizeof(rxsockbuff));
			rxsock_len=0;
			//break;		
		}


		if((nread = read(serialfd, tmpbuff, 24))>0)
		{
			memcpy(&rxbuff[count],tmpbuff,nread);
			count+=nread;
		}
                if(count >= 24 && rxbuff[count-3]==0x7b)
                {
                       	rxbuff[count+1] = '\0';  
			send(sockfd,rxbuff,count,0);
                     	printf("\n readlen: %d", count);
			count=0;
                        //break;
                }

		usleep(50000); //50ms
	}
	close(serialfd);
	pthread_exit(NULL);
	return 0;
}
