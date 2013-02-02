//主程序
#include "bootpack.h"
#include <stdio.h>

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

unsigned int memtest(unsigned int start,unsigned int end);

void HariMain()
{
	struct BOOTINFO *binfo=(struct BOOTINFO *) ADR_BOOTINFO;
	struct MOUSE_DEC mdec;
	char s[40],mcursor[256],keybuf[32],mousebuf[128];
	int mx,my,i;
	
	init_gdtidt();
	init_pic();
	
	io_sti();			//interrupt flag 变为1，开始接受中断
	
	fifo8_init(&keyfifo, 32, keybuf);		//初始化键盘FIFO
	fifo8_init(&mousefifo,128,mousebuf);	//初始化鼠标FIFO
	
	io_out8(PIC0_IMR, 0xf9); /* 1 2 号端口打开，PIC1和键盘启用(11111001) */
	io_out8(PIC1_IMR, 0xef); /* IRQ12打开，鼠标启用(11101111) */
	
	init_keybroad();
	enable_mouse(&mdec);
	
	init_palette();
	init_screen8(binfo->vram,binfo->scrnx,binfo->scrny);//画屏幕背景
	mx=(binfo->scrnx-16)/2;		//鼠标定位
	my=(binfo->scrny-16)/2;
	init_mouse_cursor8(mcursor,COL8_008484);
	putblock8_8(binfo->vram,binfo->scrnx,16,16,mx,my,mcursor,16);
	sprintf(s,"(%d,%d)",mx,my);	//写入内存
	putfonts8_asc(binfo->vram,binfo->scrnx,0,0,COL8_FFFFFF,s);//输出mx，my
	
	//内存检查结果
	i = memtest(0x00400000,0xbfffffff) / (1024 * 1024);
	sprintf(s,"memory %d MB",i);
	putfonts8_asc(binfo->vram,binfo->scrnx,0,32,COL8_FFFFFF,s);
	
	
	
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
				boxfill8(binfo->vram,binfo->scrnx,COL8_008484,0,16,15,31);
				putfonts8_asc(binfo->vram,binfo->scrnx,0,16,COL8_FFFFFF,s);
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
					boxfill8(binfo->vram,binfo->scrnx,COL8_008484,32,16,32+15*8-1,31);		//与键盘显示区错开
					putfonts8_asc(binfo->vram,binfo->scrnx,32,16,COL8_FFFFFF,s);
					//移动鼠标的部分
					boxfill8(binfo->vram,binfo->scrnx,COL8_008484,mx,my,mx+16,my+16);		//擦除原鼠标
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
					boxfill8(binfo->vram,binfo->scrnx,COL8_008484,0,0,79,15);	//抹去原数字
					putfonts8_asc(binfo->vram,binfo->scrnx,0,0,COL8_FFFFFF,s);	//输出mx，my
					
					putblock8_8(binfo->vram,binfo->scrnx,16,16,mx,my,mcursor,16);	//重画鼠标
				}
				
			}
		}
	}
}

//内存检查
#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

unsigned int memtest(unsigned int start,unsigned int end)
{
	char flag486 = 0;
	unsigned int eflag,cr0,i;
	//确认是386还是486
	eflag = io_load_eflags();
	eflag |= EFLAGS_AC_BIT;				//设定AC_BIT = 1,386设完后会自动归零
	io_store_eflags(eflag);
	eflag = io_load_eflags();
	if((eflag & EFLAGS_AC_BIT) != 0)	//判断AC_BIT是否为1
	{
		flag486 = 1;
	}
	eflag &= ~EFLAGS_AC_BIT;			//AC_BIT = 0
	io_store_eflags(eflag);
	
	if(flag486 != 0)
	{
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE;		//禁止缓存
		store_cr0(cr0);
	}
	
	i = memtest_sub(start,end);
	
	if(flag486 != 0)
	{
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE;		//启用缓存
		store_cr0(cr0);
	}
	return i;
}

