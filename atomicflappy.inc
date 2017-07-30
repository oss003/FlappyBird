;--------------------------------------------------------------------------
; ATOMIC FLAPPY BIRD
;
; Flappy bird port for the Acorn Atom based upon Flagon Bird for the Dragon
; written by Bosco (Steve Bamford)
;
; Program principle:
;
; - Main loop start
; -   Loop through every char in scr_current
; -      Execute action nr (=char nr) from ani_table
; -      Put changed screen chars in scr_update buffer
; -   Loop through every char in scr_update buffer 
; -      Plot changed char on screen
; -   Plot flappy
; -   Plot score
; -   Scan keys
; - Repeat main loop
;
; Variable arrays:
; - scr_base	= array of 32x12 characters for basic screen layout
; - scr_current	= array of 32x12 characters for displayed screen
; - scr_update	= array with screenbytes to be updated
;
; - ani_table	= table with 256 addresses of ani_routines called
;                 by action routine
; - sprites	= storage chars for bottombar, background, pipes, flappy
; - tiles	= storage graphics for tiles like start, scoreboard
; - numbers	= storage graphics for score numbers;                 
;
; Routine:
; - ani_action	= routine which steps through scr_current and executes
;		  action depending on character nr
; - scr_timer	= routine to display timer
; - scr_fl_bird	= routine do display flappy bird
; - prg_scankey	= routine to scan keys
; 
;--------------------------------------------------------------------------

;--------------------------------------------------------------------------
; Start Atomic Flappy Bird
;--------------------------------------------------------------------------

SID	= 1			; Quick hack for SIDplayer at $800 and 
				; Angry Bird song at $900-$2094

.IF SID

init_SID	= $800
silent_SID	= $842
start_SID	= $900
stop_SID	= $909

.ENDIF

display		= $8000
gr_mode		= $b000
timer		= $b808

stack_ptr	= 4

os_random	= $c986
os_wait_sync2	= $fe66
os_wait_sync 	= $fe6b
os_keyscan	= $fe71
os_waitkey	= $ffe3

ws		= $80

addr_ptr	= ws
addr_ptr2	= ws+2
addr_ptr3	= ws+4

x_pos		= ws+6
y_pos		= ws+7

jmp_addr	= ws+8

tmp_x		= ws+10
tmp_y		= ws+11
counter		= ws+12

chr_ptr		= ws+13
txt_x		= ws+14
txt_y		= ws+15
txt_flag	= ws+16

seed		= ws+17
score_flag	= ws+18

flappy_y	= ws+19
flappy_speed	= ws+20

key_pressed	= ws+21
key_released	= ws+22
hit_flag	= ws+23
pipe_top	= ws+24
pipe_bot	= ws+25
death_flag	= ws+26
flappy_xpos	= ws+27

save_x		= ws+28
save_y		= ws+29

digit1_buf	= ws+30	; 6 bytes
digit2_buf	= ws+36	; 6 bytes
digit3_buf	= ws+42	; 6 bytes

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Constants
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

flappy_upwards	= 6		
flappy_max	= 76
flappy_gravity	= 1

columns		= 32
rows		= 12

;--------------------------------------------------------------------------
; Switch to CLEAR3 colour mode and init vars
;
; char   = 4 x 8 coloured pixels
; MODE3a = 128 x 96 coloured pixels
;        = 32 x 12 chars
;--------------------------------------------------------------------------

program_entry:

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Init vars
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	lda #$ff		; Reset frame counter
	sta counter

	lda #0			; Flag to indicate end of game
	sta death_flag

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Screen init
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	lda #$90		; Switch to CLEAR3a
	sta gr_mode

.IF SID
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Init SID
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	jsr init_SID
	jsr silent_SID
	jsr stop_SID
.ENDIF

;--------------------------------------------------------------------------
; Display title screen with text scroller
;--------------------------------------------------------------------------

title_scr:
	lda #13			; Center Flappy horizontal
	sta flappy_xpos

	jsr plot_background	; Copy background to screen and mark as changed
	jsr update_scr		; Display background
	jsr reset_flappy	; Reset Flappy coordinates and clear buffer

	ldx #banner_title	; Display Get Ready banner
	jsr plot_banner

	lda #44			; Y-position Flappy
	sta flappy_y
	jsr backup_back		; Backup background into buffer

	lda #0
	sta chr_ptr		; Reset textpointer
	sta txt_flag		; Reset pauzeflag after displaying one line

	lda #<$86df		; Set Textscroller entry point
	sta txt_x
	lda #>$86df
	sta txt_y

title_loop:

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Display screen
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	jsr update_scr		; Print changed chars on screen
	jsr animate_scr		; Animate chars 
	
	lda flappy_y
	cmp #$22		; If Flappy to far down, emulate keypress
	bcc no_move_flappy1
	lda #$ff		; Emulate keypress
	sta key_pressed
no_move_flappy1:

	jsr flappy_scr		; Plot Flappy on screen and update coordinates

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Update text scroller
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	lda chr_ptr		; If not EOL, skip
	and #$1f
	bne no_wait

	inc txt_flag		; EOL, start incremening pauze counter
	lda txt_flag
	and #$7f
	bne skip_scroll		; Skip if pauzecounter<$80

	lda #0			; End of pauze, reset pauzeflag
	sta txt_flag

	lda chr_ptr
	cmp #text_scroll_len	; Check end of text
	bne no_wait
	ldx #0			; If end of text, reset textpointer
	stx chr_ptr
no_wait:
	ldx chr_ptr		; Set textpointer
	jsr textscroller	; Scroll text
	inx
	stx chr_ptr		; Point to next text scroll character

skip_scroll:
	inc counter		; Increment framecounter

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Check keypress
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	jsr os_keyscan		; Loop until keypress
	bcs title_loop

;--------------------------------------------------------------------------
; Start intro loop
;
; - Display screen
; - Display Get Ready banner
; - Display Tap key banner
; - Animate Flappy and wait for key
;--------------------------------------------------------------------------

start_intro:
	lda #2			; Position Flappy to the left
	sta flappy_xpos

	lda #0			; Mark Flappy undeath
	sta death_flag

	jsr plot_background	; Copy background to screen and mark as changed
	jsr update_scr		; Display background
	jsr reset_flappy

	ldx #banner_get_ready
	jsr plot_banner		; Display Get Ready banner
	ldx #banner_tap
	jsr plot_banner		; Display Tap key banner

intro_loop:
	jsr update_scr		; Print changed chars on screen
	jsr animate_scr		; Animate chars 
	
	lda flappy_y
	cmp #$2a		; If Flappy falling, emulate keypress
	bcc no_move_flappy
	lda #$ff		; Emulate keypress
	sta key_pressed
no_move_flappy:
	jsr flappy_scr		; Plot Flappy on screen and update coordinates

wait_loop:
	ldx #$f0
	jsr wait		; Wait ca.x*5 sec

	inc counter		; Increment framecounter

	jsr os_keyscan		; Loop until keypress
	bcs intro_loop

loop_entry:
	lda #0			; Reset score
	sta score
	sta score+1
	sta score+2
	sta score_flag

	jsr plot_background	; Restore background to screen
	jsr reset_flappy	; Reset Flappy coordinates and clear buffer

;--------------------------------------------------------------------------
; Start main game loop
;
; - Insert every 64 frames new pipe on screen
; - Backup score background in current screen array
; - Copy score in current screen array
; - Restore score background in current screen array
; - Update score
;--------------------------------------------------------------------------

.IF SID
	jsr start_SID
.ENDIF

main_loop:

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Insert new pipes after every 64 frames
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	lda counter		; Check if pipe has to be plotted
	and #$3f
	bne counteradd
	jsr insert_pipes	; Insert new pipe on right screenborder

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Set addresses correct number for digit
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

counteradd:
	ldx score		; Digit1
	lda num_lb,x
	sta numaddr1+1
	lda num_hb,x
	sta numaddr1+2

	ldx score+1		; Digit2
	lda num_lb,x
	sta numaddr2+1
	lda num_hb,x
	sta numaddr2+2

	ldx score+2		; Digit3
	lda num_lb,x
	sta numaddr3+1
	lda num_hb,x
	sta numaddr3+2

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Backup score background from scr_current
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	ldx #5
score_loop:
	lda score+2		; Check if 100-digit=0
	beq chk_bup2		; If 0 then check 10-digit=0

	ldy digit3,x		; Buffer background digit3 pos
	lda scr_current,y
	sta digit3_buf,x
numaddr3:
	lda $FFFF,x
	sta scr_current,y
	jmp skip_chk_bup2

chk_bup2:
	lda score+1		; Check if 10-digit=0
	beq backup1		; If 0 then backup 1-digit

skip_chk_bup2:
	ldy digit2,x		; Buffer background digit1 pos
	lda scr_current,y
	sta digit2_buf,x
