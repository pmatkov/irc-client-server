# Makefile
CC = gcc
CFLAGS = -g -Wall
AR = ar
ARFLAGS = rcs
DLIB_FLAGS = -fPIC -shared -lc
SLIB_FLAGS = -L$(LIBDIR) $(patsubst $(LIBDIR)/%.a, -l:%.a, $(LIB))

SRCDIR = src
OBJDIR = obj
LIBDIR = lib

# source and object Files
SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))
DEPS = $(OBJS:.o=.d)

DLIBS = $(patsubst $(SRCDIR)/%.c, $(LIBDIR)/%.so, $(SRCS))
SLIBS = $(patsubst $(SRCDIR)/%.c, $(LIBDIR)/%.a, $(SRCS))
LIBS = $(patsubst $(SRCDIR)/%.c, %.a, $(SRCS))
LIB = $(LIBDIR)/libcommon.a

# test dirs and files
TDIR = tests
TSRCS = $(wildcard $(TDIR)/*.c)
TBINS = $(patsubst $(TDIR)/%.c, $(TDIR)/bin/%, $(TSRCS))

# special test requirements
TEST_CFLAGS = $(CFLAGS) -DTEST
TEST_LDFLAGS = -lcheck -lm -lpthread -lrt -lsubunit
TEST_LIBFLAGS = -L$(LIBDIR) $(patsubst $(LIBDIR)/%.a, -l:%.a, $(TEST_LIB))

TEST_OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%_test.o, $(SRCS))
TEST_LIB = $(LIBDIR)/libtestcommon.a

all: $(SLIBS) $(DLIBS) $(OBJS)

# build test common static library
test_libcom: $(TEST_LIB)

$(TEST_LIB): $(TEST_OBJS)
	$(AR) $(ARFLAGS) -o $@ $^

# build common static library
libcom: $(LIB)

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) -o $@ $^

libs: $(SLIBS) $(DLIBS)

# build static library
$(SLIBS): $(OBJS)
	$(AR) $(ARFLAGS) -o $@ $<

# build dynamic libraries
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
$(TDIR)/bin/%: $(TDIR)/%.c $(TEST_LIB)
	$(CC) $(TEST_CFLAGS) $^ -o $@ $(TEST_LDFLAGS) $(TEST_LIBFLAGS)

clean:
	rm $(BIN) $(OBJDIR)/* $(LIBDIR)/* $(TDIR)/bin/*