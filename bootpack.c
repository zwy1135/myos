//主程序
#include "bootpack.h"
#include <stdio.h>

void make_window8(unsigned char *buf, int xsize, int ysize, char *title);

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;
extern struct TIMERCTL timerctl;

void HariMain()
{
	struct BOOTINFO *binfo=(struct BOOTINFO *) ADR_BOOTINFO;
	struct MOUSE_DEC mdec;
	char s[40],keybuf[32],mousebuf[128];
	int mx,my,i;
	unsigned int memtotal,count = 0;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SHTCTL *shtctl;
	struct SHEET *sht_back,*sht_mouse,*sht_win,*sht_win_counter;
	unsigned char *buf_back,buf_mouse[256],*buf_win,*buf_win_counter;
	
	//初始化部分
	init_gdtidt();
	init_pic();
	
	io_sti();			//interrupt flag 变为1，开始接受中断
	
	fifo8_init(&keyfifo, 32, keybuf);		//初始化键盘FIFO
	fifo8_init(&mousefifo,128,mousebuf);	//初始化鼠标FIFO
	
	init_pit();
	io_out8(PIC0_IMR, 0xf8); /* 0 1 2 号端口打开，PIT PIC1和键盘启用(11111000) */
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
	sht_win = sheet_alloc(shtctl);
	sht_win_counter = sheet_alloc(shtctl);
	buf_back = (unsigned char *)memman_alloc_4k(memman,binfo->scrnx * binfo->scrny);
	buf_win = (unsigned char *)memman_alloc_4k(memman,160 * 68);
	buf_win_counter = (unsigned char *)memman_alloc_4k(memman,160 * 52);
	sheet_setbuf(sht_back,buf_back,binfo->scrnx,binfo->scrny,-1);	//背景无透明色
	sheet_setbuf(sht_mouse,buf_mouse,16,16,99);		//透明色为99
	sheet_setbuf(sht_win,buf_win,160,68,-1);		//无透明色
	sheet_setbuf(sht_win_counter,buf_win_counter,160,52,-1);
	
	//以下是显示的内容
	init_screen8(buf_back,binfo->scrnx,binfo->scrny);//画屏幕背景
	init_mouse_cursor8(buf_mouse,99);//透明色99
	make_window8(buf_win,160,68,"A window");
	putfonts8_asc(buf_win,160,24,28,COL8_000000,"That's os of");
	putfonts8_asc(buf_win,160,50,44,COL8_000000,"z wy");
	make_window8(buf_win_counter,160,52,"timer");
	
	sheet_slide(sht_back,0,0);
	mx=(binfo->scrnx- 16)/2;		//鼠标定位
	my=(binfo->scrny- 28 - 16)/2;
	
	sheet_slide(sht_mouse,mx,my);
	sheet_updown(sht_back,0);
	sheet_updown(sht_mouse,3);
	sheet_updown(sht_win,1);
	sheet_updown(sht_win_counter,2);
	sheet_slide(sht_win,0,72);
	sheet_slide(sht_win_counter,160,72);
	
	sprintf(s,"(%3d,%3d)",mx,my);	//写入内存
	putfonts8_asc(buf_back,binfo->scrnx,0,0,COL8_FFFFFF,s);//输出mx，my
	
	//内存信息
	sprintf(s,"memory %d MB , free %d KB , lost %d KB",memtotal / (1024*1024),memman_total(memman) / 1024,memman->lostsize / 1024);
	putfonts8_asc(buf_back,binfo->scrnx,0,32,COL8_FFFFFF,s);
	sheet_refresh(sht_back,0,0,binfo->scrnx,96);		//因为上面sheet_slide刷新过一次，所以只刷新到48
	
	
	
	while(1)
	{
		sprintf(s,"%010d",timerctl.count);
		boxfill8(buf_win_counter,160,COL8_C6C6C6,40,28,119,43);
		putfonts8_asc(buf_win_counter,160,40,28,COL8_000000,s);
		sheet_refresh(sht_win_counter,40,28,120,44);
		
		io_cli();
		if(fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0)
			io_sti();
		else
		{
			if(fifo8_status(&keyfifo) != 0)
			{
				i = fifo8_get(&keyfifo);
				io_sti();
				sprintf(s,"%02X",i);
				boxfill8(buf_back,binfo->scrnx,COL8_008484,0,16,15,31);
				putfonts8_asc(buf_back,binfo->scrnx,0,16,COL8_FFFFFF,s);
				sheet_refresh(sht_back,0,16,16,32);		//只刷新键盘信息区
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
					sheet_refresh(sht_back,32,16,32+15*8-1,32);		//刷新鼠标动作信息区
					//移动鼠标的部分
					mx += mdec.x;
					my += mdec.y;
					//防止越出边界
					if(mx < 0)
						mx = 0;
					if(my < 0)
						my = 0;
					if(mx > binfo->scrnx - 1)
						mx = binfo->scrnx - 1;
					if(my > binfo->scrny - 1)
						my = binfo->scrny - 1;
						
					sprintf(s,"(%3d,%3d)",mx,my);								//写入内存
					boxfill8(buf_back,binfo->scrnx,COL8_008484,0,0,79,15);	//抹去原数字
					putfonts8_asc(buf_back,binfo->scrnx,0,0,COL8_FFFFFF,s);	//输出mx，my
					sheet_refresh(sht_back,0,0,80,16);
					sheet_slide(sht_mouse,mx,my);
				}
				
			}
		}
	}
}

void make_window8(unsigned char *buf, int xsize, int ysize, char *title)	//描绘窗口
{
	static char closebtn[14][16] = {
		"OOOOOOOOOOOOOOO@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQQQ@@QQQQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"O$$$$$$$$$$$$$$@",
		"@@@@@@@@@@@@@@@@"
	};
	int x, y;
	char c;
	boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         xsize - 1, 0        );
	boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         xsize - 2, 1        );
	boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         0,         ysize - 1);
	boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         1,         ysize - 2);
	boxfill8(buf, xsize, COL8_848484, xsize - 2, 1,         xsize - 2, ysize - 2);
	boxfill8(buf, xsize, COL8_000000, xsize - 1, 0,         xsize - 1, ysize - 1);
	boxfill8(buf, xsize, COL8_C6C6C6, 2,         2,         xsize - 3, ysize - 3);
	boxfill8(buf, xsize, COL8_000084, 3,         3,         xsize - 4, 20       );
	boxfill8(buf, xsize, COL8_848484, 1,         ysize - 2, xsize - 2, ysize - 2);
	boxfill8(buf, xsize, COL8_000000, 0,         ysize - 1, xsize - 1, ysize - 1);
	putfonts8_asc(buf, xsize, 24, 4, COL8_FFFFFF, title);
	for (y = 0; y < 14; y++) {
		for (x = 0; x < 16; x++) {
			c = closebtn[y][x];
			if (c == '@') {
				c = COL8_000000;
			} else if (c == '$') {
				c = COL8_848484;
			} else if (c == 'Q') {
				c = COL8_C6C6C6;
			} else {
				c = COL8_FFFFFF;
			}
			buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
		}
	}
	return;
}