numaddr2:
	lda $FFFF,x
	sta scr_current,y

backup1:
	ldy digit1,x		; Buffer background digit2 pos
	lda scr_current,y
	sta digit1_buf,x
numaddr1:
	lda $FFFF,x
	sta scr_current,y

	dex
	bpl score_loop

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Update screen
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	jsr update_scr		; Print changed chars on screen
	jsr flappy_scr		; Plot Flappy on screen and update coordinates

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Restore score background in scr_current
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	ldx #5
score_loop2:
	lda score+2		; Check if 100-digit=0
	beq chk_res2		; If 0 then check 10-digit=0

	ldy digit3,x		; Restore background digit3 pos
	lda digit3_buf,x
	and #$7f
	sta scr_current,y
	jmp skip_res_chk2

chk_res2:
	lda score+1		; Check if 10-digit=0
	beq restore1		; If 0 then restore 1-digit

skip_res_chk2:
	ldy digit2,x		; Restore background digit2 pos
	lda digit2_buf,x
	and #$7f
	sta scr_current,y

restore1:
	ldy digit1,x		; Restore background digit1 pos
	lda digit1_buf,x
	and #$7f
	sta scr_current,y

	dex
	bpl score_loop2

	jsr animate_scr		; Animate chars 
	jsr scan_key

	inc counter		; Increment framecounter
	jsr chk_flappy_hit
	bne end_game

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Check if score has to be updated
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	lda scr_current
	cmp #68+$80		; Check if pipebody is hitting left border screen
	beq update_score
	cmp #40+$80		; Check if pipetop is hitting left border screen
	beq update_score
	jmp main_loop

update_score:
	inc score		; Increment score digits

	lda score		; Check overflow 1-digit
	cmp #10
	bne go_loop
	lda #0
	sta score
	inc score+1

	lda score+1		; Check overflow 10-digit
	cmp #10
	bne go_loop
	lda #0
	sta score+1
	inc score+2
	jmp main_loop

no_pipe:
	lda #0
	sta score_flag
go_loop:
	jmp main_loop		; Repeat main game loop

;--------------------------------------------------------------------------
; Flappy death, game over
;--------------------------------------------------------------------------

end_game:

.IF SID
	jsr silent_SID
	jsr stop_SID
.ENDIF

	lda #1			; Mark Flappy hit
	sta death_flag

fall_loop:
	jsr flappy_scr		; Flappy falls down

	jsr os_wait_sync2	; Wait for next flyback

	lda flappy_y		; If Flappy falling, check for hit bottom
	cmp #$4c
	bcc fall_loop

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Restore background screen so remove pipes
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	jsr plot_background	; Copy background to screen and mark as changed
	jsr add_scoreboard	; Add scoreboard to background
	jsr update_scr		; Display background

	ldx #banner_game_over
	jsr plot_banner		; Display Game Over banner

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Display Flappy death
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	jsr calc_flappy_pos	; Display Flappy death
	jsr entry_death

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Check high-score
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	lda score+2		; Check 100-digit
	cmp hi_score+2
	bcc not_hi_score
	beq chk_hi10
	jmp set_hiscore
chk_hi10:
	lda score+1		; Check 10-digit
	cmp hi_score+1
	bcc not_hi_score
	bne set_hiscore

	lda score		; Check 1-digit
	cmp hi_score
	bcc not_hi_score
set_hiscore:
	lda score		; Copy score into high score
	sta hi_score
	lda score+1
	sta hi_score+1
	lda score+2
	sta hi_score+2
not_hi_score:

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Print text in scoreboard
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	ldx #<txt_score		; Print Score
	ldy #>txt_score
	jsr print_string

	ldx #<txt_best		; Print Best
	ldy #>txt_best
	jsr print_string

	ldx #<txt_rank		; Print Rank
	ldy #>txt_rank
	jsr print_string

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Print score in scoreboard
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	lda score+2		; Check if 100-digit=0
	beq sc_chk_prt2		; If 0 then check 10-digit=0

	ldx #<$8392		; Display 100-digit score
	ldy #>$8392
	jsr print_digit
	jmp sc_skip_chk_prt2
sc_chk_prt2:
	lda score+1		; Check if 10-digit=0
	beq sc_print1		; If 0 then print 1-digit
sc_skip_chk_prt2:
	ldx #<$8394		; Display 10-digit score
	ldy #>$8394
	lda score+1
	jsr print_digit
sc_print1:
	ldx #<$8396		; Display 1-digit score
	ldy #>$8396
	lda score
	jsr print_digit

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Print high score in scoreboard
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	lda hi_score+2		; Check if 100-digit=0
	beq hi_chk_prt2		; If 0 then check 10-digit=0

	ldx #<$8512		; Display 100-digit hi-score
	ldy #>$8512
	jsr print_digit
	jmp hi_skip_chk_prt2
hi_chk_prt2:
	lda hi_score+1		; Check if 10-digit=0
	beq hi_print1		; If 0 then print 1-digit
hi_skip_chk_prt2:
	ldx #<$8514		; Display 10-digit hi-score
	ldy #>$8514
	lda hi_score+1
	jsr print_digit
hi_print1:
	ldx #<$8516		; Display 1-digit hi_score
	ldy #>$8516
	lda hi_score
	jsr print_digit

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Print rank (0-9) depending on 10-digit score
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	ldx #9			; Set rank pointer
	stx save_x

	lda score+2		; If score>99 -> rank always highest
	bne rank_ok
rank_loop:
	lda save_x
	cmp score+1
	beq rank_ok		; Check 10-digit
	dec save_x
	bne rank_loop
rank_ok:
	ldx save_x		; Fill in rank nr
	lda rank_lb,x
	sta addr_ptr
	lda rank_hb,x
	sta addr_ptr+1
	ldy #9
	clc
	lda score
	adc #48
	sta (addr_ptr),y

	ldx addr_ptr		; Print rank text address
	ldy addr_ptr+1
	jsr print_string
 
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Loop until keypress
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	jsr os_waitkey		; Wait keypress
wait_release:
	jsr os_keyscan
	bcc wait_release	; Wait for release

	jmp start_intro		; Restart

;--------------------------------------------------------------------------
; Copy banner on screen
;--------------------------------------------------------------------------

plot_banner:
	lda banner_addr_lb,x	; Get banner address
	sta addr_ptr
	lda banner_addr_hb,x
	sta addr_ptr+1

	lda banner_x,x		; Get screen address
	sta addr_ptr2
	lda banner_y,x
	sta addr_ptr2+1

	lda banner_width,x	; Get banner width
	sta tmp_x
	lda banner_heigth,x	; Get banner heigth
	sta tmp_y

	lda #0
	sta y_pos		; Row counter
banner_y_loop:
	ldy #0			; Column counter
banner_x_loop:
	lda (addr_ptr),y	; Display 1 row
	sta (addr_ptr2),y
	iny
	cpy tmp_x
	bne banner_x_loop

	clc			; Point to next banner address
	lda addr_ptr
	adc banner_width,x
	sta addr_ptr
	lda #0
	adc addr_ptr+1
	sta addr_ptr+1

	clc			; Point to next screen address
	lda addr_ptr2
	adc #$20
	sta addr_ptr2
	lda #0
	adc addr_ptr2+1
	sta addr_ptr2+1

	inc y_pos		; Repeat for all rows
	lda y_pos
	cmp tmp_y
	bne banner_y_loop

	rts

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Banner data storage
; lb+hb Banneraddress, lb+hb Screen address, Banner width, Banner heigth
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

banner_get_ready	= 0
banner_tap		= 1
banner_game_over	= 2
banner_title		= 3

banner_addr_lb:	.byte <getready,<tap  ,<gameover,<title
banner_addr_hb:	.byte >getready,>tap  ,>gameover,>title
banner_x:	.byte <$8105   ,<$84c9,<$8045	,<$8005
banner_y:	.byte >$8105   ,>$84c9,>$8045   ,>$8005
banner_width:	.byte 22       ,14    ,22       ,23
banner_heigth:	.byte 24       ,18    ,20       ,47

;--------------------------------------------------------------------------
; Copy background into current screen and update screen
;
; - Copy complete screen background into current screen
; - Mark all char as changed
;--------------------------------------------------------------------------

plot_background:
	ldx #0			; Copy first 8 rows
x1_loop:
	lda scr_base,x		; Get background character
	ora #$80		; Mark as changed
	sta scr_current,x	; Copy into current screen
	inx
	bne x1_loop
x2_loop:
	lda scr_base+$100,x	; Get background character
	ora #$80		; Mark as changed
	sta scr_current+$100,x	; Copy into current screen
	inx
	bpl x2_loop

	rts

;--------------------------------------------------------------------------
; Insert pipes
;
; Every 64 frames, a new pipe must be plotted on the left screenborder
;--------------------------------------------------------------------------

