
	ORG $0

reset:
	nop
	ld a, $55
	ld c, $AA
	out (c), a
	defb $ed, $71 ; out (c), 0 (undocumented)
	in a, (c)
	ld hl, reset
	ld (hl), $76
	jp reset
