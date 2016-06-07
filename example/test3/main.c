#include "stdio.h"
#include "stdlib.h"
#include "math.h"
main()
{
	float a,b,c,d;
	float x0,x1,x2,f0,f1,f2;
	//printf("请输入方程系数 a,b,c,d;");
	//scanf("%f,%f,%f,%f",&a,&b,&c,&d);
	a=1;b=-3;c=-1;d=3;
	do
	{
		//printf("请输入变量的有效范围 x1,x2:");
		//scanf("%f,%f",&x1,&x2);
		x1=-100.0;x2=100,0;
		f1=a*x1*x1*x1+b*x1*x1+c*x1+d;
		if(f1==0){x0=x1;goto loop;}
		f2=a*x2*x2*x2+b*x2*x2+c*x2+d;;
		if(f2==0){x0=x2;goto loop;}
	}while(f1*f2>0);
	
	do
	{
		x0=(x1+x2)/2;
		f0=a*x0*x0*x0+b*x0*x0+c*x0+d;
		if(f0==0)break;
		if(f0*f1<0)
		{x2=x0;f2=f0;}
		else
		{x1=x0;f1=f0;}
	}while(fabs(f0)>=1e-5);
	loop:printf("方程的根 x=%f\n",x0);
}
