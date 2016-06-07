#include<stdio.h>
#include<math.h>
int a,b,c,d;
int count=0;
double x[3]={-200,-200,-200};
void fun(double x1,double x2)
{
	double x3,y1,y2,y3;
	int i=0;
	 
	 while (x2-x1>10e-6)
	 {
		x3=(x1+x2)/2;
		y1=a*x1*x1*x1+b*x1*x1+c*x1+d;
        	y2=a*x2*x2*x2+b*x2*x2+c*x2+d;
		y1=a*x1*x1*x1+b*x1*x1+c*x1+d;
        	y3=a*x3*x3*x3+b*x3*x3+c*x3+d;
		if (y1==0)
		{
			for (i=0;i<3;i++)
			{
				if (x[i]==x1)
				{
					return;
				}
				if (x[i]==-200)
				{
					count++;
					x[i]=x1;
					return;
				}
				
			}
		}
		else if (y2==0)
		{
			for (i=0;i<3;i++)
			{
				if (x[i]==x2)
				{
					return;
				}
				if (x[i]==-200)
				{
					count++;
					x[i]=x2;
					return;
				}
			}
		}
		else if (y3==0)
		{
			for (i=0;i<3;i++)
			{
				if (x[i]==x3)
				{
					return;
				}
				if (x[i]==-200)
				{
					count++;
					x[i]=x3;
					return;
				}
			}
		}
		else if (y1*y3<0)
		{
			x2=x3;
		}
		else
		{
			x1=x3;
		}
	 }
	 for (i=0;i<3;i++)
	 {
		 if (x[i]==x1)
		 {
			 return;
		 }
		 if (x[i]==-200)
		 {
			 count++;
			 x[i]=x1;
			 return;
		}
	 }

}
int main()
{
	int i;
	double x1,x2,y1,y2,x3,x4;
	scanf("%d,%d,%d,%d",&a,&b,&c,&d);
	for (x1=-100,x2=x1+1;x1<100;x1+=1,x2+=1)
	{
		y1=a*x1*x1*x1+b*x1*x1+c*x1+d;
        	y2=a*x2*x2*x2+b*x2*x2+c*x2+d;
        	x3=x1;
		x4=x2;
		if (y1*y2<=0)
		{
			fun(x3,x4);
			if (count==3)
			{
				break;
			}
		}
	}
	for(i=0;i<3;i++)
	{
		printf("%.2lf ",x[i]);
	}
	printf("\n");
	return 0;
}
