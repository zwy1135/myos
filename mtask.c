#include "bootpack.h"

//struct TIMER *mt_timer;
//int mt_tr;
//
//void mt_init()
//{
//	mt_timer = timer_alloc();//没数据要写所以不用init
//	timer_settime(mt_timer, 2);
//	mt_tr = 3 * 8;
//	return;
//}
//
//void mt_taskswitch()
//{
//	mt_tr = (mt_tr == 3 * 8)?4 * 8:3 * 8;
//	timer_settime(mt_timer, 2);
//	farjump(0, mt_tr);
//	return;
//}


struct TIMER *task_timer;
struct TASKCTL *taskctl;

struct TASK* task_init(struct MEMMAN *memman)
{
	int i;
	struct TASK *task;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof(struct TASKCTL));
	for(i=0; i<MAX_TASKS; i++)
	{taskctl->tasks0[i].flags = 0;
		taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
		set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);
	}

	task = task_alloc();
	task->flags = 2;
	task->next = task;
	taskctl->head = task;
	taskctl->nil = task;
	taskctl->current = task;
	load_tr(task->sel);
	task_timer = timer_alloc();
	timer_settime(task_timer, 2);

	return task;

}

struct TASK* task_alloc(void)
{
	int i;
	struct TASK *task;
	for(i=0; i<MAX_TASKS; i++)
	{
		if(taskctl->tasks0[i].flags == 0) //find the free place
		{
			task = &taskctl->tasks0[i];
			task->flags = 1; //used
			task->tss.eflags = 0x00000202; //IF=1
			task->tss.eax = 0;
			task->tss.ecx = 0;
			task->tss.edx = 0;
			task->tss.ebx = 0;
			task->tss.ebp = 0;
			task->tss.esi = 0;
			task->tss.edi = 0;
			task->tss.es = 0;
			task->tss.ds = 0;
			task->tss.fs = 0;
			task->tss.gs = 0;
			task->tss.ldtr = 0;
			task->tss.iomap = 0x40000000;
			return task;
		}
	}

	return NULL; //all place is used.
}



void task_run(struct TASK* task)
{
	if(task == NULL)
		return;
	task->flags = 2;
	task->next = taskctl->head;
	taskctl->nil->next = task;
	taskctl->nil = task;
	return;
}


void task_switch(void)
{
	timer_settime(task_timer, 2);
	taskctl->current = taskctl->current->next;
	farjump(0, taskctl->current->sel);
}