insert_pipes:
	jsr get_random		; Get random nr 0-3
	and #3

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Insert new pipe
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	tax			; Point to initial pipe display

	lda pipeline0,x
	sta scr_current+0*columns+31	; Pos 31,0

	lda pipeline1,x
	beq pipe2
	sta scr_current+1*columns+31	; Pos 31,1
pipe2:
	lda pipeline2,x
	beq pipe3
	sta scr_current+2*columns+31	; Pos 31,2
pipe3:
	lda pipeline3,x
	beq pipe4
	sta scr_current+3*columns+31	; Pos 31,3
pipe4:
	lda pipeline4,x
	beq pipe5
	sta scr_current+4*columns+31	; Pos 31,4
pipe5:
	lda pipeline5,x
	beq pipe6
	sta scr_current+5*columns+31	; Pos 31,5
pipe6:
	lda pipeline6,x
	beq pipe7
	sta scr_current+6*columns+31	; Pos 31,6
pipe7:
	lda pipeline7,x
	beq pipe8
	sta scr_current+7*columns+31	; Pos 31,7
pipe8:
	lda pipeline8,x
	beq pipe9
	sta scr_current+8*columns+31	; Pos 31,8
pipe9:
	lda pipeline9,x
	beq pipe10
	sta scr_current+9*columns+31	; Pos 31,9
pipe10:
	lda pipeline10,x
	sta scr_current+10*columns+31	; Pos 31,10

	rts

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Storage initial pipe display, marked as changed
;    0     = Nothing
;   40+$80 = Char top pipe marked as changed
;   68+$80 = Char body pipe marked as changed
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

pipeline0:	.byte 40+$80,68+$80,68+$80,68+$80
pipeline1:	.byte      0,40+$80,68+$80,68+$80
pipeline2:	.byte      0,     0,40+$80,68+$80
pipeline3:	.byte      0,     0,     0,40+$80
pipeline4:	.byte      0,     0,     0,     0
pipeline5:	.byte      0,     0,     0,     0
pipeline6:	.byte      0,     0,     0,     0
pipeline7:	.byte 40+$80,     0,     0,     0
pipeline8:	.byte 68+$80,40+$80,     0,     0
pipeline9:	.byte 68+$80,68+$80,40+$80,     0
pipeline10:	.byte 68+$80,68+$80,68+$80,40+$80

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Storage hit positions Flappy for top-/bottom frames:
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

hit_top:	.byte 8,16,24,32
hit_bot:	.byte 48,56,64,72

;--------------------------------------------------------------------------
; Plot changed characters in update screen array
;
; - Loop trough current screen array
; -   Plot every char which is changed (bit7=set)
; -   Reset bit7 to unchanged (bit7=unset)
; - Repeat loop
;
; Timing: 2 + 12*(24 + 32*(19+198)+2 + 6) + 6 
;         row: 6946
;         tot: 83720 usec
;--------------------------------------------------------------------------

update_scr:
	ldx #0			;2 Loop y counter
x5_loop:
	lda scrcur_addr_lb,x	;4 Set scr_current pointer
	sta x6_loop+1		;4
	sta updptr+1		;4
	lda scrcur_addr_hb,x	;4 Set scr_current pointer
	sta x6_loop+2		;4
	sta updptr+2		;4

	ldy #0			;2 Loop x counter
x6_loop:
	lda $ffff,y		;4
	bpl no_update		;2 Skip if character is not changed
	and #$7f		;2 Reset changed bit
	jsr plot_tile		;198 Plot char
updptr:
	sta $ffff,y		;5 Mark char in current screen array as unchanged
no_update:
	iny			;2
	cpy #columns		;2
	bne x6_loop		;2 Repeat x

	inx			;2
	cpx #rows		;2
	bne x5_loop		;2 Repeat y

	rts			;6

;--------------------------------------------------------------------------
; Animate characters from current screen
;
; - Loop trough current screen array
; -   Get character value
; -   Read address corresponding animation routine
; -   Jump animation subroutine
; - Repeat loop
;--------------------------------------------------------------------------

animate_scr:
	ldx #0			; y counter
x7_loop:
	lda scrcur_addr_lb,x	; Set scr_current pointer
	sta addr_ptr2
	lda scrcur_addr_hb,x	; Set scr_current pointer
	sta addr_ptr2+1

	ldy #0			; x counter
x8_loop:
	sty tmp_y		; Save y
	lda (addr_ptr2),y	; Read char number
	tay
	lda ani_table_hb,y	; Get hb animation routine
	sta jmp_addr+1
	bmi no_update2
	lda ani_table_lb,y	; Get lb animation routine
	sta jmp_addr

	ldy tmp_y		; Restore y
	jsr animate		; Execute action

no_update2:
	ldy tmp_y
	iny
	cpy #columns
	bne x8_loop		; Repeat x

	inx
	cpx #rows
	bne x7_loop		; Repeat y

	rts

animate:
	jmp (jmp_addr)		; Jump to animation routine

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Current screen row address table
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

scrcur_addr_lb:
	.byte <scr_current,<(scr_current+1*columns)
	.byte <(scr_current+2*columns),<(scr_current+3*columns)
	.byte <(scr_current+4*columns),<(scr_current+5*columns)
	.byte <(scr_current+6*columns),<(scr_current+7*columns)
	.byte <(scr_current+8*columns),<(scr_current+9*columns)
	.byte <(scr_current+10*columns),<(scr_current+11*columns)
scrcur_addr_hb:
	.byte >scr_current,>(scr_current+1*columns)
	.byte >(scr_current+2*columns),>(scr_current+3*columns)
	.byte >(scr_current+4*columns),>(scr_current+5*columns)
	.byte >(scr_current+6*columns),>(scr_current+7*columns)
	.byte >(scr_current+8*columns),>(scr_current+9*columns)
	.byte >(scr_current+10*columns),>(scr_current+11*columns)

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Current background screen row address table
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

scrbase_addr_lb:
	.byte <scr_base,<(scr_base+1*columns)
	.byte <(scr_base+2*columns),<(scr_base+3*columns)
	.byte <(scr_base+4*columns),<(scr_base+5*columns)
	.byte <(scr_base+6*columns),<(scr_base+7*columns)
	.byte <(scr_base+8*columns),<(scr_base+9*columns)
	.byte <(scr_base+10*columns),<(scr_base+11*columns)
scrbase_addr_hb:
	.byte >scr_base,>(scr_base+1*columns)
	.byte >(scr_base+2*columns),>(scr_base+3*columns)
	.byte >(scr_base+4*columns),>(scr_base+5*columns)
	.byte >(scr_base+6*columns),>(scr_base+7*columns)
	.byte >(scr_base+8*columns),>(scr_base+9*columns)
	.byte >(scr_base+10*columns),>(scr_base+11*columns)

;--------------------------------------------------------------------------
; Animation routines
;
; - Read address animation routine from animation table
; - Execute animation routine
;--------------------------------------------------------------------------

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Bottom bar pixel shift
; Change char 0->1, 1->2 ... 6->7 and 7->0
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

ani0:				; Shift character 1 bit left in byte, cycle=8 chars
	lda (addr_ptr2),y	; Get screen character
	clc
	adc #1
	and #7			; Add 1 and mask only 8 sprites
	ora #$80		; Mark as changed
	sta (addr_ptr2),y	; Set screen character
	rts

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Pipe top and body pixel shift
; Change char 40->41, 41->42 ... and 66->67
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

ani1:				; Shift character 1 bit left in byte, cycle=28 chars
	lda (addr_ptr2),y	; Get screen character
	clc
	adc #1
	cmp #68			; Add 1 and mask only 28 sprites
	bne no_reset
	lda #40			; Set start pipe char
no_reset:
	ora #$80		; Mark as changed
	sta (addr_ptr2),y	; Get screen character
	rts

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Pipe top and body shifting out of byte boundry
; Pixel overflow left byte boundry, copy pipe chars 1 byte to the left
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

ani2:				; Shift character to previous character
	cpy #0			; Do nothing at left border screen
	beq end_ani2

	lda (addr_ptr2),y	; Get pipe left front char
	sec
	sbc #3			; Decrement by 3
	dey
	ora #$80		; Mark as changed
	sta (addr_ptr2),y	; Set char 1 byte to left on screen

	iny
	clc
	adc #4			; Add 4 to previous char
	ora #$80		; Mark as changed
	sta (addr_ptr2),y	; Set char on previous screen pos
end_ani2:
	rts

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Pipe top and body restoring background after last byte
; Pixel overflow right byte boundry, restore background
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

ani3:
	cpy #0			; Do nothing at left border screen
	beq end_ani3

	lda (addr_ptr2),y	; Get pipe left front char
	sec
	sbc #3			; Decrement by 3
	dey
	ora #$80		; Mark as changed
	sta (addr_ptr2),y	; Set char 1 byte to left on screen

	iny
