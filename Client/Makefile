# Makefile
CC=gcc
CFLAGS=-g -Wall
LDFLAGS=-lncursesw
SRCDIR=src
OBJDIR=obj
BINDIR=bin
BIN=$(BINDIR)/chat
SRCS=$(wildcard $(SRCDIR)/*.c)
OBJS=$(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

# use *.d files in make process if they exist
-include $(OBJS:.o=.d)			

# -MM flag tels compiler to auto generate dependency rules but omit prerequisites on system header files
# prerequisites are saved in *.d files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<
	$(CC) $(CFLAGS) -MM $< > $(OBJDIR)/$*.d		

clean:
	rm $(BIN) $(OBJDIR)/*