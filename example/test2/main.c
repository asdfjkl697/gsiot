//注意要在输出结果后面加\n，不然会有格式错误，坑爹啊!!!
#include "stdio.h"
#include "stdlib.h"
#include "math.h"

int main()
{
	double p,q,x1,x2,disc,a,b,c;
	scanf("%lf,%lf,%lf",&a,&b,&c);//特别注意:此处要求输入时a,b,c(三个数)之间用逗号隔开,而不是空格!
	disc=b*b-4*a*c;
	if(disc>0)
	{
		p=-b/(2.0*a);
		q=(sqrt(disc))/(2.0*a);
		x1=p+q;
		x2=p-q;
		printf("x1=%lf\tx2=%lf\n",x1,x2);
	}
	else if(disc==0)//只有此处有错,关系表达式的等于符号是"==",此句改为else if(disc==0);即可
	{
		p=-b/(2.0*a);
		x1=p;
		printf("x1=x2=%lf\n",x1);
	}
	else if(disc<0)
	{
		disc=-disc;
		p=-b/(2.0*a);
		q=(sqrt(disc))/(2.0*a);
		printf("x1=%lf+%lfi\n",p,q);
		printf("x2=%lf-%lfi\n",p,q);
	}
	return 0;
}