end_ani3:
	lda scrbase_addr_lb,x	; Set scr_current pointer
	sta addr_ptr3
	lda scrbase_addr_hb,x
	sta addr_ptr3+1
	lda (addr_ptr3),y	; Get background char
	ora #$80		; Mark as changed
	sta (addr_ptr2),y	; Set char on previous screen pos
	rts

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Flappy animation
; Change char 0->1, 1->2 2-3 and 3->0
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

ani4:
	lda counter		; Animate flappy once every 2 frames
	and #1
	beq end_ani4

	lda (addr_ptr2),y	; Rotate Flappy tilenr bit0/1 -> 0-1-2-3-0-1-2-....
	and #$fc
	sta tmp_nr
	lda (addr_ptr2),y
	clc
	adc #1
	and #3
	adc tmp_nr
	ora #$80		; Mark as changed
	sta (addr_ptr2),y
end_ani4:
	rts

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Dummy entry 
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

no_ani:
	rts		;	= $ffff

tmp_nr:	.byte 0

;--------------------------------------------------------------------------
; Add scoreboard to current screen to be displayed
; At pos 7,3 width a width of 18 and height of 5
;
; Top border: 119-120-121
; Middle    : 122-123-124
; Bottom    : 125-126-127
;--------------------------------------------------------------------------

add_scoreboard:
	lda #119+$80		; Left top
	sta scr_current+7+3*32

	lda #122+$80		; Left
	sta scr_current+7+4*32
	sta scr_current+7+5*32
	sta scr_current+7+6*32

	lda #125+$80		; Left bottom
	sta scr_current+7+7*32

	ldx #8
top_loop:
	lda #120+$80
	sta scr_current+3*32,x	; Top middle
	lda #123+$80
	sta scr_current+4*32,x	; Middle
	sta scr_current+5*32,x
	sta scr_current+6*32,x
	lda #126+$80
	sta scr_current+7*32,x	; Bottom middle
	inx
	cpx #24
	bne top_loop

	lda #121+$80		; Top right
	sta scr_current+24+3*32

	lda #124+$80
	sta scr_current+24+4*32	; Right
	sta scr_current+24+5*32
	sta scr_current+24+6*32

	lda #127+$80
	sta scr_current+24+7*32	; Bottom right

	rts

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Scoreboard text
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

txt_score:	.byte <$83c8,>$83c8,"SCORE",13
txt_best:	.byte <$8548,>$8548,"BEST",13
txt_rank:	.byte <$86e8,>$86e8,"RANK",13
txt_cscore:	.byte <$83d6,>$83d6,"00",13
txt_hi_cscore:	.byte <$8556,>$8556,"00",13

rank_pos	= $86f0

txt_rank0:	.byte <rank_pos,>rank_pos,"  FAFFY.",13
txt_rank1:	.byte <rank_pos,>rank_pos,"  GOOFY.",13
txt_rank2:	.byte <rank_pos,>rank_pos," DROOPY.",13
txt_rank3:	.byte <rank_pos,>rank_pos,"AVERAGE.",13
txt_rank4:	.byte <rank_pos,>rank_pos,"  SUPER.",13
txt_rank5:	.byte <rank_pos,>rank_pos,"   HERO.",13
txt_rank6:	.byte <rank_pos,>rank_pos,"   ICON.",13
txt_rank7:	.byte <rank_pos,>rank_pos,"   KING.",13
txt_rank8:	.byte <rank_pos,>rank_pos,"EMPEROR.",13
txt_rank9:	.byte <rank_pos,>rank_pos,"    GOD.",13

rank_lb:	.byte <txt_rank0,<txt_rank1,<txt_rank2,<txt_rank3
		.byte <txt_rank4,<txt_rank5,<txt_rank6,<txt_rank7
		.byte <txt_rank8,<txt_rank9

rank_hb:	.byte >txt_rank0,>txt_rank1,>txt_rank2,>txt_rank3
		.byte >txt_rank4,>txt_rank5,>txt_rank6,>txt_rank7
		.byte >txt_rank8,>txt_rank9

;--------------------------------------------------------------------------
; Display Flappy Bird and update coordinates (startposition y=36, $84c0)
;
; - Copy background back to current position Flappy on screen
; - Copy new position Flappy from screen to background
; - Plot Flappy Bird
;--------------------------------------------------------------------------

flappy_scr:
	jsr calc_flappy_pos	; Calculate screen address

	jsr os_wait_sync	; Wait for vsync

	ldx #0
flappy_loop1:
	ldy scrtab,x
	lda flappy_bg_buffer,x	; Remove Flapppy top chars
	sta (addr_ptr),y
	inx
	cpx #32
	bne flappy_loop1
	inc addr_ptr+1
flappy_loop2:
	ldy scrtab-32,x
	lda flappy_bg_buffer,x	; Remove Flappy bottom chars
	sta (addr_ptr),y
	inx
	cpx #64
	bne flappy_loop2

	bit key_pressed		; If key pressed speed = -upward_speed
	bpl not_pressed
	lda #flappy_upwards
	eor #$ff
	clc
	adc #1
	sta flappy_speed
	lda #0
	sta key_pressed

not_pressed:
	clc			; speed = speed + gravity
	lda flappy_speed
	adc #flappy_gravity
	sta flappy_speed

	clc			; sprite_y = sprite_y + speed
	adc flappy_y
	sta flappy_y

	bpl chk_bottom
	lda #0			; speed=0
	sta flappy_speed
	jmp not_on_bottom

chk_bottom:
	cmp #flappy_max		; If sprite_y > max
	bcc not_on_bottom
	lda #0			;  speed=0
	sta flappy_speed
entry_death:
	lda #flappy_max		;  flappy_y = max

not_on_bottom:
	sta flappy_y		; Set Flappy-y

skip_frame:
	jsr backup_back		; Restore background
	
	ldx #4			; If Flappy death, sprite=4
	lda death_flag
	bne flappy_death

	lda counter		; Determine flying Flappy sprite
	lsr a
	and #3
	tax

flappy_death:
	lda flappy_ani_lb,x	; Set sprite data address
	sta fl_addr1+1
	sta fl_addr2+1
	lda flappy_ani_hb,x
	sta fl_addr1+2
	sta fl_addr2+2

	lda flappy_msk_lb,x	; Set sprite mask address
	sta fl_mask1+1
	sta fl_mask2+1
	lda flappy_msk_hb,x
	sta fl_mask1+2
	sta fl_mask2+2

	dec addr_ptr+1		; Adjust Hi-byte screen address

	ldx #0
flappy_loop5:			; Upper half Flappy
	ldy scrtab,x
	lda (addr_ptr),y	; Get screen data
fl_mask1:
	and fma96,x		; Mask screen data
fl_addr1:
	ora ch96,x		; Add Flapppy data
	sta (addr_ptr),y	; Display data on screen
	inx
	cpx #32
	bne flappy_loop5

	inc addr_ptr+1		; Adjust Hi-byte screen address

flappy_loop6:			; Lower half Flappy
	ldy scrtab-32,x
	lda (addr_ptr),y	; Get screen data
fl_mask2:
	and fma96,x		; Mask screen data
fl_addr2:
	ora ch96,x		; Add Flapppy data
	sta (addr_ptr),y	; Display data on screen
	inx
	cpx #64
	bne flappy_loop6

	rts

flappy_bg_buffer:		; Backup background flappy
	.byte 0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0
	.byte 0,0,0,0,0,0,0,0

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Calculate screen address from flappy y-coordinate
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

calc_flappy_pos:
	lda flappy_y		; Calculate screenaddress Flappy top chars
	lsr a
	lsr a
	lsr a
	ora #$80
	sta addr_ptr+1
	lda flappy_y
	and #7
	tax
	clc
	lda scrtab,x
	adc flappy_xpos
	sta addr_ptr
	bne no_boundry
	inc addr_ptr+1
no_boundry:
	rts

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Reset flappy initial y coordinate and clear buffer
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

reset_flappy:

	lda #38
	sta flappy_y		; Initial Flappy y-position
	lda #0
	sta flappy_speed	; Flappy acceleration speed

	ldx #63			; Clear background buffer flappy
clear_buffer:
	sta flappy_bg_buffer,x
	dex
	bpl clear_buffer
	rts

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Backup background into buffer
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

backup_back:
	jsr calc_flappy_pos	; Calculate Flappy screen address

	ldx #0
flappy_loop3:
	ldy scrtab,x
	lda (addr_ptr),y	; Backup screen Flapppy top half
	sta flappy_bg_buffer,x
	inx
	cpx #32
	bne flappy_loop3
	inc addr_ptr+1
flappy_loop4:
	ldy scrtab-32,x
	lda (addr_ptr),y	; Backup screen Flappy bottom half
	sta flappy_bg_buffer,x
	inx
	cpx #64
	bne flappy_loop4

	rts

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Check if Flappy hits pipes
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

