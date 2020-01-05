# ZXSpectrum VSync Test Program

A test program that allows to do tests around the Vertical Synching.

Tests are:
- Check for 50 or 60Hz sync
- Tearing

The program waits for the vertical sync and counts them.
Every 50th sync a number is increased and displayed.
One can check the time it requires to reach 60 seconds.
If it takes a minute the VSync happens at 50Hz. If it takes less than a minute, i.e. 50 secs, then the vsync is at 60Hz.

To test the tearing a vertical bar is moved from left to right. The bar is moved in sync with the VSync. 
I.e. if the monitor's is not in sync with the emulated vsync you will see [tearing](https://en.wikipedia.org/wiki/Screen_tearing) (the upper part of the bar already advanced to the right whereas the lower part is still one frame behind).

Both test run at the same time.

Program can be started in MAME/MESS with:
~~~
mess64 spec128 -dump zxvsynctest.sna -rompath .../zspectrum/roms -video opengl -ui_active
~~~

