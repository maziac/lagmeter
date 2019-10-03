# LagMeter

**Note: This is still work in progress. I.e. not finished. Conclusions/SW/HW is not ready for public usage. Use only on your own risk.**




The goal of this project is to build a HW/SW that allows for easy measurement of input (output) lag.

The tested setup in my case is a self-made aracde cabinet which is based on a debian/Linux and an Intel NUC HW. But the LagMeter can also be used for other PC-like or even console-like setups.

The LagMeter will stimulate a button on a controller and measure the time it takes until a reaction occurs on the monitor.
Apart from the LagMeter itself some SW is required on the tested system that changes the color or brightness of a (small) area on the screen depending on the state of the input device (the button press).
For Linux the jstest-gtk program is enough, for Windows the "Windows USB Controller Setup" program does the job.
If it is required to measure the lag including some emulator lag it is necessary to run a program on the emulator that changes the brightness of the screen depending on the input.


## Disclaimer

This is mainly a HW project and with HW projects, although the risk should be small, it is always possible that you damage your HW or HW that you are going to measure.
You should only try to do this yourself if you already have experience with electronics and baiscally know what you are doing.

IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THE HW, SCHEMATICS, SW OR ANYTHING ELSE PRESENTED HERE.




# Lag

## Theory

There are a lot of entities that sum up to the total lag.
Here are the most important ones:
- **Controller lag**: The controller is normally a USB device. It contains a CPU to measure the button presses and then transmits the status via USB. (Wireless controllers add an additional lag of about 5ms.)
- **USB lag**: This is caused by the USB polling rate. An USB device cannot simply send data to the host when e.g. a button is pressed. Instead it has to wait until it is polled from the host to send the button status. The USB polling rate is usually (default) 10ms and can be reduced down to 1ms.
- **OS lag**: The time the operating system requires to process the USB data and pass it to the user land, i.e. the application.
- **Application lag**: The time the application (e.g. jstest-gtk or an emulator) requires to read the input and react on it. This is in the range of a few frames, e.g. n*20ms. Triple buffering or enabling of graphics effects will increase the lag.
- **Video encoding lag**: Creating the video signal will also introduce a little lag. I'm not sure, mabye this is neglectible, but in theory it is there and can also vary for the used encoding, i.e. SVGA vs HDMI.
- **Monitor lag** (response time): The monitor needs some time from input of the signal until it is displayed. This typically is in the range of a few ms up to some frames (n*20ms). It is important to disable any video processing inside the monitor as this adds extra delay. This is normally done by putting the monitor in a special 'game mode' via its settings. The monitor lag might also vary depending on the used video signal. I.e. the lag for SVGA and HDMI might be different even for the same monitor.

It would be beneficial if we could measure each lag individually but this is normally not possible.
Only the sum of lags can be measured and conclusions might be drawn that may or may not be correct :-)


## Influence of Intervals (Polling)

Understanding the impact of polling intervals is essential when dealing with input lag.
When the joystick button is pressed it is not immediately send to the program that evaluates it. Instead the button value is polled at certain intervals.
Depending on the time when the button is pressed relative to the poll a different time is required until the button press information arrives at its destination.
This is the most common source of introduced lag. And the bad thing about it is that it is not constant: the lag time varies at exactly the polling interval time.
Therefore it is important to use short (or better no) polling wherever possible.
It simply increase the variance in the lag.

Unfortunately there is not only one component that introduces lag via polling.
There are at least:
- The USB controller (normally it is not possible to obtain the sources for the firmware of a controller so a particular controller might could work differently but in general it is safe to assume that the buttons etc. are polled at a certain frequency.)
- The USB polling inside the OS. This is often set to 8ms (125Hz) but can be reduced to a minimum of 1ms.
- The emulator polling the joystick: This is done at frame rate, i.e. 20ms (50Hz) for Europe and 16.7ms (60Hz) for US.
- The monitor signal HW: This as well uses the frame rate, i.e. 20 or 16.7ms, but might not be in sync (vsync) with the emulator.
Note: Of course you could increase the frame rate to lower the interval/lower the latency. The problem is that if you use another frame rate than the source (i.e. the emulator) you may encounter other visual artefacts.