chk_flappy_hit:
	lda #0			; Reset hitflag
	sta hit_flag

	ldx flappy_y		; Get Flappy y-pos

	lda flappy_speed	; Check if flappy is going up or down
	bmi flappy_up		; Checkpoint is top Flappy sprite
flappy_down:
	txa			; Checkpoint is bottom Flappy sprite
	clc
	adc #12
	tax
flappy_up:
	txa			; Calculate row in current screen array
	lsr a
	lsr a
	lsr a
	tax

	lda scrcur_addr_lb,X	; Set current screen array address
	sta addr_ptr
	lda scrcur_addr_hb,x
	sta addr_ptr+1

	ldy #2
chk_hit:
	lda (addr_ptr),y	; Get current screen tile
	cmp #41			; Check if tilenr is >40 (pipe)
	bcc no_hit
	inc hit_flag		; Flappy hit pipe, set hitflag
no_hit:
	iny
	cpy #6			; Flappy is 4 bytes wide, check all tiles
	bne chk_hit

	lda hit_flag		; Return with hitflag
	rts

;--------------------------------------------------------------------------
; Plot character
;
; - Plot char A at Y,X
;     A = character
;     Y = X-coordinate !!!!!!
;     X = Y-coordinate !!!!!!
;
; Timing: 44 + 17*8+2 + 16 = 198 usec/char
;--------------------------------------------------------------------------

plot_tile:
	stx save_x		;3 Save x,y
	sty save_y		;3
	pha			;3

	tax			;1 Get character data address
	lda ch_lb,x		;4
	sta datplt+1		;4
	lda ch_hb,x		;4
	sta datplt+2		;4

	ldx save_y		;3 Calculate screen address
	stx scrplt+1		;4
	lda save_x		;3
	ora #$80		;2
	sta scrplt+2		;4

	ldx #7			;2 Row counter
loop:
datplt:
	lda $ffff,x		;4 Get char data
	ldy scrtab,x		;4
scrplt:
	sta $ffff,y		;5 Plot byte on screen
	dex			;2
	bpl loop		;2 Repeat 8 rows

	pla			;4
	ldy save_y		;3 Restore x,y
	ldx save_x		;3
	rts			;6
	
scrtab:
	.byte 0,$20,$40,$60,$80,$a0,$c0,$e0
	.byte 1,$21,$41,$61,$81,$a1,$c1,$e1
	.byte 2,$22,$42,$62,$82,$a2,$c2,$e2
	.byte 3,$23,$43,$63,$83,$a3,$c3,$e3
dataptr:
	.byte 0,1,2,3,4,5,6,7

score:
	.byte 0,0,0
hi_score:
	.byte 0,0,0

;--------------------------------------------------------------------------
; Text scroller
;--------------------------------------------------------------------------

textscroller:
	lda text0,x		; Get textscroll character
	jsr plot_chr		; Put character on right border screen

	ldy #0			; Scroll all chars of scroller 1 to left
scroll_line:
	lda $86c1,y
	sta $86c0,y
	lda $86e1,y
	sta $86e0,y
	lda $8701,y
	sta $8700,y
	lda $8721,y
	sta $8720,y
	lda $8741,y
	sta $8740,y
	lda $8761,y
	sta $8760,y
	iny
	cpy #31
	bne scroll_line

	lda #0			; Erase character
	sta $86df
	sta $86ff
	sta $871f
	sta $873f
	sta $875f
	sta $877f

	rts

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Text scroller for 8 lines of 32 chars
;
;              01234567890123456789012345678901
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

text0:	.byte "      ATOMIC FLAPPY BIRD 2014   "
	.byte "     WRITTEN BY KC  BASED UPON  "
	.byte "     FLAGONBIRD FOR THE DRAGON  "
	.byte "       WRITTEN BY BOSCO 2014    "
	.byte "       PRESS A KEY TO START     "
text1:
	text_scroll_len=text1-text0

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Put character in right border on screen
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

plot_chr:
	stx tmp_x
	sty tmp_y

	sec			; Get characterdata address
	sbc #32
	tay
	lda font_lb,y
	sta addr_ptr
	lda font_hb,y
	sta addr_ptr+1

	lda txt_x
	sta chr_addr+1
	sta chr_addr+4
	lda txt_y
	sta chr_addr+2
	sta chr_addr+5

	ldy #0			; Display line0
chr_loop:
	ldx scrtab,y
	lda (addr_ptr),y
chr_addr:
	ora $86df,x
	sta $86df,x
	iny			; Display line1
	cpy #6
	bne chr_loop

	ldy tmp_y
	ldx tmp_x
	rts

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Print string from given address until char=13
;
; X=lb address
; Y=hb address
;
; address: lb_screen,hb_screen,"string",13
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

print_string:
	stx addr_ptr2		; Set address pointer
	sty addr_ptr2+1

	ldy #0
	lda (addr_ptr2),y	; Set screenaddress
	sta txt_x
	iny
	lda (addr_ptr2),y
	sta txt_y

print_loop:
	iny
	lda (addr_ptr2),y	; Get string character
	cmp #13			; Stop if char=13
	beq end_print_string
	jsr plot_chr		; Print char
	inc txt_x
	jmp print_loop		; Next char

end_print_string:
	rts

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Print digit 8x10 in scoreboard at address given
;
; X=lb address
; Y=hb address
; A=number to print
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

print_digit:
	stx addr_ptr2		; Set address pointer
	sty addr_ptr2+1

	tax			; Set digit data address
	lda sc_digit_lb,x
	sta digit_addr+1
	sta digit_addr2+1
	lda sc_digit_hb,x
	sta digit_addr+2
	sta digit_addr2+2

	ldx #0
digit1_loop:
	ldy sc_tab,x
digit_addr:
	lda $8000,x		; Get digit data
	sta (addr_ptr2),y	; Display first 8 lines on screen
	inx
	cpx #16
	bne digit1_loop

	inc addr_ptr2+1
digit_loop2:
	ldy sc_tab-16,x
digit_addr2:
	lda $8000,x		; Get digit data
	sta (addr_ptr2),y	; Display 2 last lines on screen
	inx
	cpx #20
	bne digit_loop2

	rts
 
;--------------------------------------------------------------------------
; Scan key
;
; - Scan for key press (carry set is pressed, carry clear is no press)
; - If pressed and key released in between, keypress flag set
;--------------------------------------------------------------------------

scan_key:
	jsr os_keyscan		; Check if key pressed
	bcc no_key_pressed

	bit key_released	; If pressed, check for release
	bmi not_released

	lda #$ff		; Mark key pressed
	sta key_released
	sta key_pressed
	jmp end_keyscan

not_released:
	lda #0			; Mark key not released
	sta key_pressed
	jmp end_keyscan

no_key_pressed:
	lda #0			; Mark key released
	sta key_released
end_keyscan:
	rts

;--------------------------------------------------------------------------
; Wait
;
; - Wait for 6 + x*5 + 6 usec
;--------------------------------------------------------------------------

wait:
	dex			; Wait a while
	bne wait
	rts

;--------------------------------------------------------------------------
; Get random nr, cycled after 256 times
;--------------------------------------------------------------------------

get_random:
	lda seed
	sta tmp_seed
	beq doEor
	asl a
	beq noEor
	bcc noEor
doEor:
	eor #$1d
noEor:
	sta seed
	eor tmp_seed
	and #3
	bne end_random
	inc seed
end_random:
	lda seed
	rts

tmp_seed:	.byte 0

;--------------------------------------------------------------------------
; Sprite data storage
;
; - Char 0-255 data storage 
; - Char = 4 x 8 coloured pixels
; - Coloured pixel 00 = green
;                  01 = yellow
;                  10 = blue
;                  11 = red
;--------------------------------------------------------------------------

sprites:

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Bottom bar, line11 (cycle every 2 chars)
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

ch0:	.byte $ff,$fd,$f5,$d5,$55,$ff,$55,$55	; First byte 4x shifted left
ch1:	.byte $ff,$f5,$d5,$55,$57,$ff,$55,$55
ch2:	.byte $ff,$d5,$55,$57,$5f,$ff,$55,$55
ch3:	.byte $ff,$55,$57,$5f,$7f,$ff,$55,$55

ch4:	.byte $ff,$57,$5f,$7f,$ff,$ff,$55,$55	; Second byte 4x shifted left
ch5:	.byte $ff,$5f,$7f,$ff,$fd,$ff,$55,$55
ch6:	.byte $ff,$7f,$ff,$fd,$f5,$ff,$55,$55
ch7:	.byte $ff,$ff,$fd,$f5,$d5,$ff,$55,$55

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Buildings sprites, lines 7-10 (static)
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

