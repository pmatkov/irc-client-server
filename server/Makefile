# Makefile
CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -L$(LIBDIR) $(patsubst $(LIBDIR)/lib%.a, -l%, $(LIBCOM))

SRCDIR = src
OBJDIR = obj
BINDIR = bin
TDIR = tests
LIBDIR = ../shared/lib

SRCS = $(wildcard $(SRCDIR)/*.c)
TSRCS = $(wildcard $(TDIR)/*.c)

OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))
TEST_OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%_test.o, $(SRCS))

DEPS = $(OBJS:.o=.d)

LIBCOM = $(LIBDIR)/libcommon.a
TEST_LIBCOM = $(LIBDIR)/libtestcommon.a
LIBMOCK = $(LIBDIR)/libmock.a

BIN = $(BINDIR)/server
TBINS = $(patsubst $(TDIR)/%.c, $(TDIR)/bin/%, $(TSRCS))

TEST_CFLAGS = $(CFLAGS) -DTEST
TEST_LDFLAGS = -lcheck -lm -lpthread -lrt -lsubunit -L$(LIBDIR) $(patsubst $(LIBDIR)/lib%.a, -l%, $(TEST_LIBCOM) $(LIBMOCK))

all: $(BIN)

$(BIN): $(OBJS) $(LIBCOM)
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

$(TDIR)/bin/%: $(TDIR)/%.c $(TEST_OBJS) $(TEST_LIBCOM) $(LIBMOCK)
	$(CC) $(TEST_CFLAGS) $< $(TEST_OBJS) -o $@ $(TEST_LDFLAGS)

clean:
	rm $(BIN) $(OBJDIR)/* $(TDIR)/bin/*