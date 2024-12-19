# Makefile
CC = gcc
CFLAGS = -g -Wall
TEST_CFLAGS = $(CFLAGS) -DTEST

LDFLAGS = -lpthread -L$(LIBDIR) $(patsubst $(LIBDIR)/lib%.a, -l%, $(LIB)) -lm
TEST_LDFLAGS = -lcheck -lm -lpthread -lrt -lsubunit -L$(LIBDIR) $(patsubst $(LIBDIR)/lib%.a, -l%, $(LIB_TEST) $(LIB_MOCK)) -lncursesw

SRCDIR = src
OBJDIR = obj
BINDIR = bin
TESTDIR = tests
LIBDIR = ../libs/bin

EXCLUDE_SRCS = $(SRCDIR)/core.c $(SRCDIR)/mt_core.c
SRCS = $(filter-out $(EXCLUDE_SRCS), $(wildcard $(SRCDIR)/*.c))
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))
BIN = $(BINDIR)/server

EXCLUDE_TEST_SRCS =
TEST_SRCS = $(filter-out $(EXCLUDE_TEST_SRCS), $(wildcard $(TESTDIR)/*.c))
TEST_OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%_test.o, $(SRCS))
TEST_BINS = $(patsubst $(TESTDIR)/%.c, $(TESTDIR)/bin/%, $(TEST_SRCS))

DEPS = $(OBJS:.o=.d)

LIB = $(LIBDIR)/libcommon.a
LIB_TEST = $(LIBDIR)/libtest.a
LIB_MOCK = $(LIBDIR)/libmock.a

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

test: $(TEST_BINS)
	for test in $(TEST_BINS); do ./$$test; done

$(TESTDIR)/bin/%: $(TESTDIR)/%.c $(TEST_OBJS) $(LIB_TEST) $(LIB_MOCK)
	$(CC) $(TEST_CFLAGS) $< $(TEST_OBJS) -o $@ $(TEST_LDFLAGS)

clean:
	rm $(BIN) $(OBJDIR)/* $(TESTDIR)/bin/*