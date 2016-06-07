#include     	<stdio.h>      /*  */
#include	<stdio.h>
#include     	<unistd.h>     /* */
#include     	<sys/types.h> 
#include     	<sys/stat.h>  
#include     	<fcntl.h>      /* */
#include     	<termios.h>    /* */
#include     	<errno.h>      /* */
#include        <string.h>

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
