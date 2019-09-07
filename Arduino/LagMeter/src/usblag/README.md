# usblag

Arduino Project to measure USB Controller input lag

## Hardware requirement

You need a arduino board with a [USB Host Shield](https://github.com/felis/USB_Host_Shield_2.0) properly installed.

_Note_: if you own a rev1 board (not ICSP connector) be sure to check out the hardware manual to bind the four signals to the right pins.

After that, hook up the desired button signal using the necessary protection like an inverted diode to pin 7 and you're good to go.

## Software Installation

Assuming that you have the Arduino IDE, just install the library [USB Host Shield Library 2.0](https://github.com/felis/USB_Host_Shield_2.0)
in the Tools > Manage Libraries section. After that, upload the sketch to the board, and open the serial monitor.

## Device Support
This contains extra-minimal drivers for HID compliant devices and standard XInput devices (maybe not the latest xbox one stuff thought). Don't hesitate to create an MR if one of your device is not supported.

This program may or may not support hotplugs so be sure to hit the reset button if you switch you device.

## Commands
The serial monitor will support different commands to play with your device:

| command | effect |
| ------ | ------ |
| 0 | Set the button signal to False |
| 1 | Set the button signal to True |
| t | Fire a test of 1000 data points |
| o | Override polling interval to 1ms |
| p | Override polling interval like a PC would do |
| n | Set the polling interval back to normal |
| sX | Override polling interval to Xms |
| l | Displays the latest loop duration |

## Protocol

* Connect your device
* Reset the arduino to be sure
* Note the polling interval
* Try command 0 and 1 to see if you signal is properly routed to your board. If it works you should see a lag value in ms.
* Clear output
* Send the command t... wait
* Change the polling interval at will and do the experiment again
 
Don't hesitate to use the [google spreadsheet](https://docs.google.com/spreadsheets/d/1kG7k6A1OHC0YIzG-KFlUMSfKDR1uYCY3-1D4dpXatsc/edit?usp=sharing) to analyze the results.

## Licence
The drivers code are mainly built on extracted code from the [USB Host Shield Library 2.0](https://github.com/felis/USB_Host_Shield_2.0), licenced using GPL2.

The rest of the code is under the [MIT Licence](LICENSE).