;===========================================================================
; fill.asm
; Submodule with memory fill routines.
;===========================================================================

; Some constants
BCKG_LINE_SIZE:  equ     32

; Colors 
BLACK:          equ 0
BLUE:           equ 1
RED:            equ 2
MAGENTA:        equ 3
GREEN:          equ 4
CYAN:           equ 5
YELLOW:         equ 6
WHITE:          equ 7

BRIGHT:         equ 01000000b


; Fills a memory area with a certain value.
; a = contains the fill value.
; hl = address to fill
; bc = size
fill_memory:
    ld (hl),a
    ld e,l
    ld d,h
    inc de
    dec bc
    ldir
    ret	

 

