;===========================================================================
; main.asm
;===========================================================================

    DEVICE ZXSPECTRUM48

; Interface II joystick ports.
PORT_IF2_JOY_0: equ 0xEFFE ; Keys: 6, 7, 8, 9, 0
;PORT_IF2_JOY_1: equ 0xF7FE ; Keys: 5, 4, 3, 2, 1



    ORG 0x4000
    defs 0x6000 - $    ; move after screen area
screen_top: defb    0   ; WPMEM
    

;===========================================================================
; Persistent watchpoint.
; Change WPMEMx (remove the 'x' from WPMEMx) below to activate.
; If you do so the program will hit a breakpoint when it tries to
; write to the first byte of the 3rd line.
; When program breaks in the fill_memory sub routine please hover over hl
; to see that it contains 0x5804 or COLOR_SCREEN+64.
;===========================================================================

; WPMEMx 0x5840, 1, w


;===========================================================================
; Include modules
;===========================================================================
    include "utilities.asm"
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

    ; Init
lbl1:
    ld hl,fill_colors
    ld (fill_colors_ptr),hl
    ld de,COLOR_SCREEN
    
    
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
    cp (hl)
    jr z,check_keyboard

    ; Store
    ld (hl),a
    or a    ; Check if key pressed or released

    ld a,BLACK  ; paper color = black
    jr z,no_press  ; Jump if no key pressed
    ; Some key pressed
    ld a,(WHITE<<3)+BRIGHT ; Paper color = white
no_press:
    call set_backg
    jr main_loop


; Used to store thelast keypress.
last_keys: defb 0FFh


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
    