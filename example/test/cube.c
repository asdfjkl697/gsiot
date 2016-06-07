#include <stdio.h>

int a[400];//存储魔方

void m21(int N)
{
    
    int i;
    int col,row;

    col = (N-1)/2;
    row = 0;

    //a[row][col] = 1;
    a[row*20+col]=1;

    for(i = 2; i <= N*N; i++)
    {
        if((i-1)%N == 0 )
        {
            row++;
        }
        else
        {
            // if row = 0, then row = N-1, or row = row - 1
            row--;
            row = (row+N)%N;

            // if col = N, then col = 0, or col = col + 1
            col ++;
            col %= N;
        }
        a[row*20+col] = i;
    }

    //return a;
}

void m4(int N)
{
    int temparray[N*N/2];//存储取出的元素
    int i;//循环变量
    int col, row;// col 列，row 行

    //初始化
        i = 1;
        for(row = 0;row < N; row++)
        {
            for(col = 0;col < N; col ++)
            {
                a[row*20+col] = i;
                i++;
            }
        }
    //取出子方阵中对角线上的元素，且恰好按从小到大的顺序排放
    i = 0;
    for(row = 0;row < N; row++)
    {
        for(col = 0;col < N; col ++)
        {
             if((col % 4 == row % 4) || ( 3 == ( col % 4 + row % 4)))
            {
                temparray[i] = a[row*20+col];
                i++;
            }
        }
    }
    //将取出的元素按照从大到小的顺序填充到n×n方阵中
    i = N*N/2 -1;
    for(row = 0;row < N; row++)
    {
        for(col = 0;col < N; col ++)
        {
            if((col % 4 == row % 4) || ( 3 == ( col % 4 + row % 4)))
            {
                a[row*20+col] = temparray[i];
                i--;
            }
        }
    }
    //return a;
}

void m42(int N)
{
    int i,k,temp;
    int col,row;// col 列，row 行

    //初始化
    k = N / 2;
    col = (k-1)/2;
    row = 0;
    a[row*20+col] = 1;
    //生成奇魔方A
    for(i = 2; i <= k*k; i++)
    {
        if((i-1)%k == 0 )//前一个数是3的倍数
        {
            row++;
        }
        else
        {
            // if row = 0, then row = N-1, or row = row - 1
            row--;
            row = (row+k)%k;

            // if col = N, then col = 0, or col = col + 1
            col ++;
            col %= k;
        }
        a[row*20+col] = i;
    }

    //根据A生成B、C、D魔方
    for(row = 0;row < k; row++)
    {
        for(col = 0;col < k; col ++)
        {
            a[(row+k)*20+col+k] = a[row*20+col] + k*k;
            a[row*20+col+k] = a[row*20+col] + 2*k*k;
            a[(row+k)*20+col] = a[row*20+col] + 3*k*k;
        }
    }

    // Swap A and C
    for(row = 0;row < k;row++)
    {
        if(row == k / 2)//中间行，交换从中间列向右的m列，N = 2*(2m+1)
        {
            for(col = k / 2; col < k - 1; col++)
            {
                temp = a[row*20+col];
                a[row*20+col] = a[(row+k)*20+col];
                a[(row+k)*20+col] = temp;
            }
        }
        else//其他行，交换从左向右m列,N = 2*(2m+1)
        {
            for(col = 0;col < k / 2;col++)
            {
                temp = a[row*20+col];
                a[row*20+col] = a[(row+k)*20+col];
                a[(row+k)*20+col] = temp;
            }
        }
    }

    // Swap B and D
    for(row = 0; row < k;row++)//交换中间列向左m-1列，N = 2*(2m+1)
    {
        for(i = 0;i < (k - 1)/2 - 1;i++)
        {
            temp = a[row*20+k+ k/2 - i];
            a[row*20+k+ k/2 - i] = a[(row + k)*20+k+k/2 -i];
            a[(row + k)*20+k+k/2 -i] = temp;
        }
    }

    //return a;
}

void cube_func(int NUM)
{
	int i,sum=0;
	int col,row;
	if(NUM%2) m21(NUM);
	else if(NUM%4) m42(NUM);
	else m4(NUM);
        //输出魔方阵
        for(row = 0;row < NUM; row++)
        {
        	for(col = 0;col < NUM; col ++)
        	{
            		//printf("%5d",a[row][col]);
			printf("%5d",a[row*20+col]);
        	}
        	printf("\n");
    	}	

	sum=(NUM*NUM+1)*NUM/2;
	printf("NUM=%d SUM=%d\n",NUM,sum);
}

int main()
{
	int i;
	for(i=3;i<13;i++)
	cube_func(i);
   	return 0;
}
