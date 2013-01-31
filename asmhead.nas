; haribote-os boot asm
; TAB=4
;UTF-8
BOTPAK	EQU		0x00280000		; bootpack�̃��[�h��
DSKCAC	EQU		0x00100000		; �f�B�X�N�L���b�V���̏ꏊ
DSKCAC0	EQU		0x00008000		; �f�B�X�N�L���b�V���̏ꏊ�i���A�����[�h�j

; BOOT_INFO�֌W
CYLS	EQU		0x0ff0			; �u�[�g�Z�N�^���ݒ肷��
LEDS	EQU		0x0ff1
VMODE	EQU		0x0ff2			; �F���Ɋւ�����B���r�b�g�J���[���H
SCRNX	EQU		0x0ff4			; �𑜓x��X
SCRNY	EQU		0x0ff6			; �𑜓x��Y
VRAM	EQU		0x0ff8			; �O���t�B�b�N�o�b�t�@�̊J�n�Ԓn

		ORG		0xc200			; ���̃v���O�������ǂ��ɓǂݍ��܂��̂�

; ��ʃ��[�h��ݒ�

		MOV		AL,0x13			; VGA�O���t�B�b�N�X�A320x200x8bit�J���[
		MOV		AH,0x00
		INT		0x10
		MOV		BYTE [VMODE],8	; ��ʃ��[�h����������iC���ꂪ�Q�Ƃ���j
		MOV		WORD [SCRNX],320
		MOV		WORD [SCRNY],200
		MOV		DWORD [VRAM],0x000a0000

; �L�[�{�[�h��LED��Ԃ�BIOS�ɋ����Ă��炤

		MOV		AH,0x02
		INT		0x16 			; keyboard BIOS
		MOV		[LEDS],AL

; PIC关闭一切中断?
;	根据AT兼容机的规格，如果要初始化PIC，则/n
;	必须在CLI执行前进行，否则有时会挂起
;	随后进行PIC初始化

		MOV		AL,0xff
		OUT		0x21,AL			;将AL写入0x21端口的设备，相当于io_out8(PIC0_IMR,0xff)
		NOP						; 休息命令，放在两个OUT之间防止出错
		OUT		0xa1,AL			;写入PIC1_IMR

		CLI						; 禁止CPU级别中断

; 为了让CPU访问1MB以上的空间，设定A20GATE信号线为可用

		CALL	waitkbdout		;调用函数等待设置完成
		MOV		AL,0xd1
		OUT		0x64,AL			;相当于io_out8(PORT_KEYCMD,KEY_WRITE_OUTPUT)
		CALL	waitkbdout
		MOV		AL,0xdf			; enable A20
		OUT		0x60,AL			;相当于io_out8(PORT_KEY_OUTPUT_A20GATE_ENABLE)
		CALL	waitkbdout

; 切换到保护模式（本文中指32位模式）

[INSTRSET "i486p"]				; 调入486指令（LGDT、EAX等命令）

		LGDT	[GDTR0]			; 设置临时GDT，随机设置，以后自己重设
		MOV		EAX,CR0
		AND		EAX,0x7fffffff	; bit31（CR0的最高位）设置为0（为了禁止颁？？？？？）
		OR		EAX,0x00000001	; bit0（最低位）设置为1
		MOV		CR0,EAX
		JMP		pipelineflush	;切模式后必须JMP
pipelineflush:					;把除CS外的段寄存器都设为0x0008（相当于gdt+1），CS以后设
		MOV		AX,1*8			;  可读写的段32bit
		MOV		DS,AX
		MOV		ES,AX
		MOV		FS,AX
		MOV		GS,AX
		MOV		SS,AX

; bootpack的传送

		MOV		ESI,bootpack	; 传送源
		MOV		EDI,BOTPAK		; 目的地
		MOV		ECX,512*1024/4	;传送大小
		CALL	memcpy			;调用传送函数

; 磁盘数据传送到本来的位置去

; 从启动扇区开始

		MOV		ESI,0x7c00		; 同上
		MOV		EDI,DSKCAC		
		MOV		ECX,512/4
		CALL	memcpy

; 所有剩下的

		MOV		ESI,DSKCAC0+512	; 同上
		MOV		EDI,DSKCAC+512	
		MOV		ECX,0
		MOV		CL,BYTE [CYLS]
		IMUL	ECX,512*18*2/4	; 从柱面变换字节数/4
		SUB		ECX,512/4		; 减去IPL
		CALL	memcpy

; asmhead的工作完成了
;	以后交给bootpack完成

; bootpack启动

		MOV		EBX,BOTPAK
		MOV		ECX,[EBX+16]
		ADD		ECX,3			; ECX += 3;
		SHR		ECX,2			; ECX /= 4，实际上是右移2位
		JZ		skip			; 没有要传送的东西时
		MOV		ESI,[EBX+20]	; 传送源
		ADD		ESI,EBX
		MOV		EDI,[EBX+12]	; 目的地
		CALL	memcpy
skip:
		MOV		ESP,[EBX+12]	; 栈的初始值
		JMP		DWORD 2*8:0x0000001b	;将2*8代入CS并移动到段号为2的0x1b地址

waitkbdout:
		IN		 AL,0x64
		AND		 AL,0x02
		IN		 AL,0X60		;清空垃圾数据
		JNZ		waitkbdout		; and的结果不等于零则跳转
		RET

memcpy:
		MOV		EAX,[ESI]
		ADD		ESI,4
		MOV		[EDI],EAX
		ADD		EDI,4
		SUB		ECX,1
		JNZ		memcpy			; 同上
		RET
; ？？

		ALIGNB	16				;执行DBO到地址能被16整除为止
GDT0:
		RESB	8				; 空出第一位
		DW		0xffff,0x0000,0x9200,0x00cf	; 可读写的段32bit
		DW		0xffff,0x0000,0x9a28,0x0047	; 可执行的段32bit（bootpack用）

		DW		0
GDTR0:
		DW		8*3-1
		DD		GDT0

		ALIGNB	16
bootpack:
