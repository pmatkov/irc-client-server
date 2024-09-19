# Makefile
CC = gcc

CFLAGS = -g -Wall
TEST_CFLAGS = $(CFLAGS) -DTEST

AR = ar
ARFLAGS = rcs

DLIB_FLAGS = -fPIC -shared -lc
TEST_LDFLAGS = -lcheck -lm -lpthread -lrt -lsubunit -L$(LIBDIR) $(patsubst $(LIBDIR)/lib%.a, -l%, $(TEST_LIBCOM) $(LIBMOCK)) -lncursesw

SRCDIR = src
OBJDIR = obj
TESTDIR = tests
LIBDIR = lib

EXCLUDE_SRCS = $(SRCDIR)/libmock.c
EXCLUDE_TSRCS = $(TESTDIR)/test_hash_table.c
SRCS = $(filter-out $(EXCLUDE_SRCS), $(wildcard $(SRCDIR)/*.c))
TSRCS = $(filter-out $(EXCLUDE_TSRCS), $(wildcard $(TESTDIR)/*.c))

OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(filter-out $(SRCDIR)/libmock.c, $(SRCS)))
TEST_OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%_test.o, $(filter-out $(SRCDIR)/libmock.c, $(SRCS)))

TBINS = $(patsubst $(TESTDIR)/%.c, $(TESTDIR)/bin/%, $(TSRCS))

DEPS = $(OBJS:.o=.d)

SLIBS = $(patsubst $(SRCDIR)/%.c, $(LIBDIR)/%.a, $(SRCS))
DLIBS = $(patsubst $(SRCDIR)/%.c, $(LIBDIR)/%.so, $(SRCS))
LIBCOM = $(LIBDIR)/libcommon.a
TEST_LIBCOM = $(LIBDIR)/libtestcommon.a
LIBMOCK = $(LIBDIR)/libmock.a

all: $(LIBCOM)

libmock: $(LIBMOCK)

$(LIBMOCK): $(OBJDIR)/libmock.o
	$(AR) $(ARFLAGS) -o $@ $^

# build test common static library
test_libcom: $(TEST_LIBCOM)

$(TEST_LIBCOM): $(TEST_OBJS)
	$(AR) $(ARFLAGS) -o $@ $^

# build common static library
libcom: $(LIBCOM)

$(LIBCOM): $(OBJS)
	$(AR) $(ARFLAGS) -o $@ $^

# build static libraries
slibs: $(SLIBS)

$(SLIBS): $(OBJS)
	$(AR) $(ARFLAGS) -o $@ $<

# build dynamic libraries
dlibs: $(DLIBS)

$(DLIBS): $(SRCS)
	$(CC) $(CFLAGS) $(DLIB_FLAGS) -o $@ $<

objs: $(OBJS) $(TEST_OBJS)

# build object files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
	$(CC) $(CFLAGS) -MM $< -MT $@ -MF $(patsubst %.o, %.d, $@)

# build test object files
$(OBJDIR)/%_test.o: $(SRCDIR)/%.c
	$(CC) $(TEST_CFLAGS) -c $< -o $@
	$(CC) $(TEST_CFLAGS) -MM $< -MT $@ -MF $(patsubst %_test.o, %_test.d, $@)

# include dependencies
-include $(DEPS)

# run tests
test: $(TBINS)
	for test in $(TBINS); do ./$$test; done

# build test binaries
$(TESTDIR)/bin/%: $(TESTDIR)/%.c $(TEST_LIBCOM) $(LIBMOCK)
	$(CC) $(TEST_CFLAGS) $< -o $@ $(TEST_LDFLAGS)

clean:
	rm $(BIN) $(OBJDIR)/* $(LIBDIR)/* $(TESTDIR)/bin/*