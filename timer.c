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
	struct TIMER *timer;

	io_out8(PIC0_OCW2,0x60);
	timerctl.count++;

	if (timerctl.next > timerctl.count)
	{
		return;
	}
	
	timer = timerctl.t0;
	for (i = 0; i < timerctl.using; ++i)
	{
		if (timer->timeout > timerctl.count)
			break;

		timer->flags = TIMER_FLAGS_ALLOC;
		fifo32_put(timer->fifo,timer->data);
		timer = timer->next;
	}

	timerctl.using -= i;

	timerctl.t0 = timer;

	timerctl.next = (timerctl.using > 0)? timerctl.t0->timeout : 0xffffffff;
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
	int e;
	struct TIMER *t;
	timer->timeout = timerctl.count + timeout;
	timer->flags = TIMER_FLAGS_USING;
	e = io_load_eflags();
	io_cli();
	
	timerctl.using ++;
	//插入空队列
	if(timerctl.using == 1)
	{
		timerctl.t0 = timer;
		timerctl.next = timer->timeout;
		timer->next = NULL;
		io_store_eflags(e);
		return;
	}

	//插入非空队列
	t = timerctl.t0;
	//插入最前
	if(timer->timeout < t->timeout)
	{
		timer->next = t;
		timerctl.t0 = timer;
		timerctl.next = timer->timeout;
		io_store_eflags(e);
		return;
	}
	else
	{
		for(;t->next != NULL && t->next->timeout < timer->timeout;t = t->next);

		timer->next = t->next;
		t->next = timer;
		io_store_eflags(e);
		return;
	}

	return;
}

