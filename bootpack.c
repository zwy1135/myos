//主程序
#include "bootpack.h"

#include <stdio.h>

extern struct FIFO8 keyfifo;

void HariMain()
{
	struct BOOTINFO *binfo=(struct BOOTINFO *) ADR_BOOTINFO;
	char s[40],mcursor[256],keybuf[32];
	int mx,my,i;
	
	init_gdtidt();
	init_pic();
	
	io_sti();			//interrupt flag 变为1，开始接受中断
	fifo8_init(&keyfifo, 32, keybuf);	//初始化键盘FIFO
	
	io_out8(PIC0_IMR, 0xf9); /* 1 2 号端口打开，PIC1和键盘启用(11111001) */
	io_out8(PIC1_IMR, 0xef); /* IRQ12打开，鼠标启用(11101111) */
	
	init_palette();
	init_screen8(binfo->vram,binfo->scrnx,binfo->scrny);//画屏幕背景
	mx=(binfo->scrnx-16)/2;		//鼠标定位
	my=(binfo->scrny-16)/2;
	init_mouse_cursor8(mcursor,COL8_008484);
	putblock8_8(binfo->vram,binfo->scrnx,16,16,mx,my,mcursor,16);
	sprintf(s,"(%d,%d)",mx,my);	//写入内存
	putfonts8_asc(binfo->vram,binfo->scrnx,0,0,COL8_FFFFFF,s);//输出mx，my
	
	
	
	while(1)
	{
		io_cli();
		if(fifo8_status(&keyfifo) == 0)
		{
			io_stihlt();
		}
		else
		{
			i = fifo8_get(&keyfifo);
			io_sti();
			sprintf(s,"%02X",i);
			boxfill8(binfo->vram,binfo->scrnx,COL8_008484,0,16,15,31);
			putfonts8_asc(binfo->vram,binfo->scrnx,0,16,COL8_FFFFFF,s);
		}
	}
}
