# Makefile
CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lncursesw

SRCDIR = src
OBJDIR = obj
BINDIR = bin
BIN = $(BINDIR)/server

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))
DEPS = $(OBJS:.o=.d)

TDIR = tests
TSRCS = $(wildcard $(TDIR)/*.c)
TBINS = $(patsubst $(TDIR)/%.c, $(TDIR)/bin/%, $(TSRCS))

TEST_CFLAGS = $(CFLAGS) -DTEST
TEST_LDFLAGS = -lcheck -lm -lpthread -lrt -lsubunit -lncursesw

# TEST_OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(filter-out $(SRCDIR)/main.c, $(SRCS))) $(OBJDIR)/main_test.o

TEST_OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%_test.o, $(SRCS))

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

# -MM flag tels the compiler to auto generate dependency rules but omit prerequisites on system header files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
	$(CC) $(CFLAGS) -MM $< -MT $@ -MF $(patsubst %.o, %.d, $@)

$(OBJDIR)/%_test.o: $(SRCDIR)/%.c
	$(CC) $(TEST_CFLAGS) -c $< -o $@
	$(CC) $(TEST_CFLAGS) -MM $< -MT $@ -MF $(patsubst %_test.o, %_test.d, $@)

# $(OBJDIR)/main_test.o: $(SRCDIR)/main.c
# 	$(CC) $(TEST_CFLAGS) -c $< -o $@
# 	$(CC) $(TEST_CFLAGS) -MM $< -MT $@ -MF $(patsubst %.o, %.d, $@)

-include $(DEPS)

test: $(TBINS)
	for test in $(TBINS); do ./$$test; done

$(TDIR)/bin/%: $(TDIR)/%.c $(TEST_OBJS)
	$(CC) $(TEST_CFLAGS) $< $(TEST_OBJS) -o $@ $(TEST_LDFLAGS)

clean:
	rm $(BIN) $(OBJDIR)/* $(TDIR)/bin/*