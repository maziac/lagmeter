
/**
 * Author: Jason White
 * 
 * Description:
 * Reads joystick/gamepad events and displays them.
 *
 * Compile:
 * gcc -g -Wall XLagTest.cpp
 * or make.
 *
 * Run e.g.:
 * ./xlagtest [/dev/input/jsX]
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

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>

/**
 * Reads a joystick event from the joystick device.
 *
 * Returns 0 on success. Otherwise -1 is returned.
 */
int read_event(int fd, struct js_event *event)
{
    ssize_t bytes;

    bytes = read(fd, event, sizeof(*event));

    return bytes;
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


// X11
Display *dis;
int screen;
Window win;
GC gc;
unsigned long black,white;

void init_x() {
	/* get the colors black and white (see section for details) */

	/* use the information from the environment variable DISPLAY 
	   to create the X connection:
	*/	
	dis=XOpenDisplay((char *)0);
   	screen=DefaultScreen(dis);
	black=BlackPixel(dis,screen),	/* get color black */
	white=WhitePixel(dis, screen);  /* get color white */

	/* once the display is initialized, create the window.
	   This window will be have be 200 pixels across and 300 down.
	   It will have the foreground white and background black
	*/
   	win=XCreateSimpleWindow(dis,DefaultRootWindow(dis),0,0,	
		200, 300, 5, white, black);

	/* here is where some properties of the window can be set.
	   The third and fourth items indicate the name which appears
	   at the top of the window and the name of the minimized window
	   respectively.
	*/
	XSetStandardProperties(dis,win,"Press q to quit","",None,NULL,0,NULL);

	/* this routine determines which types of input are allowed in
	   the input.  see the appropriate section for details...
	*/
	XSelectInput(dis, win, ExposureMask|ButtonPressMask|KeyPressMask);

	/* create the Graphics Context */
    gc=XCreateGC(dis, win, 0,0);        

	/* here is another routine to set the foreground and background
	   colors _currently_ in use in the window.
	*/
	XSetBackground(dis,gc,white);
	XSetForeground(dis,gc,black);

	/* clear the window and bring it on top of the other windows */
	XClearWindow(dis, win);
	XMapRaised(dis, win);
}

void close_x() {
/* it is good programming practice to return system resources to the 
   system...
*/
	XFreeGC(dis, gc);
	XDestroyWindow(dis,win);
	XCloseDisplay(dis);	
	exit(1);				
}


void redraw() {
	XClearWindow(dis, win);
}


// Main program
int main(int argc, char *argv[])
{
    const char* js_device;
    int js;
    struct js_event jsevent;
    struct axis_state axes[3] = {};
    size_t axis;

    // Open joystick device
    if (argc > 1)
        js_device = argv[1];
    else
        js_device = "/dev/input/js0";

    js = open(js_device, O_RDONLY | O_NONBLOCK);

    if (js == -1) {
        perror("Could not open joystick");
        return -1;
    }

	XEvent event;		/* the XEvent declaration !!! */
	KeySym key;		/* a dealie-bob to handle KeyPress Events */	
	char text[255];		/* a char buffer for KeyPress Events */

	init_x();

	/* look for events forever... */
	while(1) {		
		// Process pending events:
        while (XPending(dis) > 0) {
            /* get the next event and stuff it into our event variable.
            Note:  only events we set the mask for are detected!
            */
            XNextEvent(dis, &event);
            printf("1\n");
        //continue;

            if (event.type==Expose && event.xexpose.count==0) {
            /* the window was exposed redraw it! */
                redraw();
            }
            if (event.type==KeyPress&&
                XLookupString(&event.xkey,text,255,&key,0)==1) {
                /* use the XLookupString routine to convert the invent
                KeyPress data into regular text.  Weird but necessary...
                */

                if (text[0]=='c') {
                    XSetForeground(dis,gc,white);
                    XFillRectangle(dis,win,gc, 0, 0, -1, -1);
                }
                else if (text[0]=='v') {
                    XSetForeground(dis,gc,black);
                    XFillRectangle(dis,win,gc, 0, 0, -1, -1);
                }
                else if (text[0]=='q') {
                    close_x();
                }
                printf("You pressed the %c key!\n",text[0]);
            }
            if (event.type==ButtonPress) {
            /* tell where the mouse Button was Pressed */
                int x=event.xbutton.x,
                    y=event.xbutton.y;

                strcpy(text,"X is FUN!");
                XSetForeground(dis,gc,rand()%event.xbutton.x%255);
                XDrawString(dis,win,gc,x,y, text, strlen(text));
            }
        }


        /* This loop will exit if the controller is unplugged. */
        if(read_event(js, &jsevent) != -1)
        {
            switch (jsevent.type)
            {
                case JS_EVENT_BUTTON:
                    printf("Button %u %s\n", jsevent.number, jsevent.value ? "pressed" : "released");
                    XSetForeground(dis, gc, jsevent.value ? white : black);
                    XFillRectangle(dis, win, gc, 0, 0, -1, -1);
                    break;
                case JS_EVENT_AXIS:
                    axis = get_axis_state(&jsevent, axes);
                    if (axis < 3)
                        printf("Axis %zu at (%6d, %6d)\n", axis, axes[axis].x, axes[axis].y);
                    break;
                default:
                    /* Ignore init events. */
                    break;
            }
        }

        printf("2\n");
    }

    close(js);
    return 0;
}
