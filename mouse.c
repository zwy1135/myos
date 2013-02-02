//有关鼠标的部分
#include "bootpack.h"

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

struct FIFO8 mousefifo;

void inthandler2c(int *esp)					//处理PS/2鼠标的中断
{
	unsigned char data;
	io_out8(PIC1_OCW2,0x64);				//恢复12号中断响应，PIC1的4号
	io_out8(PIC0_OCW2,0x62);				//恢复2号中断响应，PIC0的2号链接PIC1
	data = io_in8(PORT_KEYDAT);
	fifo8_put(&mousefifo,data);
	return;
}

