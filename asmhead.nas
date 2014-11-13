; haribote-os boot asm
; TAB=4
;UTF-8
BOTPAK	EQU		0x00280000		; bootpackのロード先
DSKCAC	EQU		0x00100000		; ディスクキャッシュの場所
DSKCAC0	EQU		0x00008000		; ディスクキャッシュの場所（リアルモード）

; BOOT_INFO関係
CYLS	EQU		0x0ff0			; ブートセクタが設定する
LEDS	EQU		0x0ff1
VMODE	EQU		0x0ff2			; 色数に関する情報。何ビットカラーか？
SCRNX	EQU		0x0ff4			; 解像度のX
SCRNY	EQU		0x0ff6			; 解像度のY
VRAM	EQU		0x0ff8			; グラフィックバッファの開始番地

		ORG		0xc200			; このプログラムがどこに読み込まれるのか

; 画面モードを設定

		MOV		AL,0x13			; VGAグラフィックス、320x200x8bitカラー
		MOV		AH,0x00
		INT		0x10
		MOV		BYTE [VMODE],8	; 画面モードをメモする（C言語が参照する）
		MOV		WORD [SCRNX],320
		MOV		WORD [SCRNY],200
		MOV		DWORD [VRAM],0x000a0000

; キーボードのLED状態をBIOSに教えてもらう

		MOV		AH,0x02
		INT		0x16 			; keyboard BIOS
		MOV		[LEDS],AL

; PIC蜈ｳ髣ｭ荳�蛻�荳ｭ譁ｭ?
;	譬ｹ謐ｮAT蜈ｼ螳ｹ譛ｺ逧�隗�譬ｼ�ｼ悟ｦよ棡隕∝�晏ｧ句喧PIC�ｼ悟��/n
;	蠢�鬘ｻ蝨ｨCLI謇ｧ陦悟燕霑幄｡鯉ｼ悟凄蛻呎怏譌ｶ莨壽撃襍ｷ
;	髫丞錘霑幄｡訓IC蛻晏ｧ句喧

		MOV		AL,0xff
		OUT		0x21,AL			;蟆�AL蜀吝�･0x21遶ｯ蜿｣逧�隶ｾ螟��ｼ檎嶌蠖謎ｺ司o_out8(PIC0_IMR,0xff)
		NOP						; 莨第�ｯ蜻ｽ莉､�ｼ梧叛蝨ｨ荳､荳ｪOUT荵矩龍髦ｲ豁｢蜃ｺ髞�
		OUT		0xa1,AL			;蜀吝�･PIC1_IMR

		CLI						; 遖∵ｭ｢CPU郤ｧ蛻ｫ荳ｭ譁ｭ

; 荳ｺ莠�隶ｩCPU隶ｿ髣ｮ1MB莉･荳顔噪遨ｺ髣ｴ�ｼ瑚ｮｾ螳哂20GATE菫｡蜿ｷ郤ｿ荳ｺ蜿ｯ逕ｨ

		CALL	waitkbdout		;隹�逕ｨ蜃ｽ謨ｰ遲牙ｾ�隶ｾ鄂ｮ螳梧��
		MOV		AL,0xd1
		OUT		0x64,AL			;逶ｸ蠖謎ｺ司o_out8(PORT_KEYCMD,KEY_WRITE_OUTPUT)
		CALL	waitkbdout
		MOV		AL,0xdf			; enable A20
		OUT		0x60,AL			;逶ｸ蠖謎ｺ司o_out8(PORT_KEY_OUTPUT_A20GATE_ENABLE)
		CALL	waitkbdout

; 蛻�謐｢蛻ｰ菫晄侃讓｡蠑擾ｼ域悽譁�荳ｭ謖�32菴肴ｨ｡蠑擾ｼ�

[INSTRSET "i486p"]				; 隹�蜈･486謖�莉､�ｼ�LGDT縲・AX遲牙多莉､�ｼ�

		LGDT	[GDTR0]			; 隶ｾ鄂ｮ荳ｴ譌ｶGDT�ｼ碁囂譛ｺ隶ｾ鄂ｮ�ｼ御ｻ･蜷手�ｪ蟾ｱ驥崎ｮｾ
		MOV		EAX,CR0
		AND		EAX,0x7fffffff	; bit31�ｼ�CR0逧�譛�鬮倅ｽ搾ｼ芽ｮｾ鄂ｮ荳ｺ0�ｼ井ｸｺ莠�遖∵ｭ｢鬚��ｼ滂ｼ滂ｼ滂ｼ滂ｼ滂ｼ�
		OR		EAX,0x00000001	; bit0�ｼ域怙菴惹ｽ搾ｼ芽ｮｾ鄂ｮ荳ｺ1
		MOV		CR0,EAX
		JMP		pipelineflush	;蛻�讓｡蠑丞錘蠢�鬘ｻJMP
