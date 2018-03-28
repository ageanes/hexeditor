PROGRAM := hexeditor

CFLAGS:=-lncurses -fpie -Wl,-z,relro

ifdef DEBUG
	CFLAGS := $(CFLAGS) -g -DDEBUG=1
endif

SRCFILES=$(wildcard *.c)

OBJS = main.o $(SRCFILES:%.c=%.o)

DEPS = $(OBJS:%.o=%.d)

$(PROGRAM): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $^

-include $(DEPS)

%.o: %.c
	$(CC) -c -o $@ $(CFLAGS) $< -MMD

clean:
	rm -f $(OBJS)
	rm -rf $(DEPS)
