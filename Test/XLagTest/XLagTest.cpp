
/**
 * Author: Jason White
 * 
 * Description:
 * Reads joystick/gamepad events and displays them.
 *
 * Compile:
 * gcc -g -Wall InputOutput.cpp
 * or make.
 *
 * Run e.g.:
 * ./joystick /dev/input/jsX /dev/serial/by-id/usb-Maziac_Arcade_Joystick_Encoder_5668360-if00 
 *
 * See also:
 * https://www.kernel.org/doc/Documentation/input/joystick-api.txt
 * 
 * Modified by maziac to write any change to serial output of
 * 'FastestJoystick' (a Teensy Joystick project).
 * The idea is just to test/measure the turnaround times.
 * 
 * Notes:
 * To access the serial port on linux as normal user you need to give access rights with:
 * sudo usermod -a -G dialout <user>
 * (Maybe also: sudo usermod -a -G plugdev <user>)
 * and then reboot.
 * 
 * 
 */
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <linux/joystick.h>


/**
 * Reads a joystick event from the joystick device.
 *
 * Returns 0 on success. Otherwise -1 is returned.
 */
int read_event(int fd, struct js_event *event)
{
    ssize_t bytes;

    bytes = read(fd, event, sizeof(*event));

    if (bytes == sizeof(*event))
        return 0;

    /* Error, could not read full event. */
    return -1;
}

/**
 * Returns the number of axes on the controller or 0 if an error occurs.
 */
size_t get_axis_count(int fd)
{
    __u8 axes;

    if (ioctl(fd, JSIOCGAXES, &axes) == -1)
        return 0;

    return axes;
}

/**
 * Returns the number of buttons on the controller or 0 if an error occurs.
 */
size_t get_button_count(int fd)
{
    __u8 buttons;
    if (ioctl(fd, JSIOCGBUTTONS, &buttons) == -1)
        return 0;

    return buttons;
}

/**
 * Current state of an axis.
 */
struct axis_state {
    short x, y;
};

/**
 * Keeps track of the current axis state.
 *
 * NOTE: This function assumes that axes are numbered starting from 0, and that
 * the X axis is an even number, and the Y axis is an odd number. However, this
 * is usually a safe assumption.
 *
 * Returns the axis that the event indicated.
 */
size_t get_axis_state(struct js_event *event, struct axis_state axes[3])
{
    size_t axis = event->number / 2;

    if (axis < 3)
    {
        if (event->number % 2 == 0)
            axes[axis].x = event->value;
        else
            axes[axis].y = event->value;
    }

    return axis;
}


// Writes a digital output value.
// fd: Serial device
// dout: dout number
// on: true/or false. On or off.
void write_dout(int fd, int dout, bool on) {
    char buffer[100];
    sprintf(buffer, "o%d=%d\n", dout, on? 1 : 0);
    printf("%s", buffer);
    int len = strlen(buffer); 
    ssize_t result = write(fd, buffer, len);
    if(result != len) {
        perror("Failed to write to serial");
        exit(-1);
    }
}


// Main program
int main(int argc, char *argv[])
{
    const char* js_device;
    int js;
    const char* serial_device;
    int serial;
    struct js_event event;
    struct axis_state axes[3] = {};
    size_t axis;

    // Open joystick device
    if (argc > 1)
        js_device = argv[1];
    else
        js_device = "/dev/input/js0";

    js = open(js_device, O_RDONLY);

    if (js == -1) {
        perror("Could not open joystick");
        return -1;
    }


    // Open serial port
    if (argc < 3) {
        printf("No serial port given.\n");
        return -1;
    }
    serial_device = argv[2];
    serial = open(serial_device, O_RDWR | O_SYNC);
    printf("Serial device=%s (%d)\n", serial_device, serial);
    if (serial == -1) {
        perror("Could not open serial device");
        return -1;
    }


    /* This loop will exit if the controller is unplugged. */
    while (read_event(js, &event) == 0)
    {
        switch (event.type)
        {
            case JS_EVENT_BUTTON:
                printf("Button %u %s\n", event.number, event.value ? "pressed" : "released");
                write_dout(serial, 1, event.value);
                break;
            case JS_EVENT_AXIS:
                axis = get_axis_state(&event, axes);
                if (axis < 3)
                    printf("Axis %zu at (%6d, %6d)\n", axis, axes[axis].x, axes[axis].y);
                break;
            default:
                /* Ignore init events. */
                break;
        }
    }

    close(serial);
    close(js);
    return 0;
}
