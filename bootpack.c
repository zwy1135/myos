//主程序
#include "bootpack.h"
#include <stdio.h>

void make_window8(unsigned char *buf, int xsize, int ysize, char *title);
void make_textbox8(struct SHEET * sht, int x0, int y0, int xsize, int ysize, int color);
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int color, int background, char *s,int length);
void task_b_main();


extern struct TIMERCTL timerctl;

static char keytable[0x54] = {
	0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '+', 0,   0,
	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0,   0,   'A', 'S',
	'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', 0,   0,   '\\', 'Z', 'X', 'C', 'V',
	'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
	0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
	'2', '3', '0', '.'
};

void HariMain()
{
	struct BOOTINFO *binfo=(struct BOOTINFO *) ADR_BOOTINFO;
	struct MOUSE_DEC mdec;
	char s[40];
	int mx,my,i;
	unsigned int memtotal,count = 0;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SHTCTL *shtctl;
	struct SHEET *sht_back,*sht_mouse,*sht_win,*sht_win_counter;
	unsigned char *buf_back,buf_mouse[256],*buf_win,*buf_win_counter;
	//timer initial
	struct TIMER *timer, *timer2, *timer3;
	//all fifo
	int fifobuf[256];
	struct FIFO32 fifo;
	//for textbox
	int cursor_x = 8,cursor_color = COL8_FFFFFF;
	//for multitask
	int task_b_esp;
	struct TSS32 tss_a, tss_b;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)ADR_GDT;
	struct TIMER *timer_ts;

	




	//初始化部分
	init_gdtidt();
	init_pic();
	
	io_sti();			//interrupt flag 变为1，开始接受中断
	
	fifo32_init(&fifo, 256, fifobuf);		//初始化键盘FIFO

	
	init_pit();
	io_out8(PIC0_IMR, 0xf8); /* 0 1 2 号端口打开，PIT PIC1和键盘启用(11111000) */
	io_out8(PIC1_IMR, 0xef); /* IRQ12打开，鼠标启用(11101111) */
	
	init_keyboard(&fifo, 256);
	enable_mouse(&fifo, 512, &mdec);
	
	memtotal = memtest(0x00400000,0xbfffffff);
	memman_init(memman);
	memman_free(memman,0x00001000,0x0009e000);			//这块地址是什么情况？？？
	memman_free(memman,0x00400000, memtotal - 0x00400000);
	
	init_palette();
	
	shtctl = shtctl_init(memman,binfo->vram,binfo->scrnx,binfo->scrny);
	sht_back = sheet_alloc(shtctl);
	sht_mouse = sheet_alloc(shtctl);
	sht_win = sheet_alloc(shtctl);
	//sht_win_counter = sheet_alloc(shtctl);
	buf_back = (unsigned char *)memman_alloc_4k(memman,binfo->scrnx * binfo->scrny);
	buf_win = (unsigned char *)memman_alloc_4k(memman,160 * 68);
	//buf_win_counter = (unsigned char *)memman_alloc_4k(memman,160 * 52);
	sheet_setbuf(sht_back,buf_back,binfo->scrnx,binfo->scrny,-1);	//背景无透明色
	sheet_setbuf(sht_mouse,buf_mouse,16,16,99);		//透明色为99
	sheet_setbuf(sht_win,buf_win,160,68,-1);		//无透明色
	//sheet_setbuf(sht_win_counter,buf_win_counter,160,52,-1);

	//超时定时器初始化
	fifo32_init(&fifo, 256, fifobuf);
	timer = timer_alloc();
	timer_init(timer, &fifo, 10);
	timer_settime(timer, 1001);
	timer2 = timer_alloc();
	timer_init(timer2, &fifo, 3);
	timer_settime(timer2, 301);
	timer3 = timer_alloc();
	timer_init(timer3, &fifo, 1);
	timer_settime(timer3, 51);
	//for multitask timer
	timer_ts = timer_alloc();
	timer_init(timer_ts, &fifo, 2);
	timer_settime(timer_ts,2);

	







	//以下是显示的内容
	init_screen8(buf_back,binfo->scrnx,binfo->scrny);//画屏幕背景
	init_mouse_cursor8(buf_mouse,99);//透明色99
	make_window8(buf_win,160,68,"text window");
	//putfonts8_asc(buf_win,160,24,28,COL8_000000,"That's os of");
	//putfonts8_asc(buf_win,160,50,44,COL8_000000,"z wy");
	//make_window8(buf_win_counter,160,52,"timer");
	make_textbox8(sht_win,8, 28, 144, 16, COL8_FFFFFF);
	
	sheet_slide(sht_back,0,0);
	mx=(binfo->scrnx- 16)/2;		//鼠标定位
	my=(binfo->scrny- 28 - 16)/2;
	
	sheet_slide(sht_mouse,mx,my);
	//最高只能写到top+1，所以初始化顺序很重要
	sheet_updown(sht_back,0);
	//sheet_updown(sht_win_counter,2);
	sheet_updown(sht_win,1);
	sheet_updown(sht_mouse,3);
	sheet_slide(sht_win,130,72);
	//sheet_slide(sht_win_counter,100,72);
	
	sprintf(s,"(%3d,%3d)",mx,my);	//写入内存
	putfonts8_asc(buf_back,binfo->scrnx,0,0,COL8_FFFFFF,s);//输出mx，my
	
	//内存信息
	sprintf(s,"memory %d MB , free %d KB , lost %d KB",memtotal / (1024*1024),memman_total(memman) / 1024,memman->lostsize / 1024);
	putfonts8_asc(buf_back,binfo->scrnx,0,32,COL8_FFFFFF,s);
	sheet_refresh(sht_back,0,0,binfo->scrnx,96);		//因为上面sheet_slide刷新过一次，所以只刷新到48



	//多任务的内容
	tss_a.ldtr = 0;
	tss_a.iomap = 0x40000000;
	tss_b.ldtr = 0;
	tss_b.iomap = 0x40000000;

	set_segmdesc(gdt + 3, 103, (int)&tss_a, AR_TSS32);	//AR_TSS32 = 0x0089,书上没讲
	set_segmdesc(gdt + 4, 103, (int)&tss_b, AR_TSS32);
	task_b_esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;

	load_tr(3 * 8);

	tss_b.eip = (int)&task_b_main;
	tss_b.eflags = 0x00000202; //IF = 1
	tss_b.eax = 0;
	tss_b.ecx = 0;
	tss_b.edx = 0;
	tss_b.ebx = 0;
	tss_b.esp = task_b_esp;	//定义了一个int变量，专门为任务b定义的栈
	tss_b.ebp = 0;
	tss_b.esi = 0;
	tss_b.edi = 0;
	tss_b.es = 1 * 8;
	tss_b.cs = 2 * 8;
	tss_b.ss = 1 * 8;
	tss_b.ds = 1 * 8;
	tss_b.fs = 1 * 8;
	tss_b.gs = 1 * 8;

	
	
	




	//中断FIFO处理
	for(;;)
	{
		//sprintf(s,"%010d",timerctl.count);
		//boxfill8(buf_win_counter,160,COL8_C6C6C6,40,28,119,43);
		//putfonts8_asc(buf_win_counter,160,40,28,COL8_000000,s);
		//sheet_refresh(sht_win_counter,40,28,120,44);
		//count ++;
		
		io_cli();
		if(fifo32_status(&fifo) == 0)
			io_stihlt();
		else
		{
			i = fifo32_get(&fifo);
			io_sti();
			if(256 <= i && i <= 511)
			{
				sprintf(s,"%02X",i - 256);
				putfonts8_asc_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, s, 2);
				if(i < 256 + 0x54 && cursor_x < 144 && keytable[i - 256] != '\0')
				{
					s[0] = keytable[i - 256];
					s[1] = '\0';
					putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
					cursor_x += 8;
				}
				if(i == 256 + 0x0e && cursor_x > 8)
				{
					putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
					cursor_x -= 8;
				}
				boxfill8(sht_win->buf, sht_win->bxsize, cursor_color, cursor_x,28,cursor_x + 7, 43);
				sheet_refresh(sht_win,cursor_x,28,cursor_x + 8,44);	
			}
			else if(512 <= i && i <= 767)
			{
				if((mouse_decode(&mdec,i - 512)) != 0)
				{
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
					putfonts8_asc_sht(sht_back,0,0,COL8_FFFFFF,COL8_008484,s,10);
					sheet_slide(sht_mouse,mx,my);

					sprintf(s,"[lcr %4d %4d]",mdec.x,mdec.y);
					if((mdec.btn & 0x01) != 0)							//第一位左键
					{
						s[1] = 'L';
						sheet_slide(sht_win, mx - 80, my + 8);
					}
					if((mdec.btn & 0x02) != 0)							//第二位右键
					{
						s[3] = 'R';
					}
					if((mdec.btn & 0x04) != 0)							//第三位中键
					{
						s[2] = 'C';
					}
					putfonts8_asc_sht(sht_back,48,16,COL8_FFFFFF,COL8_008484,s,15);
					
				}
				
			}
			else if(i == 10)
			{
				putfonts8_asc_sht(sht_back,0,64,COL8_FFFFFF,COL8_008484,"10 seconds",10);
				//sprintf(s,"%010d",count);
				//putfonts8_asc_sht(sht_win_counter,40,28,COL8_000000,COL8_C6C6C6,s,10);
			}
			else if(i == 3)
			{
				putfonts8_asc_sht(sht_back,0,80,COL8_FFFFFF,COL8_008484,"3 seconds",9);
				//count = 0;
			}
			else if(i == 2)
			{
				farjump(0,4*8);
				timer_settime(timer_ts, 2);
			}
			else
			{
				if(i == 1)
				{
					timer_init(timer3,&fifo,0);
					//boxfill8(sht_back->buf,sht_back->bxsize,COL8_FFFFFF,0,96,7,111);
					cursor_color = COL8_FFFFFF;
				}
				else if(i == 0)
				{
					timer_init(timer3,&fifo,1);
					//boxfill8(sht_back->buf,sht_back->bxsize,COL8_008484,0,96,7,111);
					cursor_color = COL8_000000;
				}
				timer_settime(timer3,51);
				boxfill8(sht_win->buf, sht_win->bxsize, cursor_color, cursor_x,28,cursor_x + 7, 43);
				sheet_refresh(sht_win,cursor_x,28,cursor_x + 8,44);	
			}
		}
	}
}


