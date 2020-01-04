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
    call clear_backg
    ld bc,PORT_BORDER
    ld a,BLACK
    out (c),a
    
    ; Print toe upper screen
    ld a,2
    call 5633

    ; Set INK and PAPER for output
    ld de,black_on_white
    ld bc,black_on_white.end-black_on_white
    call ROM_PRINT_STRING 

    ; Enable VSync Interrupt
    ei

main_loop:
    ; Wait on VSync
    halt

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
    