ch8:	.byte $00,$00,$00,$00,$00,$00,$01,$05	; Line7, sky
ch9:	.byte $00,$00,$00,$00,$01,$15,$55,$55
ch10:	.byte $00,$00,$00,$00,$55,$55,$55,$55
ch11:	.byte $00,$00,$00,$00,$40,$54,$55,$55
ch12:	.byte $00,$00,$00,$00,$00,$00,$40,$50
ch13:	.byte $00,$00,$00,$00,$00,$00,$00,$00
ch14:	.byte $00,$00,$00,$00,$00,$00,$00,$00
ch15:	.byte $00,$00,$00,$00,$00,$00,$00,$00

ch16:	.byte $15,$55,$aa,$59,$a9,$55,$dd,$dd	; Line8, sky + buildings
ch17:	.byte $55,$55,$a9,$59,$d9,$59,$da,$d9
ch18:	.byte $55,$55,$55,$55,$55,$55,$a5,$65
ch19:	.byte $55,$55,$55,$55,$55,$55,$55,$55
ch20:	.byte $50,$54,$55,$55,$55,$55,$55,$55
ch21:	.byte $01,$15,$55,$55,$55,$55,$5a,$59
ch22:	.byte $55,$55,$55,$55,$55,$55,$aa,$55
ch23:	.byte $40,$54,$56,$56,$5a,$59,$99,$99

ch24:	.byte $55,$dd,$dd,$55,$55,$dd,$dd,$55	; Line9, buildings
ch25:	.byte $5a,$d9,$d9,$59,$59,$d9,$d9,$59
ch26:	.byte $aa,$55,$55,$77,$77,$55,$77,$77
ch27:	.byte $a5,$65,$65,$65,$65,$65,$65,$65
ch28:	.byte $55,$5a,$59,$59,$59,$59,$59,$59
ch29:	.byte $59,$aa,$55,$dd,$dd,$55,$dd,$dd
ch30:	.byte $d5,$aa,$55,$d5,$d5,$6a,$e5,$e7
ch31:	.byte $99,$99,$99,$99,$99,$a9,$59,$59

ch32:	.byte $dd,$ad,$0a,$00,$02,$08,$80,$00	; Line10, bush
ch33:	.byte $d9,$d8,$40,$8a,$a0,$00,$00,$00
ch34:	.byte $55,$03,$00,$aa,$02,$00,$00,$00
ch35:	.byte $65,$6a,$a0,$00,$a0,$0a,$00,$00
ch36:	.byte $59,$a9,$0a,$00,$02,$08,$80,$00
ch37:	.byte $55,$dd,$00,$8a,$a0,$00,$00,$00
ch38:	.byte $67,$e5,$65,$aa,$02,$00,$00,$00
ch39:	.byte $56,$5a,$a0,$00,$a0,$0a,$00,$00

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Pipe top (cylce every 7 chars)
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

ch40:	.byte $02,$02,$02,$02,$02,$02,$02,$02	; First byte 4x shifted left
ch41:	.byte $0a,$0b,$0b,$0b,$0b,$0b,$0b,$0a
ch42:	.byte $2a,$2f,$2d,$2d,$2d,$2d,$2f,$2a
ch43:	.byte $aa,$bf,$b5,$b5,$b5,$b5,$bf,$aa

ch44:	.byte $aa,$ff,$d5,$d5,$d5,$d5,$ff,$aa	; Second byte 4x shifted left
ch45:	.byte $aa,$ff,$55,$55,$55,$55,$ff,$aa
ch46:	.byte $aa,$ff,$57,$57,$57,$57,$ff,$aa
ch47:	.byte $aa,$ff,$5d,$5d,$5d,$5d,$ff,$aa

ch48:	.byte $aa,$ff,$77,$77,$77,$77,$ff,$aa	; Third byte 4x shifted left
ch49:	.byte $aa,$ff,$df,$df,$df,$df,$ff,$aa
ch50:	.byte $aa,$ff,$7f,$7f,$7f,$7f,$ff,$aa
ch51:	.byte $aa,$ff,$ff,$ff,$ff,$ff,$ff,$aa

ch52:	.byte $aa,$ff,$ff,$ff,$ff,$ff,$ff,$aa	; Fourth byte 4x shifted left
ch53:	.byte $aa,$ff,$ff,$ff,$ff,$ff,$ff,$aa
ch54:	.byte $aa,$ff,$ff,$ff,$ff,$ff,$ff,$aa
ch55:	.byte $aa,$ff,$ff,$ff,$ff,$ff,$ff,$aa

ch56:	.byte $aa,$ff,$ff,$ff,$ff,$ff,$ff,$aa	; Fifth byte 4x shifted left
ch57:	.byte $aa,$ff,$ff,$ff,$ff,$ff,$ff,$aa
ch58:	.byte $aa,$ff,$ff,$ff,$ff,$ff,$ff,$aa
ch59:	.byte $aa,$ff,$fd,$fd,$fd,$fd,$ff,$aa

ch60:	.byte $aa,$ff,$f7,$f7,$f7,$f7,$ff,$aa	; Sixth byte 4x shifted left
ch61:	.byte $aa,$ff,$df,$df,$df,$df,$ff,$aa
ch62:	.byte $aa,$fe,$7e,$7e,$7e,$7e,$fe,$aa
ch63:	.byte $a8,$f8,$f8,$f8,$f8,$f8,$f8,$a8

ch64:	.byte $a0,$e0,$e0,$e0,$e0,$e0,$e0,$a0	; Seventh byte 4x shifted left
ch65:	.byte $80,$80,$80,$80,$80,$80,$80,$80
ch66:	.byte 0,0,0,0,0,0,0,0
ch67:	.byte 0,0,0,0,0,0,0,0

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Pipe body (cylce every 7 chars)
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

ch68:	.byte 0,0,0,0,0,0,0,0			; First byte 4x shifted left
ch69:	.byte $02,$02,$02,$02,$02,$02,$02,$02
ch70:	.byte $0b,$0b,$0b,$0b,$0b,$0b,$0b,$0b
ch71:	.byte $2d,$2d,$2d,$2d,$2d,$2d,$2d,$2d

ch72:	.byte $b5,$b5,$b5,$b5,$b5,$b5,$b5,$b5	; Second byte 4x shifted left
ch73:	.byte $d5,$d5,$d5,$d5,$d5,$d5,$d5,$d5
ch74:	.byte $55,$55,$55,$55,$55,$55,$55,$55
ch75:	.byte $57,$57,$57,$57,$57,$57,$57,$57

ch76:	.byte $5d,$5d,$5d,$5d,$5d,$5d,$5d,$5d	; Third byte 4x shifted left
ch77:	.byte $77,$77,$77,$77,$77,$77,$77,$77
ch78:	.byte $df,$df,$df,$df,$df,$df,$df,$df
ch79:	.byte $7f,$7f,$7f,$7f,$7f,$7f,$7f,$7f

ch80:	.byte $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff	; Fourth byte 4x shifted left
ch81:	.byte $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff
ch82:	.byte $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff
ch83:	.byte $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff

ch84:	.byte $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff	; Fifth byte 4x shifted left
ch85:	.byte $ff,$ff,$ff,$ff,$ff,$ff,$ff,$ff
ch86:	.byte $fd,$fd,$fd,$fd,$fd,$fd,$fd,$fd
ch87:	.byte $f7,$f7,$f7,$f7,$f7,$f7,$f7,$f7

ch88:	.byte $df,$df,$df,$df,$df,$df,$df,$df	; Sixth byte 4x shifted left
ch89:	.byte $7e,$7e,$7e,$7e,$7e,$7e,$7e,$7e
ch90:	.byte $f8,$f8,$f8,$f8,$f8,$f8,$f8,$f8
ch91:	.byte $e0,$e0,$e0,$e0,$e0,$e0,$e0,$e0

ch92:	.byte $80,$80,$80,$80,$80,$80,$80,$80	; Seventh byte 4x shifted left
ch93:	.byte 0,0,0,0,0,0,0,0
ch94:	.byte 0,0,0,0,0,0,0,0
ch95:	.byte 0,0,0,0,0,0,0,0

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Score
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

ch96:	.byte $aa,$95,$95,$95,$95,$95,$95,$95	; 0
ch97:	.byte $aa,$55,$55,$55,$65,$65,$65,$65
ch98:	.byte $a8,$5a,$5a,$5a,$5a,$5a,$5a,$5a
ch99:	.byte $95,$95,$95,$95,$95,$95,$aa,$2a
ch100:	.byte $65,$65,$65,$55,$55,$55,$aa,$aa
ch101:	.byte $5a,$5a,$5a,$5a,$5a,$5a,$aa,$aa

ch102:	.byte $aa,$95,$95,$95,$95,$a5,$25,$25	; 1
ch103:	.byte $25,$25,$25,$25,$25,$25,$2a,$0a

ch104:	.byte $aa,$95,$95,$95,$95,$aa,$95,$95	; 2
ch105:	.byte $aa,$55,$55,$55,$55,$a5,$55,$55
ch106:	.byte $55,$6a,$55,$55,$55,$55,$aa,$aa
ch107:	.byte $5a,$aa,$5a,$5a,$5a,$5a,$aa,$aa

