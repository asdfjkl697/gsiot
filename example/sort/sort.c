#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int main()
{
	int a[100],i,j,k;
	srand((unsigned int)time(NULL));//设置当前时间为种子
	for (i = 0; i < 100; ++i){
	        a[i] = rand()%10000;//产生1~10000的随机数
    	}

	for(i=1;i<100;i++){
		k=a[i];
		j=i-1;
		while((j>=0)&&(a[j]>k)){
			a[j+1]=a[j];
			j--;
		}
		a[j+1]=k;
	}

    	//打印生成的随机数
    	for (i = 0; i < 100; ++i){
        	printf ("%5d ", a[i]);
		if(i%10==9)printf("\n");
    	}

    	printf ("\n");
    	return 0;
}