The picture below tries to visualize the effect of the polling:
~~~wavedrom
{signal: [
  
  {name: "Relais",
   wave: 'l....H.................',
   node: '.....a'},
  
  {name: "Game Controller",
   wave: '5..35..35..35..35..35..',
   data: ["", "poll", "wait", "poll", "wait", "poll", "wait", "poll", "wait", "poll", "wait",],
   node: '.......cC..dD..eF'},
  
  {name: "USB",
   wave: '4.34.....34.....34.....',
   data: ["", "poll", "t=8ms", "poll", "t=8ms", "poll", "t=8ms", "poll",],
   node: '.........gG.....hH.....'},
  
  {name: "OS",
   wave: 'z..5z.....5..z...5.z...',
   data: ["delay", "delay", "delay",],
   node: '..........i..I...j.J'},
  
  {name: "Application/Emulator",
   wave: '434..................34',
   data: ["", "poll", "frame n, t=20ms", "poll", "n+1", "poll",],
   node: '.....................kK'},
  ],

edge:[
      'a~>c',
      'a~>d',
      'a~>e',
      
      'C~>g',
      'D~>h',

      'G~>i',
      'H~>j',

      'J~>k',
      'K~>l',
      'l~>m'
 ],

}
~~~
Relais is the simulated button press. When it's pressed the game controller notices after a few ms depending of the firmware.
This value is not directly provided to the OS. Instead the USB host controller (of the PC) polls at 8ms (default, 125Hz).
The OS receives the value and provides it to the application after some delay. This delay should be short but is not necessarily fixed, i.e. depending on the current load of the CPU it might vary.
Then the application, i.e. the emulator, reads the button value. It polls usually at the frame rate, i.e. 20ms for Europe.

In best case all pollings happen just right after each other. In that case only a small delay, the delay of the OS, would be measurable.
In worst case all polling intervals have to be added. I.e. Game controller interval (e.g. 10ms), USB interval (8ms), OS delay (5ms?), Emulator frame rate (20ms) = 10+8+5+20 = 43ms.

I.e. already with these small value we end up with a jitter of 5ms to 43ms for a button press.
But that is just the path to the emulator game controller input.

The full path needs to draw and display something on the monitor:

~~~wavedrom
{signal: [
  
  {name: "Application/Emulator",
   wave: '4...34.................',
   data: ["frame n+1, t=20ms", "", "frame n+2, t=20ms", "",],
   node: '....K'},

  {name: "Screen (Out)",
   wave: '5......5...............',
   data: ["frame n+1", "frame n+2, t=20ms", "frame n+3",],
   node: '.......l'},
  
  {name: "Monitor",
   wave: '5......5.3.............',
   data: ["", "delay", "frame n, t=20ms", "delay", "frame n+1, t=20ms",],
   node: '.........m'},

  {name: "Visible Response",
   wave: '0........1.............',
   data: ["", "delay", "frame n, t=20ms", "delay", "frame n+1, t=20ms",]},
],

edge:[
      'a~>c',
      'a~>d',
      'a~>e',
      
      'C~>g',
      'D~>h',

      'G~>i',
      'H~>j',

      'J~>k',
      'K~>l',
      'l~>m'
 ],
}
~~~
The emulator needs at least one frame to calculate the next frame depending on the game controller's input (button press).
(This is quite optimistic, often, depending on the emulated game, the emulator needs more frames. Then depending on the emulator's settings it might need additional frames, e.g. for triple buffering or for post processing of the screen output.)

The rendered frame needs to be go out of PC. This is also done at frame rate and will add an additional delay.
If the emulator's output is synched (VSYNC) with the PC screen output this delay can be reduced to 0.

At the end the monitor will add some delay. This depends on the monitor. Modern monitors should be in range 1-4ms, older monitors could around 20ms.
Sometimes monitors (especially TVs) may add post processing. This can increase the delay dramatically and should be turned off.

For a synched emulator, a one frame emulator delay and a monitor with 4ms delay we will end up at: 20+0+4 = 24ms.


With the calculation above we end up with a theoretical lag for these explanatory values of
[5ms;43ms] + 24ms = [29ms;71ms], i.e. **29ms to 71ms**.


## Interpret Measurement Results with Polling

When measuring delays and polling is involved the measured times vary between a minimum and a maximum value.
If the count of measurements is large enough the minimum measured time is the time as if there weren't any influence of the polling. 
Whereas the maximum value adds up all polling intervals.
I.e. max-min shows us the time that is lost due to polling.
At least if nothing unforeseen happens: In an operating system like linux, windows or mac it can, of course, always happen that some process delays other process for a short while. 
So the max-min value is expected to be bigger than the sum of all unsynchronized polling times.


# Additional lag by the measuring equipment

The HW uses a relais to stimulate the button press and a phototransistor to measure the resulting change on the screen.
Both have lags.
I measured that the reed relais (Relais SIL 7271-D 5V) bounces for max 40us and has a switching delay which is smaller than 250us.
The lag of phototransistors in general is in the magnitude of us.

I measured the lag with a LED tight to the reed relais and the overall shown lag was always 1ms.
So it's safe to assume the additional lag by relais and phototransistor is negligible.



# Emulator Pause Frame Stepping vs. Dynamic Delay

The usual way to measure the lag of an emulator an an easy was is to pause the emulator,
press a button and single-step the emulator frame-by-frame until a visual response is seen on the monitor.

With this one can find out quite easily the lag of the game's logic that is tested.
In e.g. MAME this can be done by pressing 'P' (for pause) and then pressing SHIFT-P to single step each frame.

Unfortunately this is only half of the truth since the dynamic behaviour is little different.
If the emulator e.g. use triple buffering or post processing the additional delay cannot be seen by frame-stepping but the delay is there when running at normal speed.

So, even if you see that there is only one frame delay when single stepping the frames, most probably at least one more frame is used e.g. for buffering/screen processing.


# HW

## Schematics (TinkerCad)

![](HW/LagMeter_TinkerCad.jpg)

Note: You don't need to wire the LCD and buttons if you use the LCD Keypad shield. It already contains the buttons and teh correct connections.


## Components List

- Arduino Uno R3
- LCD Keypad Shield
- Reed Relais SIL 7271-D 5V
- Phototransistor BPX 38-3 OSO
- R=470k
- USB Host Shield v2.0



# SW

## Arduino SW

There are 2 main functionalities implemented:
- The "LagMeter" which stimulated a game controller button and measures the time until a visual response is seen on the monitor.
- The "UsblagLcd" which directly measures the lag of a USB game controller by stimulating a button and measuring the response through the USB protocol. This part is derived from the [usblag](https://gitlab.com/loic.petit/usblag) project. It adds an UI via an LCD display.


The device starts up in the LagMeter mode and stays there.
![](<<picture of Start-Screen "LagMeter">>)

To change to UsblagLCD mode you need to attach an USB game controller. If it is recognized the display changes:
![](<<picture of Start-Screen "UsblagLCD">>)

Note: the UsblagLCD mode cannot be left automatically, you need to press reset to get back to the LagMeter mode.


### LagMeter 

![](<<picture of Start-Screen "LagMeter">>)

The LagMeter uses 5 different buttons. Each button offers a different test:
- **"Test Button"**: Will simply output the value measured at the photo resistor. At the same time a button press/release is stimulated at a frequency of approx. 1s. This is to check that the photo resistor is working and to check the values when button is pressed and released.
- **"Total Monitor Lag"**: It starts with a short calibration phase. During calibration the button is pressed for a second and the monitor output, i.e. the photo transistor value is read. 
Then the button is released and the photo transistor value is read again.
Afterwards 100 measurement cycles are done with button presses and releases. For each button press the time is measured until an action occurred on the screen.
At the end the minimum, maximum and average time is shown.
If a measurement takes too long (approx 4 secs) an error is shown.
You need a program that reacts on game controller button presses. E.g. jstest-gtk in Linux. The photo sensor need to be arranged just above the (small) screen area that changes when the button is pressed.
For the tests with the emulator you can use the ZX Spectrum program (sna-file) in this repository. It reads the (ZX Spectrum) keyboard and toggles the screen (e.g. black/white). 
In the emulator you need to map the game controller button to the "0" Spectrum key.
- **"Total SVGA Lag"**: Same as "Total Monitor Lag" but instead of measuring the photo transitor it monitors the SVGA output of the PC. I.e. as a result you get the lag without the monitor.
- **"Monitor Lag"**: This measures the monitor lag itself. For this you need to connect all cables: Game controller button, photo transitor (at monitor) and SVGA at the SVGA output ofthe PC (because the monitor is connected as well you need a Y-SVGA adapter to connect both at the same time).
Please note: monitor manufacturers have very sophisticated ways to measure the latency. The way used here is very simple, so the results may differ from your monitor's specification.
- **"Minimum Button Press Time/Reliability Test"**: It measures the minimumt time required to press the game controller's button so that it is reliably recognized. Because of polling intervals (see above) it can happen that a button press is not recognized at all if it is too short. This test measures the time and the number of button presses for a certain button press time. Whenever a button press doesn't lead to a visual response the minimum press time is increased andthe test starts all over again.
The test will run "forever", i.e. you can leave it running for a day to see if your system really catches all button presses. Or to put in another way: the tests shows you how long you have to press the button at a minimum so that it is reliably recognized.
To give some numbers: my measurements showed that with a micro switch the minimum achievable press time is around 40ms, but with leaf switches you could get down to e.g. 10-20ms. If this is good or bad depends on the rest of the system. In general it is nice to allow for short times but if the time gets smaller than the polling rate of your system than it might lead to unrecognized button presses.
Wit a few buttons you can change the press time:
  - +1ms
  - -1ms
  - +10ms
  - -10ms


You can interrupt all measurements by pressing any key.


### UsblagLcd

![](<<picture of Start-Screen "UsblagLCD">>)

The UsblagLcd uses 2 different buttons with different tests:
- **"Test Button"**: Will toggle between button press/release at a frequency of approx. 1s. You should see the LCD display changing when a game controller's button is pressed. 
For a simple test you can attach your game controller and press the buttons manually. You should see the LCD display changing.
Then you can open your game controller and attach the cables to a button to simulate button presses. If this works you see the LCD display changing at the toggle frequency.
- **"Game Controller Lag"**: Measures the lag of the game controller, i.e. from button press to USB response.
Conenct the button of your game controller with the cable and start the test.
It does 100 cycles and shows the minimum, maximum and average time used by the controller.
The test always uses a USB polling rate of 1ms.
Note: The original usblag project used 1000 cycles. I reduced this to 100 to get faster results.

You can interrupt all measurements by pressing any key.


# Validation

I did a few tests to validate the measured times.

## Reed Relais SIL 7271-D 5V

Measurements with oscilloscope:
- Switch bouncing is less than 40us
![](Images/button_reed_relais_press.BMP)
- Delay: 5V Out to relais switching: < 250us

Both together are << 1ms.



## Photo sensor 

Response time (cyan is the compare-digital-out, starts with button, ends when photo sensor range is met):

- Photosensor (yellow) attached to EIZO monitor:
![](Images/photo_sensor_following_comp_out_EIZO.BMP)
Time from dark to bright is around 10-15ms, this might be due to the monitor requiring some time to fully light the area.

- Photosensor (yellow) attached to BenQ monitor:
![](Images/photo_sensor_following_comp_out_BENQ.BMP)
Time from dark to bright here is also  around 10-15ms. What can be seen as well is that the overall response time for the BenQ is much shorter.

- Photosensor (yellow) attached simply to an (inverted) LED:
![](Images/photo_sensor_following_comp_out_LED.BMP)
Response is very fast, within 0.5ms.

The code to measure the lag has a check to test that the time between 2 measurements does not become bigger than 1ms. In that case it would show an error in the display.


## Direct feedback with LED

I put an LED to the relais. The LED emitted light into the photo sensor. 
This is the fastest feedback that could be measured. With the accuracy considerations above the overall lag should be max. 1ms.

Here is the result for 100 test cycles:
![](Images/direct_LED_feedback.jpg)





# System under Test - Setup

## HW

- NUC:	Intel NUC-Kit N3050 1.6GHz HD Graphics NUC5CPYH
- BenQ:	BenQ GW2760HS 68,5 cm (27 Zoll) Monitor (DVID, HDMI, 4ms Reaktionszeit, 16:9 Full HD). DVI/HDMI connected.
- EIZO:	Flachbildschirm 1600 x 1200 8-16ms - Eizo FlexScan S2100 21 Zoll HD Display Monitor
- EIZO-SVGA:	EIZO with SVGA signal connected
- EIZO-DVI:	EIZO with DVI signal connected
- Button:	Zero Delay USB Encoder, Dragon Rise Inc.

### Monitors:

I tried two different monitors.


#### BenQ GW2760HS 27" Monitor - 16:9, Full HD

See https://www.tftcentral.co.uk/reviews/benq_gw2760hs.htm

- Connectors for SVGA, DVI, HMI.
- Response time: 4ms

Settings:
- AMA: Advanced Motion Accelarator. Possible selections: OFF, HIGH, Premium. Was HIGH.
- PictureMode was 'Standard'. There is a 'Game' mode but I'm not sure if this is really about image processing, if set the image gets brighter.
It has no influence on lag. Same lag if 'Standard' or 'Game'.


#### Eizo FlexScan S2100 21" HD Display Monitor - 4:3, 1600x1200 8-16ms

- PVA screen
- Connectors for SVGA, DVI.
- Response time 8-16ms: http://www.webdatenblatt.de/cds/de/?pid=fa6b7ba81603252
    - On/Off Response Time: 16ms
    - Average Midtone Response Time: 8ms 
- This monitor is so old it has no settings that could influence the lag.


### USB Encoders (game controller)

Here are the tested devices together with their default polling rates.
- Buffalo Gampad: 10ms
- DragonRise, ZeroDelay USB Encoder: 10ms
- Ultimarc U360 Stik: 10ms
- FastestJoystick (TeensyLC): 1ms


## SW

- Linux:	Kernel 4.15.0, Ubuntu 16.04.1
- Windows:	Windows 10
- MAME:	v0.155
- Jstest	Jstest-gtk. Joystick tester v0.1.0


## Linux USB polling rates

Default polling rates:
- Ultimarc: 10ms, Spd=12
- Gamepad (2nd player): 10ms, Spd=1.5
- DragonRise: 10ms, Spd=1.5 (This is the USB controller I used for testing/Button)
- Mouse/Keyboard: 10ms, Spd=12

It was not possible in this setup to reduce polling times.
Nor for mouse nor for other devices.
Options 'mspoll' or 'jspoll' did change the
/sys/module/usbhid/parameters/mousepoll (or jspoll)
but still I got 125 Hz by testing with the [evhz](https://gitlab.com/iankelling/evhz) program.

I have tried:
- Setting options usbhid mousepoll=1 (jspoll=1) in /etc/modprobe.d/usbhid
- Setting usbhid.mousepoll=8 on the command line while booting
- Running sudo modprobe -r usbhid && sudo modprobe usbhid mousepoll=1 (or jspoll=1) from the command line

Problem exactly like [here](https://askubuntu.com/questions/624075/how-do-i-set-the-usb-polling-rate-correctly-for-my-logitech-mouse).

'evhz' results:
- Ultimarc: 90Hz
- Gamepad (2nd player): 34 Hz
- DragonRise: 115Hz


Note: when using the teensy as joystick with a requested poll interval of 1ms the Linux is using the 1ms poll interval. Also evhz is showing 1000Hz (but it's important to use the joystick coordinated, e.g. Joystick.X(analogRead(0)), otherwise evhz will not find any event.)

USB requested poll rate:
- Buffalo: 10ms
- TeensyLC: 1ms
- DragonRise: 10ms
- Ultimarc: 10ms



# Tests

# Joystick turnaround time

With FastestJoystick it could be seen that the OS response time is a 1ms.
So the test setup could be used to test also other Joysticks.

Setup:
1. The 'FastestJoystick' is conencted via USB to the NUC (linux) system. It is used tp outut a signal.
2. The USB-Joystick-under-test is connected to the NUC (linux) system.
3. The 'inout' (InputOutput) program is started
4. It will output a digital signal for each button press

With the LagMeter and the digital output connected to the SVGA input (SVGA input is just an analog input) we measure the minimum response time.

## FastestJoystick (Teensy)

Constant value of 2ms for more than a day or a million button clicks.
(1457671 button presses, 105240 secs).

Repeated delay measuremens (100 cycles): 3-5ms. 
(Quite independent of system load. Same times measured also with attract-mode and mame running at the same time in the foreground).

## Buffalo

Stopped after 10 mins:
9ms for 10min, 5000 cycles.

Delay measurement (100 cycles): 1-9 ms

Note: My buffalo controller (as many others) has a ghosting problem. 
The left button is "pressed" quite often (5x in 10mins), the right button about 1x in 10mins.



## DragonRise Inc. Generic USB joystick, Zero Delay (Yellow PCB)

Stopped after 3 mins:
22ms for 3 min, 5000 cycles.

Delay measurement (100 cycles): 2-31 ms


## Microntek USB Joystick (Green PCB)

Stopped after 4 mins:
42ms for 4 min, 1000 cycles.

Note: this might also have something to do with the polling of the USB host. With evhz I was not able to measure it at all.

Delay measurement (100 cycles): 4-47 ms


## Ultimarc Ultrastik 360

Stopped after 8 mins:
9ms for 8 min, 4000 cycles.

Delay measurement (100 cycles): 8-17 ms

Note: The Ultrastik 360 is an analog joystick. Potentially the AD conversion will be a little bit slower than just a button press.
I had no possibility to measure it, but I don't expect that there is a significant additional delay.



## Microsoft X-Box 360 pad (white)

Stopped after 3 mins:
21ms for 3 min, 1000 cycles.


## Microsoft X-Box 360 pad (black)

Stopped after 5 mins:
8ms for 5 min, 3000 cycles.

Delay measurement (100 cycles): 4-14 ms


# Intermediate Results

(Mainly notes to myself)

- Monitor delay dependend on position:
With BenQ: If the photosensor is positioned at the top the delay is 11ms (to SVGA).
If it is positioned at the bottom the delay is 20ms (toSVGA). 
But this can be explained: the SVGA already start to measure with the first while line, i.e. the top line.
With my measurements I chose a position always at the top.

- Tests with 'inout' running on NUC and FastestJoytick:
	Turnaround time. Lag from inout (Joystick button) to output (DOUT). While running ‚inout‘ program on PC (linux).	
	Measured 3-5ms.	
  This means: There is almost no overhead for the OS. Input delay is [0.2;1.2]ms. Output delay is [0;1]ms. Total [0.2;2.2]. The variation is exactly 2ms. So the OS delay is only ca. 3ms.
  This even does not change if in parallel an emulator is running or started.
  Even with 100% cpu load for both cpus (program 'stress' was used) this did not change.

- The delay of the Eizo itself is 38ms (DDVI) and 30ms (SVGA). The BenQ has 12-13ms no matter what input.
- Eizo SVGA input is faster than EIZO DVI input by 8ms. DVI = 38ms, SVGA = 30ms. The delay is very constant. But this means 1.5 frames delay which is far too big. This monitor needs to be exchanged. 
- NUC video output: There is no difference in lag for the HDMI and the SVGA output.

- The vertical screen position (not the horizontal) matters for the lag measurement. 
The more the photo sensor is positioned to the bottom the bigger the values. From middle to bottom this is approx. 7ms for the minimum.
This correlates quite good with 20ms for a full screen (half = 10, approx 7).
- BenQ is approx. 30ms faster than EIZO-DVI (minimum).
- EIZO-SVGA is approx. 8ms faster than EIZO-DVI (minimum).
=> Could be that SVGA output is 8ms faster than HDMI output from the NUC. Then, if BenQ would have an SVGA input a lag of 30ms (minimum could be achievable).
- BenQ-SVGA and BenQ-DVI are exactly the same speed. So it seems that NUC HDMI and SVGA is the same speed, but EIZO has a bigger input lag on DVI.
- windowed vs. fullscreen: There seems to be no difference. EIZO could be a few ms faster in wondowed mode. BenQ could be a few ms faster in fullscreen mode. So I guess the deviation is only by chance.
- The (mameau) CRT geometry shader in MAME add 10-14 ms to the overall lag.
- From the usblag measurements: 
    - The Buffalo game controller has only 1ms internal lag. It's the fastest I measured.
- The ZeroDelay controller has around 10 ms. (Compared total lag with that when Buffalo game controller was used.)
- With exchanging the monitor and the game controller I can get down to a response time of 45-79ms (with MAME). If I manage to get the linux usb polling time down to 1ms I will gain another 8ms so this would result in: 37-71ms. This is less than half the original response time (!)

- If I test against the inverted range the results are a few ms better: 4 ms for BenQ and 10-20ms for Eizo. So this setting is better for comparison with a camera to check the lag until something happens on the monitor.
However, this does not mean that the monitor is showning the area in full brightness yet. For this the measurement against the non-inverted range might be more correct/more meaningful.

- The EIZO seems to have a high variance also depending on the brightness: 
If full brightness is used as with the ZXLagTest program the lag times are lower.
For the BenQ w???

- SVGA results:
    - BenQ introduces a lag of 9-12ms
    - EIZO introduces a lag of 9-12ms
    
- SVGA out vs. HDMI out: The SVGA output is about 10ms slower than HDMI.

See [spreadsheet](Docs/LagMeasurements.ods).


(Goal: 70ms lag would be nice)


# References

- ["Controller Input Lag — How to measure it?" by Loïc *WydD* Petit](https://medium.com/@WydD/controller-input-lag-how-to-measure-it-1ebfd2c9d60)
- [usblag by Loïc Petit](https://gitlab.com/loic.petit/usblag)
- ["Turn your ProMicro into a USB Keyboard/Mouse" by jimblom](https://www.sparkfun.com/tutorials/337)
- ["Zero Delay USB encoders lag measurements - THEY HAVE LAG - AVOID" by oomek](http://forum.arcadecontrols.com/index.php?topic=153825.0)
Video comparing zero delay vs. Arduino encoder. zero delay is lagging.
- ["On the Latency of USB-Connected Input Devices" - by Raphael Wimmer, Andreas Schmid and Florian Bockes (Uni Regensburg)](https://epub.uni-regensburg.de/40182/1/)On_the_Latency_of_USB-Connected_Input_Devices_author_version.pdf
- ["Warum haben Emulatoren eine Eingabeverzögerung?" by MarcTV](https://marc.tv/warum-haben-emulatoren-eine-eingabeverzoegerung/)
- [Linux USB polling rate](https://wiki.archlinux.org/index.php/Mouse_polling_rate)
- [evhz](https://gitlab.com/iankelling/evhz)
- [FastestJoystick](https://github.com/maziac/fastestjoystick)


# License

The SW includes sources from [usblag](https://gitlab.com/loic.petit/usblag) by Loïc Petit which are under MIT License.

These sources contain code from the [USB Host Shield Library 2.0](https://github.com/felis/USB_Host_Shield_2.0) which are under GPL2 license.

The rest of the sources are under MIT Licence.