pipelineflush:					;謚企勁CS螟也噪谿ｵ蟇�蟄伜勣驛ｽ隶ｾ荳ｺ0x0008�ｼ育嶌蠖謎ｺ使dt+1�ｼ会ｼ靴S莉･蜷手ｮｾ
		MOV		AX,1*8			;  蜿ｯ隸ｻ蜀咏噪谿ｵ32bit
		MOV		DS,AX
		MOV		ES,AX
		MOV		FS,AX
		MOV		GS,AX
		MOV		SS,AX

; bootpack逧�莨�騾�

		MOV		ESI,bootpack	; 莨�騾∵ｺ�
		MOV		EDI,BOTPAK		; 逶ｮ逧�蝨ｰ
		MOV		ECX,512*1024/4	;莨�騾∝､ｧ蟆�
		CALL	memcpy			;隹�逕ｨ莨�騾∝�ｽ謨ｰ

; 逎∫尨謨ｰ謐ｮ莨�騾∝芦譛ｬ譚･逧�菴咲ｽｮ蜴ｻ

; 莉主星蜉ｨ謇�蛹ｺ蠑�蟋�

		MOV		ESI,0x7c00		; 蜷御ｸ�
		MOV		EDI,DSKCAC		
		MOV		ECX,512/4
		CALL	memcpy

; 謇�譛牙黄荳狗噪

		MOV		ESI,DSKCAC0+512	; 蜷御ｸ�
		MOV		EDI,DSKCAC+512	
		MOV		ECX,0
		MOV		CL,BYTE [CYLS]
		IMUL	ECX,512*18*2/4	; 莉取浤髱｢蜿俶困蟄苓鰍謨ｰ/4
		SUB		ECX,512/4		; 蜃丞悉IPL
		CALL	memcpy

; asmhead逧�蟾･菴懷ｮ梧�蝉ｺ�
;	莉･蜷惹ｺ､扈冀ootpack螳梧��

; bootpack蜷ｯ蜉ｨ

		MOV		EBX,BOTPAK
		MOV		ECX,[EBX+16]
		ADD		ECX,3			; ECX += 3;
		SHR		ECX,2			; ECX /= 4�ｼ悟ｮ樣刔荳頑弍蜿ｳ遘ｻ2菴�
		JZ		skip			; 豐｡譛芽ｦ∽ｼ�騾∫噪荳懆･ｿ譌ｶ
		MOV		ESI,[EBX+20]	; 莨�騾∵ｺ�
		ADD		ESI,EBX
		MOV		EDI,[EBX+12]	; 逶ｮ逧�蝨ｰ
		CALL	memcpy
skip:
		MOV		ESP,[EBX+12]	; 譬育噪蛻晏ｧ句�ｼ
		JMP		DWORD 2*8:0x0000001b	;蟆�2*8莉｣蜈･CS蟷ｶ遘ｻ蜉ｨ蛻ｰ谿ｵ蜿ｷ荳ｺ2逧�0x1b蝨ｰ蝮�

waitkbdout:
		IN		 AL,0x64
		AND		 AL,0x02
		IN		 AL,0X60		;貂�遨ｺ蝙�蝨ｾ謨ｰ謐ｮ
		JNZ		waitkbdout		; and逧�扈捺棡荳咲ｭ我ｺ朱峺蛻呵ｷｳ霓ｬ
		RET

memcpy:
		MOV		EAX,[ESI]
		ADD		ESI,4
		MOV		[EDI],EAX
		ADD		EDI,4
		SUB		ECX,1
		JNZ		memcpy			; 蜷御ｸ�
		RET
; �ｼ滂ｼ�

		ALIGNB	16				;謇ｧ陦轡BO蛻ｰ蝨ｰ蝮�閭ｽ陲ｫ16謨ｴ髯､荳ｺ豁｢
GDT0:
		RESB	8				; 遨ｺ蜃ｺ隨ｬ荳�菴�
		DW		0xffff,0x0000,0x9200,0x00cf	; 蜿ｯ隸ｻ蜀咏噪谿ｵ32bit
		DW		0xffff,0x0000,0x9a28,0x0047	; 蜿ｯ謇ｧ陦檎噪谿ｵ32bit�ｼ�bootpack逕ｨ�ｼ�

		DW		0
GDTR0:
		DW		8*3-1
		DD		GDT0

		ALIGNB	16
bootpack:
