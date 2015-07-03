#define NULL 	(void*)0

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
void io_sti();
void io_stihlt();
void io_out8(int port,int data);
int io_in8(int port);
int io_load_eflags();
void io_store_eflags(int eflags);
void load_gdtr(int limit,int addr);
void load_idtr(int limit,int addr);

int load_cr0();
void store_cr0(int cr0);
unsigned int memtest_sub(unsigned int start,unsigned int end);

void asm_inthandler20();
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);

//fifo.c
struct FIFO8
{
	unsigned char *buf;
	int p,q,size,free,flags;		//写入地址，读出地址，空字节数，可用性
};
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf);
int fifo8_put(struct FIFO8 *fifo, unsigned char data);
int fifo8_get(struct FIFO8 *fifo);
int fifo8_status(struct FIFO8 *fifo);

struct FIFO32
{
	int *buf;
	int p,q,size,free,flags;
};
void fifo32_init(struct FIFO32 *fifo, int size, int *buf);
int fifo32_put(struct FIFO32 *fifo, int data);
int fifo32_get(struct FIFO32 *fifo);
int fifo32_status(struct FIFO32 *fifo);

//graphic.c
void init_palette();
void set_palette(int start,int end,unsigned char *rgb);
void boxfill8(unsigned char *vram,int xsize,unsigned char c,int x0,int y0,int x1,int y1);
void init_screen8(char *vram,int x,int y);
void putfont8(char *vram,int xsize,int x,int y,char c,char*font);
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);

void init_mouse_cursor8(char *mouse,char bc);
void putblock8_8(char *vram,int vxsize,int pxsize,int pysize,int px0,int py0,char *buf,int bxsize);
#define COL8_000000		0			//颜色
#define COL8_FF0000		1
#define COL8_00FF00		2
#define COL8_FFFF00		3
#define COL8_0000FF		4
#define COL8_FF00FF		5
#define COL8_00FFFF		6
#define COL8_FFFFFF		7
#define COL8_C6C6C6		8
#define COL8_840000		9
#define COL8_008400		10
#define COL8_848400		11
#define COL8_000084		12
#define COL8_840084		13
#define COL8_008484		14
#define COL8_848484		15


//dsctbl.c
struct SEGMENT_DESCRIPTOR
{
	short limit_low,base_low;
	char base_mid,access_right;
	char limit_high,base_high;
};
struct GATE_DESCRIPTOR
{
	short offset_low,selector;
	char dw_count,access_right;
	short offset_high;
};
void init_gdtidt();
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd,unsigned int limit,int base,int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd,int offset,int selector,int ar);
#define ADR_IDT			0x0026f800
#define LIMIT_IDT		0x000007ff
#define ADR_GDT			0x00270000
#define LIMIT_GDT		0x0000ffff
#define ADR_BOTPAK		0x00280000
#define LIMIT_BOTPAK	0x0007ffff
#define AR_DATA32_RW	0x4092
#define AR_CODE32_ER	0x409a
#define AR_INTGATE32	0x008e
#define AR_TSS32		0x0089

//int.c
void init_pic();
void inthandler27(int *esp);
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1


//keyboard.c
void init_keyboard(struct FIFO32 *fifo, int data0);
void wait_KBC_sendready();
void inthandler21(int *esp);
#define PORT_KEYDAT				0x0060
#define PORT_KEYSTA				0x0064
#define PORT_KEYCMD				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47
#define PORT_KEYDAT				0x0060

//mouse.c
struct MOUSE_DEC
{
	unsigned char buf[3],phase;
	int x,y,btn;						//存放鼠标移动和点击状态的变量
};

void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec,unsigned char dat);
void inthandler2c(int *esp);
#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4

//memory.c
#define MEMMAN_FREES		4090    //大约32kb
#define MEMMAN_ADDR			0x003c0000
#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

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
unsigned int memman_alloc_4k(struct MEMMAN *man,unsigned int size);
int memman_free_4k(struct MEMMAN *man,unsigned int addr,unsigned int size);

//sheet.c
struct SHEET						//图层信息
{
	unsigned char *buf;
	int bxsize,bysize/*图层的x，y大小*/,vx0,vy0/*图层的位置*/,col_inv/*透明色*/,height,flags;
	struct SHTCTL *ctl;
};

#define MAX_SHEETS			256		//最大图层数

struct SHTCTL						//图层组管理信息
{
	unsigned char *vram,*map;
	int xsize,ysize,top;
	struct SHEET *sheets[MAX_SHEETS];
	struct SHEET sheet0[MAX_SHEETS];
};

struct SHTCTL *shtctl_init(struct MEMMAN *memman,unsigned char *vram,int xsize,int ysize);
struct SHEET *sheet_alloc(struct SHTCTL *ctl);
void sheet_setbuf(struct SHEET *sht,unsigned char *buf,int xsize,int ysize,int col_inv);
void sheet_updown(struct SHEET *sht,int height);
void sheet_refresh(struct SHEET *sht,int bx0,int by0,int bx1,int by1);
void sheet_slide(struct SHEET *sht,int vx0,int vy0);
void sheet_free(struct SHEET *sht);
void sheet_refreshsub(struct SHTCTL *ctl,int vx0,int vy0,int vx1,int vy1,int h0,int h1);
void sheet_refreshmap(struct SHTCTL *ctl,int vx0,int vy0,int vx1,int vy1,int h0);



//timer.c
#define MAX_TIMER	500

struct TIMER
{
	struct TIMER *next;
	unsigned int timeout;
	unsigned int flags;
	struct FIFO32 *fifo;
	int data;
};

struct TIMERCTL
{
	struct TIMER timers0[MAX_TIMER];
	struct TIMER *t0;

	unsigned int count;
	unsigned int next;
	unsigned int using;
};

void init_pit();
void inthandler20(int *esp);
//void settimer(unsigned int timeout,struct FIFO8 *fifo,unsigned char data);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer,struct FIFO32 *fifo,int data);
void timer_settime(struct TIMER *timer,unsigned int timeout);


//mtask.c

#define MAX_TASKS 	1000
#define TASK_GDT0 	3

struct TSS32
{
	int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
	int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	int es, cs, ss, ds, fs, gs;
	int ldtr, iomap;
};

struct TASK
{
	int sel, flags;
	struct TSS32 tss;
	struct TASK* next;
};

struct TASKCTL
{

	struct TASK *tasks[MAX_TASKS];
	struct TASK tasks0[MAX_TASKS];
	struct TASK *head, *nil, *current;
};

//void mt_init();
//void mt_taskswitch();
void task_switch(void);
void task_run(struct TASK* task);
struct TASK* task_alloc(void);
struct TASK* task_init(struct MEMMAN *memman);