void task_b_main()
{
	struct FIFO32 fifo;
	struct TIMER *timer;
	int i, fifobuf[128];

	fifo32_init(&fifo, 128, fifobuf);
	timer = timer_alloc();
	timer_init(timer, &fifo, 1);
	timer_settime(timer, 2);
	for(;;)
	{
		io_cli();
		if(fifo32_status(&fifo) == 0)
		{
			io_stihlt();
		}
		else 
		{
			io_sti();
			i = fifo32_get(&fifo);
			if(i == 1)
			{
				farjump(0,3*8);
				timer_settime(timer, 2);
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

void make_textbox8(struct SHEET * sht, int x0, int y0, int xsize, int ysize, int color)
{
	int x1 = x0 + xsize;
	int y1 = y0 + ysize;
	boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
	boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
	boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
	boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
	boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 2, y0 - 2, x0 - 2, y1 + 0);
	boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, color      , x0 + 0, y0 + 0, x1 + 0, y1 + 0);
	return;
}

void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int color, int background, char *s,int length)
{
	boxfill8(sht->buf, sht->bxsize, background, x, y, x + length*8 - 1, y + 15);
	putfonts8_asc(sht->buf, sht->bxsize, x, y, color, s);
	sheet_refresh(sht, x, y, x + length*8, y + 16);
	return;
}



