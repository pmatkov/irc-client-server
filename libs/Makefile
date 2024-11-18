# Makefile
CC = gcc

CFLAGS = -g -Wall
TEST_CFLAGS = $(CFLAGS) -DTEST

AR = ar
ARFLAGS = rcs
DLIB_FLAGS = -fPIC -shared -lc

TEST_LDFLAGS = -lcheck -lm -lpthread -lrt -lsubunit
LIB_LDFLAGS = -L$(LIBDIR) $(patsubst $(LIBDIR)/lib%.a, -l%, $(LIB_TEST))
LIB2_LDFLAGS = -L$(LIBDIR) $(patsubst $(LIBDIR)/lib%.a, -l%, $(LIB_TEST2))
MOCK_LDFLAGS = -lmock -lncursesw

# List of binaries that require linking with the mock library
LINK_MOCK_BINS = test_mock test_io_utils test_threads

SRCDIR = src
OBJDIR = obj
TESTDIR = tests
LIBDIR = bin

EXCLUDE_SRCS = $(SRCDIR)/libmock.c
SRCS = $(filter-out $(EXCLUDE_SRCS), $(wildcard $(SRCDIR)/*.c))
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

# EXCLUDE_TEST_SRCS = $(TESTDIR)/test_hash_table.c
EXCLUDE_TEST_SRCS = 
TEST_SRCS = $(filter-out $(EXCLUDE_TEST_SRCS), $(wildcard $(TESTDIR)/*.c))
TEST_OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%_test.o, $(SRCS))
TEST_BINS = $(patsubst $(TESTDIR)/%.c, $(TESTDIR)/bin/%, $(TEST_SRCS))

EXCLUDE_OBJS = $(OBJDIR)/libthreads_test.o $(OBJDIR)/libio_utils_test.o
INCLUDE_OBJS = $(OBJDIR)/libthreads_test2.o $(OBJDIR)/libio_utils_test2.o

DEPS = $(OBJS:.o=.d)

SLIBS = $(patsubst $(SRCDIR)/%.c, $(LIBDIR)/%.a, $(SRCS))
DLIBS = $(patsubst $(SRCDIR)/%.c, $(LIBDIR)/%.so, $(SRCS))
LIB = $(LIBDIR)/libcommon.a
LIB_TEST = $(LIBDIR)/libtest.a
LIB_TEST2 = $(LIBDIR)/libtest2.a
LIB_MOCK = $(LIBDIR)/libmock.a

all: lib libtest libmock

# build common library
lib: $(LIB)

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) -o $@ $^

# build common library for testing
libtest: $(LIB_TEST)

$(LIB_TEST): $(TEST_OBJS)
	$(AR) $(ARFLAGS) -o $@ $^

libtest2: $(LIB_TEST2)

$(LIB_TEST2): $(filter-out $(EXCLUDE_OBJS), $(TEST_OBJS)) $(INCLUDE_OBJS)
	$(AR) $(ARFLAGS) -o $@ $^

# build mocking library
libmock: $(LIB_MOCK)

$(LIB_MOCK): $(OBJDIR)/libmock.o
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

$(OBJDIR)/%_test2.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
	$(CC) $(CFLAGS) -MM $< -MT $@ -MF $(patsubst %_test2.o, %_test2.d, $@)

# build mock object file
$(OBJDIR)/libmock.o: $(SRCDIR)/libmock.c
	$(CC) $(CFLAGS) -c $< -o $@
	$(CC) $(CFLAGS) -MM $< -MT $@ -MF $(patsubst %.o, %.d, $@)

# include dependencies
-include $(DEPS)

# run tests
test: $(TEST_BINS)
	for test in $(TEST_BINS); do ./$$test; done

# build test binaries
$(TESTDIR)/bin/%: $(TESTDIR)/%.c $(LIB_TEST) $(if $(filter $*, $(LINK_MOCK_BINS)), $(LIB_MOCK))
	$(CC) $(TEST_CFLAGS) $< -o $@ $(TEST_LDFLAGS) $(LIB_LDFLAGS) $(if $(filter $*, $(LINK_MOCK_BINS)), $(MOCK_LDFLAGS))

$(TESTDIR)/bin/test_threads: $(TESTDIR)/test_threads.c $(LIB_TEST2)
	$(CC) $(TEST_CFLAGS) $< -o $@ $(TEST_LDFLAGS) $(LIB2_LDFLAGS)

$(TESTDIR)/bin/test_signal_handler: $(TESTDIR)/test_signal_handler.c $(LIB_TEST2)
	$(CC) $(TEST_CFLAGS) $< -o $@ $(TEST_LDFLAGS) $(LIB2_LDFLAGS)

clean:
	rm $(BIN) $(OBJDIR)/* $(LIBDIR)/* $(TESTDIR)/bin/*