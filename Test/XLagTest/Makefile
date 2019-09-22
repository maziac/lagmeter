CC = gcc
CFLAGS  = -g -Wall
TARGET = xlagtest

all:	$(TARGET)

default:	all

$(TARGET):	XLagTest.cpp
	$(CC) $(CFLAGS) -o $(TARGET) XLagTest.cpp -lX11

clean:
	rm $(TARGET)