ch108:	.byte $95,$aa,$95,$95,$95,$95,$aa,$2a	; 3
ch109:	.byte $55,$a5,$55,$55,$55,$55,$aa,$aa

ch110:	.byte $aa,$65,$65,$65,$65,$65,$65,$65	; 4
ch111:	.byte $95,$95,$95,$aa,$2a,$00,$00,$00
ch112:	.byte $55,$55,$55,$a5,$a5,$25,$2a,$0a

ch113:	.byte $aa,$55,$55,$55,$55,$6a,$55,$55	; 5
ch114:	.byte $a8,$5a,$5a,$5a,$5a,$aa,$5a,$5a

ch115:	.byte $55,$65,$55,$55,$55,$55,$aa,$aa	; 6

ch116:	.byte $aa,$2a,$00,$00,$00,$00,$00,$00	; 7
ch117:	.byte $a5,$a5,$25,$25,$25,$25,$2a,$0a

ch118:	.byte $aa,$55,$55,$55,$55,$65,$55,$55	; 8

ch119:	.byte $00,$0a,$25,$25,$95,$95,$95,$95	; Scoreboard
ch120:	.byte $aa,$55,$55,$55,$55,$55,$55,$55
ch121:	.byte $00,$a0,$58,$58,$56,$56,$56,$56
ch122:	.byte $95,$95,$95,$95,$95,$95,$95,$95
ch123:	.byte $55,$55,$55,$55,$55,$55,$55,$55
ch124:	.byte $56,$56,$56,$56,$56,$56,$56,$56
ch125:	.byte $95,$95,$95,$95,$25,$25,$0a,$00
ch126:	.byte $55,$55,$55,$55,$55,$55,$55,$aa
ch127:	.byte $56,$56,$56,$56,$58,$58,$a0,$00

digit1:	.byte 16,17,18,48,49,50	; Position digits on scoreboard
digit2:	.byte 13,14,15,45,46,47
digit3:	.byte 10,11,12,42,43,44

num0:	.byte 96+$80,97+$80,98+$80,99+$80,100+$80,101+$80
num1:	.byte 102+$80,98+$80,13+$80,103+$80,101+$80,13+$80
num2:	.byte 104+$80,105+$80,98+$80,99+$80,106+$80,107+$80
num3:	.byte 104+$80,105+$80,98+$80,108+$80,109+$80,101+$80
num4:	.byte 96+$80,110+$80,98+$80,111+$80,112+$80,101+$80
num5:	.byte 96+$80,113+$80,114+$80,108+$80,109+$80,101+$80
num6:	.byte 96+$80,113+$80,114+$80,99+$80,115+$80,101+$80
num7:	.byte 96+$80,97+$80,98+$80,116+$80,117+$80,101+$80
num8:	.byte 96+$80,118+$80,98+$80,99+$80,115+$80,101+$80
num9:	.byte 96+$80,118+$80,98+$80,108+$80,109+$80,101+$80

num_lb:	.byte <num0,<num1,<num2,<num3,<num4,<num5,<num6,<num7,<num8,<num9
num_hb:	.byte >num0,>num1,>num2,>num3,>num4,>num5,>num6,>num7,>num8,>num9

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Flappy bird sprite flying
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

fch96:	.byte $00,$00,$02,$2a,$95,$95,$95,$25	; Flappy up
	.byte $0a,$a5,$55,$95,$65,$59,$59,$65
	.byte $aa,$65,$95,$95,$95,$65,$5a,$6f
	.byte $00,$80,$60,$98,$98,$58,$aa,$ff
	.byte $0a,$09,$02,$00,$00,$00,$00,$00
	.byte $95,$55,$95,$2a,$00,$00,$00,$00
	.byte $ba,$6f,$5a,$a0,$00,$00,$00,$00
	.byte $aa,$fe,$a8,$00,$00,$00,$00,$00

fch104:	.byte $00,$00,$02,$09,$25,$2a,$95,$95	; Flappy middle
	.byte $0a,$a5,$55,$55,$55,$a5,$59,$59
	.byte $aa,$65,$95,$95,$95,$65,$5a,$6f
	.byte $00,$80,$60,$98,$98,$58,$aa,$ff
	.byte $2a,$09,$02,$00,$00,$00,$00,$00
	.byte $a5,$55,$95,$2a,$00,$00,$00,$00
	.byte $ba,$6f,$5a,$a0,$00,$00,$00,$00
	.byte $aa,$fe,$a8,$00,$00,$00,$00,$00

fch112:	.byte $00,$00,$02,$09,$25,$25,$2a,$95	; Flappy down
	.byte $0a,$a5,$55,$55,$55,$55,$a5,$59
	.byte $aa,$65,$95,$95,$95,$65,$5a,$6f
	.byte $00,$80,$60,$98,$98,$58,$aa,$ff
	.byte $95,$95,$2a,$00,$00,$00,$00,$00
	.byte $65,$95,$95,$2a,$00,$00,$00,$00
	.byte $ba,$6f,$5a,$a0,$00,$00,$00,$00
	.byte $aa,$fe,$a8,$00,$00,$00,$00,$00

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Flappy bird sprite death
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

fldea0:	.byte $00,$00,$00,$02,$02,$09,$09,$09	; Flappy death sprite
	.byte $0a,$25,$a5,$65,$65,$65,$5a,$aa
	.byte $00,$a0,$98,$96,$95,$95,$55,$55
	.byte $00,$00,$00,$00,$80,$80,$60,$60
	.byte $09,$09,$02,$02,$02,$02,$02,$00
	.byte $65,$b9,$ee,$ee,$ee,$ee,$ee,$ae
	.byte $6a,$95,$55,$55,$69,$56,$a8,$00
	.byte $60,$a0,$60,$60,$80,$00,$00,$00

flappy_ani_lb:
	.byte <fch96,<fch104,<fch112,<fch104,<fldea0
flappy_ani_hb:
	.byte >fch96,>fch104,>fch112,>fch104,>fldea0

;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
; Flappy bird sprite mask
;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

fma96:	.byte $ff,$ff,$fc,$c0,$00,$00,$00,$c0	; Mask Flappy up
	.byte $f0,$00,$00,$00,$00,$00,$00,$00
	.byte $00,$00,$00,$00,$00,$00,$00,$00
	.byte $ff,$3f,$0f,$03,$03,$03,$00,$00
	.byte $f0,$f0,$fc,$ff,$ff,$ff,$ff,$ff
	.byte $00,$00,$00,$c0,$ff,$ff,$ff,$ff
	.byte $00,$00,$00,$0f,$ff,$ff,$ff,$ff
	.byte $00,$00,$03,$ff,$ff,$ff,$ff,$ff

fma104:	.byte $ff,$ff,$fc,$f0,$c0,$c0,$00,$00	; Mask Flappy middle
	.byte $f0,$00,$00,$00,$00,$00,$00,$00
	.byte $00,$00,$00,$00,$00,$00,$00,$00
	.byte $ff,$3f,$0f,$03,$03,$03,$00,$00
	.byte $c0,$f0,$fc,$ff,$ff,$ff,$ff,$ff
	.byte $00,$00,$00,$c0,$ff,$ff,$ff,$ff
	.byte $00,$00,$00,$0f,$ff,$ff,$ff,$ff
	.byte $00,$00,$03,$ff,$ff,$ff,$ff,$ff

fma112:	.byte $ff,$ff,$fc,$f0,$c0,$c0,$c0,$00	; Mask Flappy down
	.byte $f0,$00,$00,$00,$00,$00,$00,$00
	.byte $00,$00,$00,$00,$00,$00,$00,$00
	.byte $ff,$3f,$0f,$03,$03,$03,$00,$00
	.byte $00,$00,$c0,$ff,$ff,$ff,$ff,$ff
	.byte $00,$00,$00,$c0,$ff,$ff,$ff,$ff
	.byte $00,$00,$00,$0f,$ff,$ff,$ff,$ff
	.byte $00,$00,$03,$ff,$ff,$ff,$ff,$ff

fdmsk0:	.byte $ff,$ff,$ff,$fc,$fc,$f0,$f0,$f0	; Mask Flappy death
	.byte $f0,$c0,$00,$00,$00,$00,$00,$00
	.byte $ff,$0f,$03,$00,$00,$00,$00,$00
	.byte $ff,$ff,$ff,$ff,$3f,$3f,$0f,$0f
	.byte $f0,$f0,$fc,$fc,$fc,$fc,$fc,$ff
	.byte $00,$00,$00,$00,$00,$00,$00,$00
	.byte $00,$00,$00,$00,$00,$00,$03,$ff
	.byte $0f,$0f,$0f,$0f,$3f,$ff,$ff,$ff

