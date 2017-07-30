;--------------------------------------------------------------------
;Atomic Flappy Bird
;
; Flappy bird port based upon the Flatom Dragon version
;--------------------------------------------------------------------

	.DEFINE asm_code $2c00
	.DEFINE header   1		; Header Wouter Ras emulator
	.DEFINE filenaam "ATOMICFLAPPY"

.org asm_code-22*header

.IF header
;********************************************************************
; ATM Header for Atom emulator Wouter Ras

name_start:
	.byte filenaam			; Filename
name_end:
	.repeat 16-name_end+name_start	; Fill with 0 till 16 chars
	  .byte $0
	.endrep

	.word asm_code			; 2 bytes startaddress
	.word exec			; 2 bytes linkaddress
	.word eind_asm-start_asm	; 2 bytes filelength

;********************************************************************
.ENDIF


exec = start_asm
start_asm:
	.include "atomicflappy.inc"	; Main program
	.include "getready.inc"		; Get Ready banner
	.include "tap.inc"		; Tap Key banner
	.include "gameover.inc"		; Game Over banner
	.include "title.inc"		; Title banner
	.include "font.inc"		; Text scroller font
	.include "score.inc"		; Scoreboard digits
eind_asm: