
# System under Test - Setup

## HW

- NUC:	Intel NUC-Kit N3050 1.6GHz HD Graphics NUC5CPYH
- BenQ:	BenQ GW2760HS 68,5 cm (27 Zoll) Monitor (DVID, HDMI, 4ms Reaktionszeit, 16:9 Full HD). DVI/HDMI connected.
- EIZO:	Flachbildschirm 1600 x 1200 8-16ms - Eizo FlexScan S2100 21 Zoll HD Display Monitor
- EIZO-SVGA:	EIZO with SVGA signal connected
- EIZO-DVI:	EIZO with DVI signal connected
- Button:	
    - Zero Delay USB Encoder, Dragon Rise Inc.
    - Or Teensy FastestJoystick, see my other project here](http://www.github.com/FastestJoystick)

### Monitors:

I tried two different monitors.


#### BenQ GW2760HS 27" Monitor - 16:9, Full HD

See https://www.tftcentral.co.uk/reviews/benq_gw2760hs.htm

- Connectors for SVGA, DVI, HMI.
- Response time (according specs): 4ms

Settings:
- AMA: Advanced Motion Accelarator. Possible selections: OFF, HIGH, Premium. Was HIGH.
- PictureMode was 'Standard'. There is a 'Game' mode but I'm not sure if this is really about image processing, if set the image gets brighter.
It has no influence on lag. Same lag if 'Standard' or 'Game'.


#### Eizo FlexScan S2100 21" HD Display Monitor - 4:3, 1600x1200 8-16ms

- PVA screen
- Connectors for SVGA, DVI.
- Response time (according specs): 8-16ms, http://www.webdatenblatt.de/cds/de/?pid=fa6b7ba81603252
    - On/Off Response Time: 16ms
    - Average Midtone Response Time: 8ms 
- This monitor is so old it has no settings that could influence the lag.


### USB Encoders (game controller)

Here are the tested devices together with their default polling rates.
- Buffalo Gamepad: 8ms (10ms)
- DragonRise, ZeroDelay USB Encoder: 8 ms(10ms)
- Ultimarc U360 Stik: 8ms (10ms)
- FastestJoystick (TeensyLC): 1ms

Note: the numbers in brackets are the requested polling times. The real polling times are a little less.


## SW

- Linux:	Kernel 4.15.0, Ubuntu 16.04.1
- Windows:	Windows 10
- MAME/MESS:	v0.155
- jstest:	Jstest-gtk. Joystick tester v0.1.0


## Linux USB polling rates

Most of the joysticks requested a polling rate of 10ms.
I try to convince Linux to use 1ms instead but without success.
I was also not able to change the mouse polling rate.
Here is what I tried.

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
- Buffalo Gamepad (2nd player): 34 Hz
- DragonRise: 115Hz


Note: when using the teensy as joystick with a requested poll interval of 1ms the Linux is using the 1ms poll interval. Also evhz is showing 1000Hz (but it's important to use the joystick coordinated, e.g. Joystick.X(analogRead(0)), otherwise evhz will not find any event.)



# Test Results

See also [spreadsheet](Docs/LagMeasurements.ods).


## Joysticks

With FastestJoystick it could be seen that the OS response time is about 1-3ms.
FastestJoystick allows also a digital output. So this output can be used to measure the other joysticks more accurate. 


Setup:
1. The 'FastestJoystick' is connected via USB to the NUC (linux) system. It is used tp outut a signal.
2. The USB-Joystick-under-test is connected to the NUC (linux) system.
3. The 'inout' (InputOutput) program is started
4. It will output a digital signal for each button press


With the LagMeter and the digital output connected to the AD2 input (AD2 input is just an analog input) we can measure the minimum response time.


### Joystick Minimum Button Press Time

This measures the minimum time required to press a button so that it is safely recognized by the system (just OS without emulator.


#### FastestJoystick (Teensy)

Constant value of 2ms for more than a day or a million button clicks.
(1457671 button presses, 105240 secs).


#### Buffalo Gamepad

Stopped after 10 mins:
9ms for 10min, 5000 cycles.


#### DragonRise Inc. Generic USB joystick, Zero Delay (Yellow PCB)

Stopped after 3 mins:
22ms for 3 min, 5000 cycles.


#### Microntek USB Joystick (Green PCB)

Stopped after 4 mins:
42ms for 4 min, 1000 cycles.

Note: this might also have something to do with the polling of the USB host. With evhz I was not able to measure it at all.


#### Ultimarc Ultrastik 360

Stopped after 8 mins:
9ms for 8 min, 4000 cycles.


#### Microsoft X-Box 360 pad (white)

Stopped after 3 mins:
21ms for 3 min, 1000 cycles.


#### Microsoft X-Box 360 pad (black)

Stopped after 5 mins:
8ms for 5 min, 3000 cycles.


### Joystick turnaround time

Here the delay is measured from button press until a response (digital out).
So we measure the delay consisting of:
- USB polling
- Game controller delay
- OS delay
- FastestJoystick output delay (1-2ms)


#### FastestJoystick (Teensy)

Repeated delay measuremens (100 cycles): 3-5ms. 

(Very independent of system load. Same times measured also with attract-mode and mame running at the same time in the foreground).


#### Buffalo Gamepad

Delay measurement (100 cycles): 1-9 ms


#### DragonRise Inc. Generic USB joystick, Zero Delay (Yellow PCB)

Delay measurement (100 cycles): 2-31 ms


#### Microntek USB Joystick (Green PCB)

Delay measurement (100 cycles): 4-47 ms


#### Ultimarc Ultrastik 360

Delay measurement (100 cycles): 8-17 ms

Note: The Ultrastik 360 is an analog joystick. Potentially the AD conversion will be a little bit slower than just a button press.
I had no possibility to measure it, but I don't expect that there is a significant additional delay.


#### Microsoft X-Box 360 pad (black)

Delay measurement (100 cycles): 4-14 ms


## OS

With FastestJoystick the complete turnaround time from button press to digital out was constantly 3-5ms.
The FastestJoystick input delay is 0.2ms. Its output delay is <<1ms.
The USB polling was 1ms.
So the theoretical minimum is 0.2ms(inp-delay)+1ms(poll)+0ms(output delay) = 1.2ms.
The theoretical maximum is 1.2ms(inp-delay+poll)+1ms(poll)+1ms(output delay) = 3.2ms.
The rest is OS delay: 2ms.

This even does not change if in parallel an emulator is running or started.
Even with 100% cpu load for both cpus (program 'stress' was used) this did not change.

## Monitor

Measured with photo resistor.

### Results for the Test Setup

The Monitor delay is dependend on the position of the photo sensor on the screen:
With BenQ for example: If the photosensor is positioned at the top the delay is 11ms (to SVGA).
If it is positioned at the bottom the delay is 20ms (toSVGA). 
But this can be explained: the SVGA already start to measure with the first while line, i.e. the top line.
(And as expected the difference in delay if the photo sensor is positioned left or right is neglectible.)

To minimize this effect for the measurements I chose a position always at the top.


### Monitor Delay Depending on Input Connector

The delay of the Eizo itself is 38ms (DVI) and 30ms (SVGA). The BenQ has 12-13ms no matter what input.

I.e. Eizo SVGA input is faster than EIZO DVI input by 8ms. DVI = 38ms, SVGA = 30ms. The delay is very constant.


## NUC Video Output Connector

Measured with AD2 (SVGA) signal.

The NUC has an SVGA and a HDMI output conenctor.
There is no difference in lag for both outputs.


## MAME/MESS

### Different MAME Versions

Measured with photo resistor.

I tested MAME (or better MESS) version 0.155 and 0.214.
Both emulators showed exactly the same delay.
(At least with driver ZXSpectrum).


### Windowed vs. Fullscreen

There seems to be no difference. 


### Shader

The (mameau) CRT geometry shader in MAME adds 10-14 ms to the overall lag.


# Results

## Joysticks

Many USB controller gamepads do strange things especially the cheaper ones.
Very bad is the variance in the delay which could be seen in the Zero Delay USB encoder cards: 2-31 ms and 3-47ms.
This is not really usable.

The Buffalo Gamepad was a surprise. It had a very low delay of 1ms. Unfortunately it uses a polling rate of 8ms so the total delay results in 1-9ms.
Anyhow it would be a good result but unfortunately the Buffalo Controller is reported for its ghosting and yes, also my Buffalo has ghosting:
The left direction is "pressed" quite often (5x in 10mins), the right direction about 1x in 10mins.

As expected the XBox Controller have a good value of 4-14ms. Although it is not extraordinary good.

The Ultrastik 360 is OK with 8-17ms. Although I decided for myself that because its already in the range of a frame I will exchange it againt another joystick.

Because if these results I started to write my own USB game controller firmware which can be found [here](http://www.github.com/FastestJoystick).
With this USB encoder the best results can be achieved: 3-5ms.

Note: from all these results 1-2ms have to be removed for the USB output.


# Conclusions for My Cabinet

1. The Zero Delay encoder and the Ultrastik are replaced by a leaf-switch joystick and the Teensy FastestJoystick encoder. This saves about 26ms at max. and the about the same in variance.
2. Monitor: The Eizo introduces a delay of 1.5 frames delay which is far too big. This monitor needs to be exchanged. 

I started with a delay of 96-153ms for the whole system.
Now (without monitor) I'm at 35-71ms. I have to buy a new (fast) monitor then I hope that I can reduce the original delay by about 50%.

