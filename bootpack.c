//主程序
#include "bootpack.h"
#include <stdio.h>

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

#define MEMMAN_FREES		4090    //大约32kb
#define MEMMAN_ADDR			0x003c0000

struct FREEINFO						//可用信息
{
	unsigned int addr,size;
};

struct MEMMAN						//内存管理
{
	int frees,maxfrees,lostsize,losts;
	struct FREEINFO free[MEMMAN_FREES];
};

unsigned int memtest(unsigned int start,unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man,unsigned int size);
int memman_free(struct MEMMAN *man,unsigned int addr,unsigned int size);

void HariMain()
{
	struct BOOTINFO *binfo=(struct BOOTINFO *) ADR_BOOTINFO;
	struct MOUSE_DEC mdec;
	char s[40],mcursor[256],keybuf[32],mousebuf[128];
	int mx,my,i;
	unsigned int memtotal;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	
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
	
	//以下是显示的内容
	init_screen8(binfo->vram,binfo->scrnx,binfo->scrny);//画屏幕背景
	mx=(binfo->scrnx- 16)/2;		//鼠标定位
	my=(binfo->scrny- 28 - 16)/2;
	init_mouse_cursor8(mcursor,COL8_008484);
	putblock8_8(binfo->vram,binfo->scrnx,16,16,mx,my,mcursor,16);
	sprintf(s,"(%d,%d)",mx,my);	//写入内存
	putfonts8_asc(binfo->vram,binfo->scrnx,0,0,COL8_FFFFFF,s);//输出mx，my
	
	//内存信息
	sprintf(s,"memory %d MB , free %d KB , lost %d KB",memtotal / (1024*1024),memman_total(memman) / 1024,memman->lostsize / 1024);
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

void memman_init(struct MEMMAN *man)
{
	man->frees = 0;						//可用信息条数
	man->maxfrees = 0;					//用于观察可用状况：frees的最大值
	man->lostsize = 0;					//释放失败的总空间大小
	man->losts = 0;						//释放失败的次数
	return;
}

unsigned int memman_total(struct MEMMAN *man)		//报告可用内存总空间
{
	unsigned int i,t =0;
	for(i = 0;i < man->frees;i++)
		t += man->free[i].size;
	return t;
}

unsigned int memman_alloc(struct MEMMAN *man,unsigned int size)		//分配内存
{
	unsigned int i,a;
	for(i = 0;i < man->frees;i++)
	{
		if(man->free[i].size >= size)				//找到了足够大的内存空间
		{
			a = man->free[i].addr;					//找出地址
			man->free[i].addr += size;				//将空间从可用空间里除去
			man->free[i].size -= size;				//同上
			if(man->free[i].size == 0)				//减掉不必要的可用信息
			{
				man->frees --;
				for(;i < man->frees;i++)
				{
					man->free[i] = man->free[i + 1];
				}
			}
			return a;
		}
	}
	return 0;										//没找到足够大的可用空间
}

int memman_free(struct MEMMAN *man,unsigned int addr,unsigned int size)
{
	int i,j;
	for(i = 0;i < man->frees;i++)					//找到被释放的地址在表中的位置
	{
		if(man->free[i].addr > addr)
			break;
	}
	if(i > 0)										//如果之前有剩余空间
	{
		if(man->free[i - 1].addr + man->free[i - 1].size == addr)	//可以与之前的空间合并
		{
			man->free[i - 1].size += size;
			if(i < man->frees)						//之后有剩余空间
			{
				if(addr + size == man->free[i].addr)	//可以与之后的剩余空间合并
				{
					man->free[i - 1].size += man->free[i].size;
					man->frees --;
					for(;i < man->frees;i++)			//合并后消除free[i]
					{
						man->free[i] = man->free[i + 1];
					}
				}
			}
			return 0;							//成功完成
		}
	}
	//至此：之前没有
	if(i < man->frees)						//如果之后有剩余空间
	{
		if(addr + size == man->free[i].addr)	//可合并
		{
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 1;
		}
	}
	//至此：之前之后都没空间
	if(man->frees < MEMMAN_FREES)			//还可向表中加入
	{
		for(j = man->frees;j > i;j--)		//腾出空间
		{
			man->free[j] = man->free[j - 1];
		}
		//以下为加入新项
		man->frees ++;						
		if(man->maxfrees < man->frees)
		{
			man->maxfrees = man->frees;
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 2;
	}
	//至此:无法释放空间
	man->losts ++;
	man->lostsize += size;
	return -1;			//释放失败
}

