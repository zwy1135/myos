//主程序
#include "bootpack.h"

#include <stdio.h>

void HariMain()
{
	struct BOOTINFO *binfo=(struct BOOTINFO *) ADR_BOOTINFO;
	char s[40],mcursor[256];
	int mx,my;
	
	init_gdtidt();
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
		io_hlt();
	}
}
