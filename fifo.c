//有关FIFO的部分
#include "bootpack.h"

//#define FLAGS_OVERRUN	0x0001		不知何用

void fifo8_init(struct FIFO8 *fifo,int size,unsigned char *buf)
{
	fifo->size = size;		//大小
	fifo->buf = buf;
	fifo->free = size;
	fifo->flags = 0;		//可用
	fifo->p = 0;			//下个数据写入位置
	fifo->q = 0;			//下个数据读出位置
	return;
}

int fifo8_put(struct FIFO8 *fifo,unsigned char data)	//输入
{
	if(fifo->free == 0)
	{
		fifo->flags = 1;	//或运算不知何用，改掉了
		return -1;			//溢出
	}
	fifo->buf[fifo->p] = data;
	(fifo->p)++;
	if(fifo->p == fifo->size)
	{
		fifo->p=0;
	}
	(fifo->free)--;
	return 0;				//未溢出
}

int fifo8_get(struct FIFO8 *fifo)	//输出
{
	int data;
	if(fifo->free == fifo->size)
	{
		return -1;			//无数据
	}
	data = fifo->buf[fifo->q];
	(fifo->q)++;
	if(fifo->q == fifo->size)
	{
		fifo->q = 0;
	}
	(fifo->free)++;
	return data;
}

int fifo8_status(struct FIFO8 *fifo)	//已用空间
{
	return fifo->size - fifo->free;
}
