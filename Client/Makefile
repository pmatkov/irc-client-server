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

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

# -MM flag tels the compiler to auto generate dependency rules but omit prerequisites on system header files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
	$(CC) $(CFLAGS) -MM $< -MT $@ -MF $(patsubst %.o, %.d, $@)

-include $(DEPS)

clean:
	rm $(BIN) $(OBJDIR)/*