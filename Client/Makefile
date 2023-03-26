# Makefile
CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lncursesw
SRCDIR = src
OBJDIR = obj
BINDIR = bin
BIN = $(BINDIR)/client
SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))
DEPS = $(OBJS:.o=.d)

TDIR = tests
TSRCS = $(wildcard $(TDIR)/*.c)
TBINS = $(patsubst $(TDIR)/%.c, $(TDIR)/bin/%, $(TSRCS))

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

# -MM flag tels the compiler to auto generate dependency rules but omit prerequisites on system header files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
	$(CC) $(CFLAGS) -MM $< -MT $@ -MF $(patsubst %.o, %.d, $@)

-include $(DEPS)

$(TDIR)/bin/%: $(TDIR)/%.c
	$(CC) $(CFLAGS) $< $(OBJDIR)/tcpclient.o -o $@ -lcheck

test: $(TDIR)/bin $(TBINS)
	for test in $(TBINS); do ./$$test; done

clean:
	rm $(BIN) $(OBJDIR)/* $(TDIR)/bin/*