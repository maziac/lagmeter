;===========================================================================
; main.asm
;===========================================================================

    DEVICE ZXSPECTRUM48

; Interface II joystick ports.
PORT_IF2_JOY_0: equ 0xEFFE ; Keys: 6, 7, 8, 9, 0
;PORT_IF2_JOY_1: equ 0xF7FE ; Keys: 5, 4, 3, 2, 1


; Port to set the border.
PORT_BORDER:    equ 0x00FE


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
    
main_loop:
    ld hl,last_keys
    ld bc,PORT_IF2_JOY_0

check_keyboard:
    ; Check keyboard 6-0/Interface II joystick
    in a,(c) ; 0=pressed
    xor 0FFh ; 1=pressed
    ;  Clear unused bits
    and 00011111b
    ; Compare with previous keys
    ld e,a  ; save value
    xor (hl)
    jr z,check_keyboard  ; Jump if no change

    ; Store
    ld (hl),e

    ; Check if key '9' has changed and was pressed (to change the color)
    bit 1,a
    jr z,no_color_change

    bit 1,e
    jr z,no_color_change

    ; increment color
    ld hl,last_color
    inc (hl)
    ld a,00000111b
    and (hl)
    ld (hl),a
    jr nz,no_col_inc
    ; Avoid BLACK
    inc (hl)
no_col_inc:
    call set_backg_paper_color
    jr main_loop
no_color_change:

    ; Check if key '0' has changed
    bit 0,a
    jr z,main_loop   ; Jump if not changed

    ; Check if key '0' pressed or released
    bit 0,e

    jr z,black  ; Jump if no key pressed

    ; Some key pressed:
    ; Set background color
    call set_backg_paper_color
    jr main_loop

black:
    ; Clear background
    call clear_backg
    jr main_loop


; Sets the background paper color.
set_backg_paper_color:
    ; load color
    ld a,(last_color)
    ; Convert color to paper color
    rlca
    rlca
    rlca 
    ;call set_backg
    ; Set only small part
    ;ld bc,2*32  ; 2 lines
    ld bc,COLOR_SCREEN_SIZE
    ld hl,COLOR_SCREEN
    call fill_memory
    ret	


; Used to store the last keypress.
last_keys: defb 0FFh

; Used to store the last used color.
last_color: defb WHITE


;===========================================================================
; Stack. 
;===========================================================================

; Stack: this area is reserved for the stack
STACK_SIZE: equ 10    ; in words


; Reserve stack space
stack_bottom:
    defs    STACK_SIZE*2, 0
stack_top:  defb 0  ; WPMEM




; Fill up to 65535
    defs 0x10000 - $


    SAVESNA "zxlagtest.sna", main
    