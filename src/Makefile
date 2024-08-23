CC ?= gcc
PKGCONFIG = $(shell which pkg-config)
CFLAGS = $(shell $(PKGCONFIG) --cflags gtk4)
LIBS = $(shell $(PKGCONFIG) --libs gtk4) -lm -lasound -lsqlite3 -lnotify

SRC = main.c customcalendar.c calendarevent.c dbmanager.c

OBJS = $(SRC:.c=.o)

all: speakingcalendar


%.o: %.c
	$(CC) -c -o $(@F) $(CFLAGS) $<


speakingcalendar: $(OBJS)
	$(CC) -o $(@F) $(OBJS) $(LIBS)

clean:
	rm -f $(OBJS)
	rm -f speakingcalendar

