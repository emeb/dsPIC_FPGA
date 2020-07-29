; fexpo.s - 1V/Oct expo calculation
; prototype for c is extern unsigned long getfont(int val);
; 2-27-10 E. Brombaugh

.global _fexpo
.section .power_table, code
.include "powtab.inc"

.text
_fexpo:
		; setup
		push	W2				;Save W2

		;Saturate to 0x1CCC
		mov		#0x1CCC, W1		;max val
		cp		W0, W1			
		bra		lt, p2nosat		;if less than don't saturate
		mov		W1, W0			;otherwise load max
p2nosat:

		;scale for 1V/Oct
		mov		#ADC_cal, W1	;Scaling constant
		mul.uu	W0, W1, W0		;value*Scale -> W0:W1
		mov		#0x8000, W2		;1/2 (rounding constant)
		add		W0, W2, W0		;Add it
		clr		W0
		addc	W1, W0, W1		;W1 now contains rounded scaled result

		;expo lookup & shift
		mov.b	W1, W0			;LSbyte is table index
		sl		W0, #1, W0		;Multiply by 2
		mov		#tblpage(powtab), W2		;Set table page
		mov		W2, _TBLPAG
		mov		#tbloffset(powtab), W2		;Get Power Table Address
		add		W0, W2, W2		;Add it to index
		clr		B
		tblrdl	[W2], W0		;Read low word from table in program space
		mov		W0, ACCBL		;low word to acc
		tblrdh	[W2], W0		;Read high word from table in program space
		mov		W0, ACCBH		;high word to acc
		lsr		W1, #8, W1		;MSbye is exponent
		mov		#12, W0			;Normal range shift
		sub		W0, W1, W1
		sftac	B, W1			;Shift Acc by exponent
		mov		ACCBL, W0		;Get result
		mov		ACCBH, W1

		; teardown
		pop		W2				;retreive W2
		return
.end
