CC	?= gcc
CFLAGS	?= -O2 -pthread
CFLAGS	+= -Wall
LDFLAGS	+= -lX11 -s
PROG	= xkbs1kls
OBJS	= xkbs1kls.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

$(OBJS): *.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(PROG) $(OBJS)

.PHONY: all install clean
