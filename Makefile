CROSS=
CC?=$(CROSS)gcc
STRIP?=$(CROSS)strip

INCLUDES=-Iinclude
CFLAGS=-Wall -Wextra -Os
LIBS=-lrt

TARGET=idle
OBJS=cmdline.o idle.o log.o main.o siglist.o

all: clean $(TARGET)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LIBS)
	ls -las $(TARGET)

clean:
	rm -f $(TARGET) *.o core
