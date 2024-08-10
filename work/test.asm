; test program for Z80

	org $0
HALT:	equ $76

reset:	nop
	ld a, $55
	ld c, $AA
	out (c), a
	defb $ed, $71 ; out (c), 0 (undocumented)
	in a, (c)
	ld hl, $0000
	ld (hl), HALT
	jp reset
