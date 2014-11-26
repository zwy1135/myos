#include "bootpack.h"

struct TIMER *mt_timer;
int mt_tr;

void mt_init()
{
	mt_timer = timer_alloc();//没数据要写所以不用init
	timer_settime(mt_timer, 2);
	mt_tr = 3 * 8;
	return;
}

void mt_taskswitch()
{
	mt_tr = (mt_tr == 3 * 8)?4 * 8:3 * 8;
	timer_settime(mt_timer, 2);
	farjump(0, mt_tr);
	return;
}