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

#define PORT_KEYDAT		0x0060
struct FIFO8 keyfifo;

void inthandler21(int *esp)					//处理PS/2键盘的中断
{
	unsigned char data;
	io_out8(PIC0_OCW2, 0x61);				//回复中断响应
	data = io_in8(PORT_KEYDAT);
	fifo8_put(&keyfifo, data);
	return;
}

void inthandler2c(int *esp)					//处理PS/2鼠标的中断
{
	struct BOOTINFO *binfo=(struct BOOTINFO *) ADR_BOOTINFO;
	boxfill8(binfo->vram,binfo->scrnx,COL8_000000,0,0,32*8-1,15);
	putfonts8_asc(binfo->vram,binfo->scrnx,0,0,COL8_FFFFFF,"INT 2C (IRQ 12):PS/2 mouse");
	
	while(1)
	{
		io_hlt();
	}
}

void inthandler27(int *esp)					//处理pic初始化时的IRQ7中断
{
	io_out8(PIC0_OCW2,0X67);
	return;
}
