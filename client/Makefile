# Makefile
CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lncursesw -L$(LIBDIR) $(patsubst $(LIBDIR)/lib%.a, -l%, $(LIB))

SRCDIR = src
OBJDIR = obj
BINDIR = bin
TESTDIR = tests
LIBDIR = ../libs/bin

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))
BIN = $(BINDIR)/client

TEST_SRCS = $(wildcard $(TESTDIR)/*.c)
TEST_OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%_test.o, $(SRCS))
TBINS = $(patsubst $(TESTDIR)/%.c, $(TESTDIR)/bin/%, $(TEST_SRCS))

DEPS = $(OBJS:.o=.d)

LIB = $(LIBDIR)/libcommon.a
LIB_TEST = $(LIBDIR)/libtest.a
LIB_MOCK = $(LIBDIR)/libmock.a

TEST_CFLAGS = $(CFLAGS) -DTEST
TEST_LDFLAGS = -lcheck -lm -lpthread -lrt -lsubunit -lncursesw -L$(LIBDIR) $(patsubst $(LIBDIR)/lib%.a, -l%, $(LIB_TEST) $(LIB_MOCK))

all: $(BIN)

$(BIN): $(OBJS) $(LIB)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

# -MM flag tels the compiler to auto generate dependency rules but omit prerequisites on system header files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
	$(CC) $(CFLAGS) -MM $< -MT $@ -MF $(patsubst %.o, %.d, $@)

$(OBJDIR)/%_test.o: $(SRCDIR)/%.c
	$(CC) $(TEST_CFLAGS) -c $< -o $@
	$(CC) $(TEST_CFLAGS) -MM $< -MT $@ -MF $(patsubst %_test.o, %_test.d, $@)

-include $(DEPS)

test: $(TBINS)
	for test in $(TBINS); do ./$$test; done

$(TESTDIR)/bin/%: $(TESTDIR)/%.c $(TEST_OBJS) $(LIB_TEST) $(LIB_MOCK)
	$(CC) $(TEST_CFLAGS) $< $(TEST_OBJS) -o $@ $(TEST_LDFLAGS)

clean:
	rm $(BIN) $(OBJDIR)/* $(TESTDIR)/bin/*