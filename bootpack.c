//主程序
#include "bootpack.h"
#include <stdio.h>

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

void HariMain()
{
	struct BOOTINFO *binfo=(struct BOOTINFO *) ADR_BOOTINFO;
	struct MOUSE_DEC mdec;
	char s[40],keybuf[32],mousebuf[128];
	int mx,my,i;
	unsigned int memtotal;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SHTCTL *shtctl;
	struct SHEET *sht_back,*sht_mouse;
	unsigned char *buf_back,buf_mouse[256];
	
	//初始化部分
	init_gdtidt();
	init_pic();
	
	io_sti();			//interrupt flag 变为1，开始接受中断
	
	fifo8_init(&keyfifo, 32, keybuf);		//初始化键盘FIFO
	fifo8_init(&mousefifo,128,mousebuf);	//初始化鼠标FIFO
	
	io_out8(PIC0_IMR, 0xf9); /* 1 2 号端口打开，PIC1和键盘启用(11111001) */
	io_out8(PIC1_IMR, 0xef); /* IRQ12打开，鼠标启用(11101111) */
	
	init_keybroad();
	enable_mouse(&mdec);
	
	memtotal = memtest(0x00400000,0xbfffffff);
	memman_init(memman);
	memman_free(memman,0x00001000,0x0009e000);			//这块地址是什么情况？？？
	memman_free(memman,0x00400000, memtotal - 0x00400000);
	
	init_palette();
	
	shtctl = shtctl_init(memman,binfo->vram,binfo->scrnx,binfo->scrny);
	sht_back = sheet_alloc(shtctl);
	sht_mouse = sheet_alloc(shtctl);
	buf_back = (unsigned char *)memman_alloc_4k(memman,binfo->scrnx * binfo->scrny);
	sheet_setbuf(sht_back,buf_back,binfo->scrnx,binfo->scrny,-1);	//背景无透明色
	sheet_setbuf(sht_mouse,buf_mouse,16,16,99);		//透明色为99
	
	//以下是显示的内容
	init_screen8(buf_back,binfo->scrnx,binfo->scrny);//画屏幕背景
	init_mouse_cursor8(buf_mouse,99);//透明色99
	sheet_slide(shtctl,sht_back,0,0);
	mx=(binfo->scrnx- 16)/2;		//鼠标定位
	my=(binfo->scrny- 28 - 16)/2;
	
	sheet_slide(shtctl,sht_mouse,mx,my);
	sheet_updown(shtctl,sht_back,0);
	sheet_updown(shtctl,sht_mouse,1);
	
	sprintf(s,"(%3d,%3d)",mx,my);	//写入内存
	putfonts8_asc(buf_back,binfo->scrnx,0,0,COL8_FFFFFF,s);//输出mx，my
	
	//内存信息
	sprintf(s,"memory %d MB , free %d KB , lost %d KB",memtotal / (1024*1024),memman_total(memman) / 1024,memman->lostsize / 1024);
	putfonts8_asc(buf_back,binfo->scrnx,0,32,COL8_FFFFFF,s);
	sheet_refresh(shtctl);
	
	
	
	while(1)
	{
		io_cli();
		if(fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0)
		{
			io_stihlt();
		}
		else
		{
			if(fifo8_status(&keyfifo) != 0)
			{
				i = fifo8_get(&keyfifo);
				io_sti();
				sprintf(s,"%02X",i);
				boxfill8(buf_back,binfo->scrnx,COL8_008484,0,16,15,31);
				putfonts8_asc(buf_back,binfo->scrnx,0,16,COL8_FFFFFF,s);
				sheet_refresh(shtctl);
			}
			else if(fifo8_status(&mousefifo) != 0)
			{
				i = fifo8_get(&mousefifo);
				io_sti();
				if((mouse_decode(&mdec,i)) != 0)
				{
					sprintf(s,"[lcr %4d %4d]",mdec.x,mdec.y);
					if((mdec.btn & 0x01) != 0)							//第一位左键
					{
						s[1] = 'L';
					}
					if((mdec.btn & 0x02) != 0)							//第二位右键
					{
						s[3] = 'R';
					}
					if((mdec.btn & 0x04) != 0)							//第三位中键
					{
						s[2] = 'C';
					}
					boxfill8(buf_back,binfo->scrnx,COL8_008484,32,16,32+15*8-1,31);		//与键盘显示区错开
					putfonts8_asc(buf_back,binfo->scrnx,32,16,COL8_FFFFFF,s);
					//移动鼠标的部分
					//TODO:FROM THIS;
					mx += mdec.x;
					my += mdec.y;
					//防止越出边界
					if(mx < 0)
						mx = 0;
					if(my < 0)
						my = 0;
					if(mx > binfo->scrnx - 16)
						mx = binfo->scrnx - 16;
					if(my > binfo->scrny - 16)
						my = binfo->scrny - 16;
						
					sprintf(s,"(%3d,%3d)",mx,my);								//写入内存
					boxfill8(buf_back,binfo->scrnx,COL8_008484,0,0,79,15);	//抹去原数字
					putfonts8_asc(buf_back,binfo->scrnx,0,0,COL8_FFFFFF,s);	//输出mx，my
					sheet_slide(shtctl,sht_mouse,mx,my);
				}
				
			}
		}
	}
}

