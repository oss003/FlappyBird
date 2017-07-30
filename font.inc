font:
cap32:	.byte $00,$00,$00,$00,$00,$00	; SPACE-32
cap33:	.byte $30,$30,$30,$10,$30,$10	; !
cap34:	.byte $cc,$cc,$44,$00,$00,$00	; "
cap35:	.byte $cc,$fc,$dc,$fc,$dc,$44	; #
cap36:	.byte $3c,$f4,$70,$3c,$f4,$50	; $
cap37:	.byte $cc,$4c,$34,$d0,$c3,$41	; %
cap38:	.byte $30,$dc,$74,$dc,$7c,$14	; &
cap39:	.byte $c0,$c0,$40,$00,$00,$00	; '
cap40:	.byte $0c,$34,$30,$30,$1c,$04	; (
cap41:	.byte $c0,$70,$30,$30,$d0,$40	; )
cap42:	.byte $00,$cc,$74,$dc,$44,$00	; *
cap43:	.byte $00,$30,$fc,$74,$10,$00	; +
cap44:	.byte $00,$00,$00,$30,$d0,$40	; ,
cap45:	.byte $00,$00,$fc,$54,$00,$00	; -
cap46:	.byte $00,$00,$00,$00,$0c,$04	; .
cap47:	.byte $0c,$0c,$34,$d0,$c0,$40	; /

cap48:	.byte $30,$dc,$fc,$dc,$74,$10	; 0-48
cap49:	.byte $30,$f0,$70,$30,$fc,$54	; 1
cap50:	.byte $f0,$5c,$34,$d0,$fc,$54	; 2
cap51:	.byte $f0,$5c,$34,$1c,$f4,$50	; 3
cap52:	.byte $c0,$cc,$fc,$5c,$0c,$04	; 4
cap53:	.byte $fc,$d4,$70,$1c,$f4,$50	; 5
cap54:	.byte $3c,$d4,$f0,$dc,$74,$10	; 6
cap55:	.byte $fc,$5c,$34,$d0,$c0,$40	; 7
cap56:	.byte $30,$dc,$74,$dc,$74,$10	; 8
cap57:	.byte $30,$dc,$7c,$1c,$f4,$50	; 9

cap58:	.byte $00,$30,$10,$30,$10,$00	; :
cap59:	.byte $00,$30,$10,$30,$d0,$40	; ;
cap60:	.byte $0c,$34,$d0,$70,$1c,$04	; <
cap61:	.byte $00,$fc,$54,$fc,$54,$00	; =
cap62:	.byte $c0,$70,$1c,$34,$d0,$40	; >
cap63:	.byte $30,$dc,$4c,$34,$30,$10	; ?
cap64:	.byte $30,$dc,$cc,$c4,$70,$10	; @

cap65:	.byte $30,$dc,$fc,$dc,$cc,$44	; A-65
cap66:	.byte $f0,$dc,$f0,$dc,$f0,$50	; B
cap67:	.byte $3c,$d4,$c0,$c0,$3c,$14	; C
cap68:	.byte $f0,$dc,$cc,$cc,$f0,$50	; D
cap69:	.byte $fc,$d4,$f0,$d0,$fc,$54	; E
cap70:	.byte $fc,$d4,$f0,$d0,$c0,$40	; F
cap71:	.byte $3c,$d4,$c0,$cc,$3c,$14	; G
cap72:	.byte $cc,$cc,$fc,$dc,$cc,$44	; H
cap73:	.byte $fc,$74,$30,$30,$fc,$54	; I
cap74:	.byte $fc,$74,$30,$30,$d0,$40	; J
cap75:	.byte $cc,$f4,$f0,$f0,$dc,$44	; K
cap76:	.byte $c0,$c0,$c0,$c0,$fc,$54	; L
cap77:	.byte $cc,$fc,$dc,$cc,$cc,$44	; M
cap78:	.byte $cc,$fc,$fc,$fc,$dc,$44	; N
cap79:	.byte $30,$dc,$cc,$cc,$74,$10	; O
cap80:	.byte $f0,$dc,$f4,$d0,$c0,$40	; P
cap81:	.byte $30,$dc,$cc,$cc,$7c,$14	; Q
cap82:	.byte $f0,$dc,$f4,$dc,$cc,$44	; R
cap83:	.byte $3c,$d4,$30,$1c,$f4,$50	; S
cap84:	.byte $fc,$74,$30,$30,$30,$10	; T
cap85:	.byte $cc,$cc,$cc,$cc,$7c,$14	; U
cap86:	.byte $cc,$cc,$cc,$cc,$74,$10	; V
cap87:	.byte $cc,$cc,$cc,$fc,$dc,$44	; W
cap88:	.byte $cc,$74,$30,$30,$dc,$44	; X
cap89:	.byte $cc,$cc,$74,$30,$30,$10	; Y
cap90:	.byte $fc,$5c,$34,$d0,$fc,$54	; Z

cap91:	.byte $00,$00,$00,$00,$00,$00	; [
cap92:	.byte $00,$00,$00,$00,$00,$00	; \
cap93:	.byte $00,$00,$00,$00,$00,$00	; ]
cap94:	.byte $00,$00,$00,$00,$00,$00	; ^
cap95:	.byte $00,$00,$00,$00,$00,$00	; _

font_lb:
	.byte <cap32,<cap33,<cap34,<cap35,<cap36,<cap37,<cap38,<cap39
	.byte <cap40,<cap41,<cap42,<cap43,<cap44,<cap45,<cap46,<cap47
	.byte <cap48,<cap49,<cap50,<cap51,<cap52,<cap53,<cap54,<cap55
	.byte <cap56,<cap57,<cap58,<cap59,<cap60,<cap61,<cap62,<cap63
	.byte <cap64,<cap65,<cap66,<cap67,<cap68,<cap69,<cap70,<cap71
	.byte <cap72,<cap73,<cap74,<cap75,<cap76,<cap77,<cap78,<cap79
	.byte <cap80,<cap81,<cap82,<cap83,<cap84,<cap85,<cap86,<cap87
	.byte <cap88,<cap89,<cap90,<cap91,<cap92,<cap93,<cap94,<cap95
font_hb:
	.byte >cap32,>cap33,>cap34,>cap35,>cap36,>cap37,>cap38,>cap39
	.byte >cap40,>cap41,>cap42,>cap43,>cap44,>cap45,>cap46,>cap47
	.byte >cap48,>cap49,>cap50,>cap51,>cap52,>cap53,>cap54,>cap55
	.byte >cap56,>cap57,>cap58,>cap59,>cap60,>cap61,>cap62,>cap63
	.byte >cap64,>cap65,>cap66,>cap67,>cap68,>cap69,>cap70,>cap71
	.byte >cap72,>cap73,>cap74,>cap75,>cap76,>cap77,>cap78,>cap79
	.byte >cap80,>cap81,>cap82,>cap83,>cap84,>cap85,>cap86,>cap87
	.byte >cap88,>cap89,>cap90,>cap91,>cap92,>cap93,>cap94,>cap95
