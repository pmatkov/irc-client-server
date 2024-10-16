# Makefile
CC = gcc

CFLAGS = -g -Wall
TEST_CFLAGS = $(CFLAGS) -DTEST

AR = ar
ARFLAGS = rcs

DLIB_FLAGS = -fPIC -shared -lc
TEST_LDFLAGS = -lcheck -lm -lpthread -lrt -lsubunit -L$(LIBDIR) $(patsubst $(LIBDIR)/lib%.a, -l%, $(LIB_TEST))
MOCK_LDFLAGS = -lncursesw
MOCK_TEST_LDFLAGS = -lmock $(MOCK_LDFLAGS) 

SRCDIR = src
OBJDIR = obj
TESTDIR = tests
LIBDIR = bin

EXCLUDE_SRCS = $(SRCDIR)/libmock.c
SRCS = $(filter-out $(EXCLUDE_SRCS), $(wildcard $(SRCDIR)/*.c))
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

EXCLUDE_TEST_SRCS = $(TESTDIR)/test_hash_table.c
TEST_SRCS = $(filter-out $(EXCLUDE_TEST_SRCS), $(wildcard $(TESTDIR)/*.c))
TEST_OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%_test.o, $(SRCS))
TEST_BINS = $(patsubst $(TESTDIR)/%.c, $(TESTDIR)/bin/%, $(TEST_SRCS))

DEPS = $(OBJS:.o=.d)

SLIBS = $(patsubst $(SRCDIR)/%.c, $(LIBDIR)/%.a, $(SRCS))
DLIBS = $(patsubst $(SRCDIR)/%.c, $(LIBDIR)/%.so, $(SRCS))
LIB = $(LIBDIR)/libcommon.a
LIB_TEST = $(LIBDIR)/libtest.a
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

# build mock object file
$(OBJDIR)/libmock.o: $(SRCDIR)/libmock.c
	$(CC) $(CFLAGS) -c $< -o $@ $(MOCK_LDFLAGS) 
	$(CC) $(CFLAGS) -MM $< -MT $@ -MF $(patsubst %.o, %.d, $@)

# include dependencies
-include $(DEPS)

# run tests
test: $(TEST_BINS)
	for test in $(TEST_BINS); do ./$$test; done

# build test binaries
$(TESTDIR)/bin/%: $(TESTDIR)/%.c $(LIB_TEST)
	$(CC) $(TEST_CFLAGS) $< -o $@ $(TEST_LDFLAGS)

$(TESTDIR)/bin/test_mock: $(TESTDIR)/test_mock.c $(LIB_TEST) $(LIB_MOCK)
	$(CC) $(TEST_CFLAGS) $< -o $@ $(TEST_LDFLAGS) $(MOCK_TEST_LDFLAGS) 

clean:
	rm $(BIN) $(OBJDIR)/* $(LIBDIR)/* $(TESTDIR)/bin/*