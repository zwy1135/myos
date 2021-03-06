; naskfunc
; TAB=4

[FORMAT "WCOFF"]				; 看不懂	
[INSTRSET "i486p"]				; 486以上cpu
[BITS 32]						; 32位模式
[FILE "naskfunc.nas"]			; 看不懂

		GLOBAL	_io_hlt, _io_cli, _io_sti, _io_stihlt
		GLOBAL	_io_in8,  _io_in16,  _io_in32
		GLOBAL	_io_out8, _io_out16, _io_out32
		GLOBAL	_io_load_eflags, _io_store_eflags
		GLOBAL	_load_gdtr, _load_idtr
		GLOBAL	_load_cr0, _store_cr0
		GLOBAL	_asm_inthandler20, _asm_inthandler21
		GLOBAL	_asm_inthandler27, _asm_inthandler2c
		GLOBAL	_memtest_sub
		;for multitask
		GLOBAL	_load_tr, _taskswitch4, _taskswitch3, _farjump

		;interrupt handler
		EXTERN	_inthandler20, _inthandler21
		EXTERN	_inthandler27, _inthandler2c

[SECTION .text]

_io_hlt:	; void io_hlt(void);
		HLT
		RET

_io_cli:	; void io_cli(void);
		CLI
		RET

_io_sti:	; void io_sti(void);
		STI
		RET

_io_stihlt:	; void io_stihlt(void);
		STI
		HLT
		RET

_io_in8:	; int io_in8(int port);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,0
		IN		AL,DX
		RET

_io_in16:	; int io_in16(int port);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,0
		IN		AX,DX
		RET

_io_in32:	; int io_in32(int port);
		MOV		EDX,[ESP+4]		; port
		IN		EAX,DX
		RET

_io_out8:	; void io_out8(int port, int data);
		MOV		EDX,[ESP+4]		; port
		MOV		AL,[ESP+8]		; data
		OUT		DX,AL
		RET

_io_out16:	; void io_out16(int port, int data);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,[ESP+8]		; data
		OUT		DX,AX
		RET

_io_out32:	; void io_out32(int port, int data);
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,[ESP+8]		; data
		OUT		DX,EAX
		RET

_io_load_eflags:	; int io_load_eflags(void);
		PUSHFD		; PUSH EFLAGS 偲偄偆堄枴
		POP		EAX
		RET

_io_store_eflags:	; void io_store_eflags(int eflags);
		MOV		EAX,[ESP+4]
		PUSH	EAX
		POPFD		; POP EFLAGS 偲偄偆堄枴
		RET

_load_gdtr:		; void load_gdtr(int limit, int addr);
		MOV		AX,[ESP+4]		; limit
		MOV		[ESP+6],AX
		LGDT	[ESP+6]
		RET

_load_idtr:		; void load_idtr(int limit, int addr);
		MOV		AX,[ESP+4]		; limit
		MOV		[ESP+6],AX
		LIDT	[ESP+6]
		RET

_load_cr0:		;int load_cr0();
		MOV		EAX,CR0
		RET
		

_store_cr0:		;void store_cr0(int cr0);
		MOV		EAX,[ESP+4]
		MOV		CR0,EAX
		RET

_asm_inthandler20:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler20
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_asm_inthandler21:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler21
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_asm_inthandler27:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler27
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_asm_inthandler2c:
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler2c
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD
		
		

_memtest_sub:	;unsigned int memtest_sub(unsigned int start,unsigned int end)
		PUSH	EDI			;还要使用这三个寄存器

		PUSH	ESI
		PUSH	EBX
		MOV		ESI,0xaa55aa55	;part0 = 0xaa55aa55
		MOV		EDI,0X55AA55AA	;part1 = 0x55aa55aa
		MOV		EAX,[ESP+12+4]
mts_loop:
		MOV		EBX,EAX
		ADD		EBX,0XFFC
		MOV		EDX,[EBX]
		MOV		[EBX],ESI
		XOR		DWORD [EBX],0XFFFFFFFF
		CMP		EDI,[EBX]
		JNE		mts_fin
		XOR		DWORD [EBX],0XFFFFFFFF
		CMP		ESI,[EBX]
		JNE		mts_fin
		MOV		[EBX],EDX
		ADD		EAX,0X1000
		CMP		EAX,[ESP+12+8]
		JBE		mts_loop
		POP		EBX
		POP		ESI
		POP		EDI
		RET
mts_fin:
		MOV		[EBX],EDX
		POP		EBX
		POP		ESI
		POP		EDI
		RET

_load_tr:		;void load_tr(int tr);
		LTR		[ESP+4]		;tr
		RET

_taskswitch4:	;void taskswitch4(void) 跳到task4
		JMP		4*8:0
		RET

_taskswitch3: 	;void taskswitch3(void)
		JMP		3*8:0
		RET

_farjump:		;void farjump(int eip, int cs);
		JMP 	FAR [ESP+4]
		RET
		
