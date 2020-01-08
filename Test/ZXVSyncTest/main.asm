;===========================================================================
; main.asm
;===========================================================================

    DEVICE ZXSPECTRUM48

; Port to set the border.
PORT_BORDER:    equ 0x00FE


; The VSync frequency
VSYNC_FREQ:     equ 50

; ROM routines
ROM_PRINT_CHAR: equ 6683
ROM_PRINT_STRING:   equ 8252



; Set to one to show a bar (COLOR Screen)
SHOW_VERT_BAR:   equ 0

; Set to one to show a thin vertical line
SHOW_VERT_LINE: equ 1

; The increment for the line x-position (at least 2)
LINE_X_INC:     equ 2


    ORG 0x8000
 


;===========================================================================
; Include modules
;===========================================================================

    include "fill.asm"
    include "clearscreen.asm"


;===========================================================================
; main routine - the code execution starts here.
; Sets up the new interrupt routine, the memory
; banks and jumps to the start loop.
;===========================================================================
main:
    ; Disable interrupts
    di
 
    ; Setup stack
    ld sp,stack_top
 
    ; CLS
    call clear_screen
    ld a,WHITE
    call set_backg
    ld bc,PORT_BORDER
    ld a,BLACK
    out (c),a
    
    ; Print to upper screen
    ld a,2
    call 5633

    ; Set INK and PAPER for output
    ld de,black_on_white
    ld bc,black_on_white.end-black_on_white
    call ROM_PRINT_STRING 

  IF SHOW_VERT_LINE
    ; Remove the line
    ld a,(line_position)
    call show_line_xor
  ENDIF
 
    ; Enable VSync Interrupt
    ei

main_loop:
    ; Wait on VSync
    halt

  IF SHOW_VERT_LINE
    ; Remove the line
    ld a,(line_position)
    call show_line_xor

    ; Move the line
    ld a,(line_position)
    add a,LINE_X_INC
    ld (line_position),a

    ; Show line
    call show_line_xor
  ENDIF


  IF SHOW_VERT_BAR
    ; Remove the bar
    ld a,(bar_position)
    ld l,a
    ld a,WHITE
    call show_bar

    ; Move the bar
    ld a,(bar_position)
    inc a
    and 00011111b
    ld (bar_position),a

    ; Show bar
    ld l,a
    ld a,WHITE<<3
    call show_bar
  ENDIF


    ; Increase counter
    ld hl,frame_counter
    dec (hl)
    jr nz,.no_sec

    ld (hl),VSYNC_FREQ
    ; Increase seconds
    ld hl,secs_counter
    ld a,(hl)
    inc a
    ld (hl),a
    cp 60   ; Compare with a minute
    jr nz,.no_min

    ; Minute over
    ld (hl),0

.no_min:
    ; Show time
    call print_time

.no_sec:
    jr main_loop


; Shows a vertical bar.
; a = color (paper color)
; l = column [0;31]
show_bar:
    ld h,high COLOR_SCREEN
    ld b,24
    ld de,32
.loop:
    ld (hl),a
    add hl,de
    djnz .loop
    ret


; Shows a vertical line.
; a = color (paper color)
; a = column [0;31]
show_line_xor:
    ; Calculate bit
    ld l,a
    and 07h
    ld b,a
    inc b
    ld a,01b
.rotate:
    rrca
    djnz .rotate
    ld c,a

    ; get byte fro, x postion
    srl l : srl l : srl l

    ld h,high SCREEN
    ld b,192    ; height
    ld de,32    ; 1 line
.loop:
    ld a,(hl)
    xor c
    ld (hl),a
    add hl,de
    djnz .loop
    ret


; Prints the time in the upper left corner.
print_time:
    ; Print AT 0,0    
    ld de,at00
    ld bc,at00.end-at00
    call ROM_PRINT_STRING
    ; Show the time
    ld hl,secs_counter
    ld b,0
    ld c,(hl)
    call ROM_PRINT_CHAR   ; Display the number
    ; Print a space
    ld a,' '
    rst 16
    ret

; The column the bar is shown.
bar_position:   defb 0

; The x-position the line is shown.
line_position:   defb 0

; Counts each frame down from 60
frame_counter:  defb 1

; Counts each second up to 60
secs_counter:   defb 57     ; Starts a few secs before the first turnaround

; Print AT 0,0
at00:   defb 22, 0, 0
.end:

; Print black on white
black_on_white: defb 16, WHITE,  17, BLACK
.end:


;===========================================================================
; Stack. 
;===========================================================================

; Stack: this area is reserved for the stack
STACK_SIZE: equ 100    ; in words


; Reserve stack space
stack_bottom:
    defw    0   ; WPMEM, 2
    defs    STACK_SIZE*2, 0
stack_top:  
    defw 0  ; WPMEM, 2




; Fill up to 65535
    defs 0x10000 - $


    SAVESNA "zxvsynctest.sna", main
    