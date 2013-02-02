//有关键盘的部分
#include "bootpack.h"


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

struct FIFO8 keyfifo;

void inthandler21(int *esp)					//处理PS/2键盘的中断
{
	unsigned char data;
	io_out8(PIC0_OCW2, 0x61);				//恢复1号中断响应，PIC0的1号
	data = io_in8(PORT_KEYDAT);
	fifo8_put(&keyfifo, data);
	return;
}