flappy_msk_lb:
	.byte <fma96,<fma104,<fma112,<fma104,<fdmsk0
flappy_msk_hb:
	.byte >fma96,>fma104,>fma112,>fma104,>fdmsk0

;--------------------------------------------------------------------------
; Character quick address lookup table
;
; - To speed up calculation of character data address, get lb,hb from table
;--------------------------------------------------------------------------

ch_lb:
	.byte <ch0,<ch1,<ch2,<ch3,<ch4,<ch5,<ch6,<ch7
	.byte <ch8,<ch9,<ch10,<ch11,<ch12,<ch13,<ch14,<ch15
	.byte <ch16,<ch17,<ch18,<ch19,<ch20,<ch21,<ch22,<ch23
	.byte <ch24,<ch25,<ch26,<ch27,<ch28,<ch29,<ch30,<ch31
	.byte <ch32,<ch33,<ch34,<ch35,<ch36,<ch37,<ch38,<ch39
	.byte <ch40,<ch41,<ch42,<ch43,<ch44,<ch45,<ch46,<ch47
	.byte <ch48,<ch49,<ch50,<ch51,<ch52,<ch53,<ch54,<ch55
	.byte <ch56,<ch57,<ch58,<ch59,<ch60,<ch61,<ch62,<ch63
	.byte <ch64,<ch65,<ch66,<ch67,<ch68,<ch69,<ch70,<ch71
	.byte <ch72,<ch73,<ch74,<ch75,<ch76,<ch77,<ch78,<ch79
	.byte <ch80,<ch81,<ch82,<ch83,<ch84,<ch85,<ch86,<ch87
	.byte <ch88,<ch89,<ch90,<ch91,<ch92,<ch93,<ch94,<ch95
	.byte <ch96,<ch97,<ch98,<ch99,<ch100,<ch101,<ch102,<ch103
	.byte <ch104,<ch105,<ch106,<ch107,<ch108,<ch109,<ch110,<ch111
	.byte <ch112,<ch113,<ch114,<ch115,<ch116,<ch117,<ch118,<ch119
	.byte <ch120,<ch121,<ch122,<ch123,<ch124,<ch125,<ch126,<ch127
ch_hb:
	.byte >ch0,>ch1,>ch2,>ch3,>ch4,>ch5,>ch6,>ch7
	.byte >ch8,>ch9,>ch10,>ch11,>ch12,>ch13,>ch14,>ch15
	.byte >ch16,>ch17,>ch18,>ch19,>ch20,>ch21,>ch22,>ch23
	.byte >ch24,>ch25,>ch26,>ch27,>ch28,>ch29,>ch30,>ch31
	.byte >ch32,>ch33,>ch34,>ch35,>ch36,>ch37,>ch38,>ch39
	.byte >ch40,>ch41,>ch42,>ch43,>ch44,>ch45,>ch46,>ch47
	.byte >ch48,>ch49,>ch50,>ch51,>ch52,>ch53,>ch54,>ch55
	.byte >ch56,>ch57,>ch58,>ch59,>ch60,>ch61,>ch62,>ch63
	.byte >ch64,>ch65,>ch66,>ch67,>ch68,>ch69,>ch70,>ch71
	.byte >ch72,>ch73,>ch74,>ch75,>ch76,>ch77,>ch78,>ch79
	.byte >ch80,>ch81,>ch82,>ch83,>ch84,>ch85,>ch86,>ch87
	.byte >ch88,>ch89,>ch90,>ch91,>ch92,>ch93,>ch94,>ch95
	.byte >ch96,>ch97,>ch98,>ch99,>ch100,>ch101,>ch102,>ch103
	.byte >ch104,>ch105,>ch106,>ch107,>ch108,>ch109,>ch110,>ch111
	.byte >ch112,>ch113,>ch114,>ch115,>ch116,>ch117,>ch118,>ch119
	.byte >ch120,>ch121,>ch122,>ch123,>ch124,>ch125,>ch126,>ch127

;--------------------------------------------------------------------------
; Screen background without bird and pipes
;
; - Initial background data
;--------------------------------------------------------------------------

scr_base:
	.byte 13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13		; Line0
	.byte 13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13

	.byte 13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13		; Line1
	.byte 13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13

	.byte 13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13		; Line2
	.byte 13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13

	.byte 13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13		; Line3
	.byte 13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13

	.byte 13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13
	.byte 13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13

	.byte 13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13
	.byte 13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13

	.byte 13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13		; Line6
	.byte 13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13

	.byte 8,9,10,11,12,13,14,15,8,9,10,11,12,13,14,15		; Line7
	.byte 8,9,10,11,12,13,14,15,8,9,10,11,12,13,14,15

	.byte 16,17,18,19,20,21,22,23,16,17,18,19,20,21,22,23		; Line8
	.byte 16,17,18,19,20,21,22,23,16,17,18,19,20,21,22,23

	.byte 24,25,26,27,28,29,30,31,24,25,26,27,28,29,30,31		; Line9
	.byte 24,25,26,27,28,29,30,31,24,25,26,27,28,29,30,31

	.byte 32,33,34,35,36,37,38,39,32,33,34,35,36,37,38,39		; Line10
	.byte 32,33,34,35,36,37,38,39,32,33,34,35,36,37,38,39

	.byte 0,4,0,4,0,4,0,4,0,4,0,4,0,4,0,4				; Line11
	.byte 0,4,0,4,0,4,0,4,0,4,0,4,0,4,0,4

;--------------------------------------------------------------------------
; Array storage, current screen and update screen
;
; - Current screen contains chars on screen
; - Update screen contains changed chars on screen, if no change value=$ff
;--------------------------------------------------------------------------

scr_current:
	.repeat 32*12
		.byte 0
	.endrep

;--------------------------------------------------------------------------
; Animation table
;
; - Every character jumps to the according routine to execute animation
;--------------------------------------------------------------------------

ani_table_lb:

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Bottombar chars
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	.byte <ani0,<ani0,<ani0,<ani0,<ani0,<ani0,<ani0,<ani0			;   0-  7

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Background chars
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	.byte <no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani	;   8- 15
	.byte <no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani   ;  16- 23
	.byte <no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani	;  24- 31
	.byte <no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani	;  32- 39

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Pipe top chars
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	.byte <ani1,<ani1,<ani1,<ani2,<ani1,<ani1,<ani1,<ani2			;  40- 47
	.byte <ani1,<ani1,<ani1,<ani2,<ani1,<ani1,<ani1,<ani2			;  48- 55
	.byte <ani1,<ani1,<ani1,<ani2,<ani1,<ani1,<ani1,<ani2			;  56- 63
	.byte <ani1,<ani1,<ani1,<ani3						;  64- 67

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Pipe body chars
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	.byte <ani1,<ani1,<ani1,<ani2						;  68- 71 
	.byte <ani1,<ani1,<ani1,<ani2,<ani1,<ani1,<ani1,<ani2			;  72- 79
	.byte <ani1,<ani1,<ani1,<ani2,<ani1,<ani1,<ani1,<ani2			;  80- 87
	.byte <ani1,<ani1,<ani1,<ani2,<ani1,<ani1,<ani1,<ani3			;  88- 95

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Score digits
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	.byte <no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani	;  96-103
	.byte <no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani	; 104-111
	.byte <no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani	; 112-119
	.byte <no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani,<no_ani	; 120-127

ani_table_hb:

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Bottombar chars
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	.byte >ani0,>ani0,>ani0,>ani0,>ani0,>ani0,>ani0,>ani0			;   0-  7

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Pipe top chars
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	.byte >no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani   ;  16- 23
	.byte >no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani	;  24- 31
	.byte >no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani	;  32- 39
	.byte >no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Pipe top chars
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	.byte >ani1,>ani1,>ani1,>ani2,>ani1,>ani1,>ani1,>ani2			;  40- 47
	.byte >ani1,>ani1,>ani1,>ani2,>ani1,>ani1,>ani1,>ani2			;  48- 55
	.byte >ani1,>ani1,>ani1,>ani2,>ani1,>ani1,>ani1,>ani2			;  56- 63
	.byte >ani1,>ani1,>ani1,>ani3						;  64- 67

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Pipe body chars
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	.byte >ani1,>ani1,>ani1,>ani2						;  68- 71
	.byte >ani1,>ani1,>ani1,>ani2,>ani1,>ani1,>ani1,>ani2			;  72- 79
	.byte >ani1,>ani1,>ani1,>ani2,>ani1,>ani1,>ani1,>ani2			;  80- 87
	.byte >ani1,>ani1,>ani1,>ani2,>ani1,>ani1,>ani1,>ani3			;  88- 95

	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.
	; Score digits
	;-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

	.byte >no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani	;  96-103
	.byte >no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani	; 104-111
	.byte >no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani	; 112-119
	.byte >no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani,>no_ani	; 120-127

