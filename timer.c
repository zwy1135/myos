#include "bootpack.h"

#define	PIT_CTRL	0x0043
#define	PIT_CNT0	0x0040

#define TIMER_FLAGS_FREE	0
#define TIMER_FLAGS_ALLOC	1
#define TIMER_FLAGS_USING	2

struct TIMERCTL timerctl;

void init_pit()
{	
	int i;
	io_out8(PIT_CTRL,0x34);
	io_out8(PIT_CNT0,0x9c);
	io_out8(PIT_CNT0,0x2e);
	timerctl.count = 0;
	timerctl.next = 0xffffffff;
	timerctl.using = 0;

	for (i = 0; i < MAX_TIMER; ++i)
	{
		timerctl.timers0[i].flags = TIMER_FLAGS_FREE;
	}
	return;
}

void inthandler20(int *esp)
{
	int i,j;
	io_out8(PIC0_OCW2,0x60);
	timerctl.count++;

	if (timerctl.next > timerctl.count)
	{
		return;
	}
	//此处timers一定是顺序排列的，排前面的一定先timeout
	for (i = 0; i < timerctl.using; ++i)
	{
		if (timerctl.timers[i]->timeout > timerctl.count)
			break;

		timerctl.timers[i]->flags = TIMER_FLAGS_ALLOC;
		fifo32_put(timerctl.timers[i]->fifo,timerctl.timers[i]->data);
	}

	timerctl.using -= i;

	for(j = 0;j < timerctl.using; j++)
	{
		timerctl.timers[j] = timerctl.timers[i + j];
	}

	timerctl.next = (timerctl.using > 0)? timerctl.timers[0]->timeout : 0xffffffff;
	return;
}

//void settimer(unsigned int timeout,struct FIFO8 *fifo,unsigned char data)
//{
//	int eflags = io_load_eflags();
//	io_cli();
//	timerctl.timeout = timeout;
//	timerctl.fifo = fifo;
//	timerctl.data = data;
//	io_store_eflags(eflags);
//	io_sti();
//	return;
//}

struct TIMER *timer_alloc(void)
{
	int i;
	for (i = 0; i < MAX_TIMER; ++i)
	{
		if (timerctl.timers0[i].flags == TIMER_FLAGS_FREE)
		{
			timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
			return &timerctl.timers0[i];
		}
	}
	return 0;
}

void timer_free(struct TIMER *timer)
{
	timer->flags = TIMER_FLAGS_FREE;
	return;
}

void timer_init(struct TIMER *timer,struct FIFO32 *fifo,int data)
{
	timer->fifo = fifo;
	timer->data = data;
	return;
}

void timer_settime(struct TIMER *timer,unsigned int timeout)
{
	int e, i, j;
	timer->timeout = timerctl.count + timeout;
	timer->flags = TIMER_FLAGS_USING;
	e = io_load_eflags();
	io_cli();
	//二分搜索该插入的位置
	int low,high,mid;
	low = 0;
	high = timerctl.using - 1;
	while(low < high)
	{
		mid = (low + high)/2;
		if (timerctl.timers[mid]->timeout == timer->timeout)
		{
			low = mid;
			break;
		}
		else if(timerctl.timers[mid]->timeout < timer->timeout)
			low = mid + 1;
		else 
		{
			high = mid - 1;
		}
	}
	i = low;
	//后移一位，腾出位置
	for(j = timerctl.using;j > i;j--)
	{
		timerctl.timers[j] = timerctl.timers[j - 1];
	}
	timerctl.using ++ ;
	//插入
	timerctl.timers[i] = timer;
	timerctl.next = timerctl.timers[0]->timeout;
	io_store_eflags(e);//eflags带有中断标志位，所以不用sti
	return;
}

