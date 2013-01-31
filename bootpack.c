//主程序
#include "bootpack.h"

#include <stdio.h>

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;
void init_keybroad();

struct MOUSE_DEC
{
	unsigned char buf[3],phase;
	int x,y,btn;						//存放鼠标移动和点击状态的变量
};

void enable_mouse(struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec,unsigned char dat);

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
	
	init_palette();
	init_screen8(binfo->vram,binfo->scrnx,binfo->scrny);//画屏幕背景
	mx=(binfo->scrnx-16)/2;		//鼠标定位
	my=(binfo->scrny-16)/2;
	init_mouse_cursor8(mcursor,COL8_008484);
	putblock8_8(binfo->vram,binfo->scrnx,16,16,mx,my,mcursor,16);
	sprintf(s,"(%d,%d)",mx,my);	//写入内存
	putfonts8_asc(binfo->vram,binfo->scrnx,0,0,COL8_FFFFFF,s);//输出mx，my
	
	enable_mouse(&mdec);
	
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

#define PORT_KEYDAT				0x0060
#define PORT_KEYSTA				0x0064
#define PORT_KEYCMD				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47

void wait_KBC_sendready()			//等待，直到准备完成
{
	while(1)
	{
		if((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0)	//第二位是0则准备好
		{
			break;
		}
	}
	return;
}

void init_keybroad()				//键鼠控制电路初始化
{
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
	return;
}

#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4

void enable_mouse(struct MOUSE_DEC *mdec)					//激活鼠标
{
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	//顺利的话，键盘控制器会返回ACK（0xfa）??//ACK=0xfa
	mdec->phase = 0;				//等待0xfa被送来
	return;
}

int mouse_decode(struct MOUSE_DEC *mdec,unsigned char dat)		//解读鼠标信息的函数
{
	if(mdec->phase == 0)					//初始状态，收到0xfa时进入工作状态
	{
		if(dat == 0xfa)
			mdec->phase = 1;
		return 0;
	}
	//，工作状态，连续接受三个信号加入buf中,成功后返回1
	if(mdec->phase == 1)					
	{
		if((dat & 0xc8) == 0x08)				//检查第一位数据，防止数据错位，错了也能在下次循环中自动归位
		{
			mdec->buf[0] = dat;
			mdec->phase = 2;
		}
		return 0;
	}
	if(mdec->phase == 2)
	{
		mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	}
	if(mdec->phase == 3)
	{
		mdec->buf[2] = dat;
		mdec->phase = 1;
		mdec->btn = mdec->buf[0] & 0x07;	//检查点击状态
		mdec->x = mdec->buf[1];				//x轴上的相对速度
		mdec->y = mdec->buf[2];				//同上
		
		if((mdec->buf[0] & 0x10) != 0)
		{
			mdec->x |= 0xffffff00;			//后8位是无用的，故不保留
		}
		if((mdec->buf[0] & 0x20) != 0)
		{
			mdec->y |= 0xffffff00;			//同上
		}
		mdec->y = - mdec->y;				//y轴反向，因为屏幕的y轴是在之前的代码中设计为从上到下的
		return 1;
	}
	return -1;								//错误信号，应该不会出现
}

