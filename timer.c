#include "bootpack.h"

#define	PIT_CTRL	0x0043
#define	PIT_CNT0	0x0040

#define TIMER_FLAGS_FREE	0
#define TIMER_FLAGS_ALLOC	1
#define TIMER_FLAGS_USING	2

struct TIMERCTL timerctl;
extern struct TIMER *task_timer;

void init_pit()
{	
	int i;
	struct TIMER *t;
	io_out8(PIT_CTRL,0x34);
	io_out8(PIT_CNT0,0x9c);
	io_out8(PIT_CNT0,0x2e);
	timerctl.count = 0;

	for (i = 0; i < MAX_TIMER; ++i)
	{
		timerctl.timers0[i].flags = TIMER_FLAGS_FREE;
	}

	t = timer_alloc();
	timer_init(t,NULL,0);
	//settime
	t->timeout = 0xffffffff;
	t->flags = TIMER_FLAGS_USING;
	t->next = NULL;

	timerctl.t0 = t;
	timerctl.next = t->next;

	return;
}

void inthandler20(int *esp)
{
	struct TIMER *timer;
	char ts = 0;//居然没有bool，我也是醉了

	io_out8(PIC0_OCW2,0x60);
	timerctl.count++;

	if (timerctl.next > timerctl.count)
	{
		return;
	}
	
	timer = timerctl.t0;
	for (;;)
	{
		if (timer->timeout > timerctl.count)
			break;

		timer->flags = TIMER_FLAGS_ALLOC;
		if(timer != task_timer) 
		{
			fifo32_put(timer->fifo,timer->data);
		}
		else
		{
			ts = 1;
		}
		timer = timer->next;
	}


	timerctl.t0 = timer;
	timerctl.next = timerctl.t0->timeout;
	if(ts != 0)
	{
		task_switch();
	}
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
	return NULL;
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
	
	//插入非空队列,有哨兵绝对非空
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

