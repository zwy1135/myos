#include "bootpack.h"
#include <stdio.h>

void init_pic()
{
	io_out8(PIC0_IMR,	0xff	);			//禁止所有中断，防止出错
	io_out8(PIC1_IMR,	0xff	);			//同上
	
	io_out8(PIC0_ICW1,	0x11	);			//边缘触发模式？
	io_out8(PIC0_ICW2,	0x20	);			//IRQ0-7由INT20-27接收
	io_out8(PIC0_ICW3,	1 << 2	);			//PIC1由IRQ2链接
	io_out8(PIC0_ICW4,	0x01	);			//无缓冲模式
	
	io_out8(PIC1_ICW1,	0x11	);			//同上
	io_out8(PIC1_ICW2,	0x28	);			//IRQ8-15由INT28-2f接收
	io_out8(PIC1_ICW3,	2		);			//PIC1由IRQ2链接
	io_out8(PIC1_ICW4,	0x01	);			//无缓冲模式
	
	io_out8(PIC0_IMR,	0xfb	);			//11111011出PIC1外全禁止
	io_out8(PIC1_IMR,	0xff	);			//全禁止
	
	return;
}

void inthandler27(int *esp)					//处理pic初始化时的IRQ7中断
{
	io_out8(PIC0_OCW2,0X67);
	return;
}
