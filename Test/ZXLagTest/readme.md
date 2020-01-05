# ZXSpectrum Screen Test Program

Simple program that changes the whole screen according to button/joystick presses.
When pressed the screen turns white. If no key is pressed the screen is black.

The program can be used to measure the lag between button press until reaction on the screen when run inside an emulator.

In the MAME emulator it takes 1 frame until the main screen changes, 2 frames until the border changes.
(measured by frame-stepping, SHIFT-P.)
Seems ot be emulator specific because in the program the border is changed earlier.

Program can be started in MAME/MESS with:
~~~
mess64 spec128 -dump zxlagtest.sna -rompath .../zspectrum/roms -video opengl -ui_active -waitvsync
~~~
