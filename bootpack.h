//asmhead.nas
struct BOOTINFO	
{
	char cyls;			//启动区读盘截至点
	char leds;			//启动时键盘LED状态
	char vmode;			//显卡模式
	char reserve;
	short scrnx,scrny;	//分辨率
	char *vram;
};
#define ADR_BOOTINFO 0x00000ff0
//naskfunc.nas
void io_hlt();
void io_cli();
void io_out8(int port,int data);
int io_load_eflags();
void io_store_eflags(int eflags);
void load_gdtr(int limit,int addr);
void load_idtr(int limit,int addr);
//graphic.c
void init_palette();
void set_palette(int start,int end,unsigned char *rgb);
void boxfill8(unsigned char *vram,int xsize,unsigned char c,int x0,int y0,int x1,int y1);
