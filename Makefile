BUILD_DIR := build

STATIC_LIB := $(BUILD_DIR)/tasks_lib.a
STATIC_LIB_TARGET := deps/ded_tasks

SANITIZER_FLAGS := -fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,integer-divide-by-zero,leak,nonnull-attribute,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,undefined,unreachable,vla-bound,vptr

CFLAGS := -D _DEBUG -ggdb3 -O0 -Wall -Wextra -Waggressive-loop-optimizations -Wmissing-declarations -Wcast-align -Wcast-qual -Wchar-subscripts  -Wconversion -Wempty-body -Wfloat-equal -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat=2 -Winline -Wlogical-op -Wopenmp-simd -Wpacked -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow -Wsign-conversion -Wstrict-overflow=2 -Wsuggest-attribute=noreturn -Wsuggest-final-methods -Wsuggest-final-types -Wswitch-default -Wswitch-enum -Wsync-nand -Wundef -Wunreachable-code -Wunused -Wuseless-cast -Wvariadic-macros -Wno-missing-field-initializers -Wno-narrowing -Wno-varargs -Wstack-protector -fcheck-new -fstack-protector -fstrict-overflow -fno-omit-frame-pointer -Wlarger-than=8192 -Wstack-usage=8192 -pie -fPIE -Werror=vla -Iinclude -D _GNU_SOURCE $(SANITIZER_FLAGS) -I$(STATIC_LIB_TARGET)/include -DSPU

ifdef USE_GTEST
override CFLAGS += -DUSE_GTEST
endif

CXXFLAGS := $(CFLAGS) -Weffc++ -Wc++14-compat -Wconditionally-supported -Wctor-dtor-privacy -Wnon-virtual-dtor -Woverloaded-virtual -Wsign-promo -Wstrict-null-sentinel -Wsuggest-override -Wno-literal-suffix -Wno-old-style-cast -std=c++17 -fsized-deallocation

CXX := g++
CC := gcc
FLAGS = $(CXXFLAGS)

LDFLAGS := -lm

# Uncomment next two lines for C compiler
# OBJCFLAGS := -xc -std=c11
# FLAGS := $(CFLAGS)
# CXX := $(CC)

# -flto-odr-type-merging

TESTLIBSRC := test/test_runner.cpp
TESTLIBOBJ := $(TESTLIBSRC:%.cpp=$(BUILD_DIR)/%.cpp.o)

TESTSRC := test/test_dummy.cpp
TESTOBJ := $(TESTSRC:%.cpp=$(BUILD_DIR)/%.cpp.o)
TEST_LIB_APP := $(BUILD_DIR)/test_derivator

DERIVATOR_SRC := src/expression.c src/tree.c src/derivator_main.c src/expression_derive.c src/expression_evaluate.c src/expression_parser.c src/expression_latex.c src/expression_simplify.c src/expression_plot.c
DERIVATOR_OBJ := $(DERIVATOR_SRC:%.c=$(BUILD_DIR)/%.c.o)
DERIVATOR_APP := $(BUILD_DIR)/derivator

INCPDSRC := $(DERIVATOR_SRC)
INCPDSRC_CPP := $(TESTSRC) $(TESTLIBSRC)
incpd := $(INCPDSRC:%.c=$(BUILD_DIR)/%.c.d) $(INCPDSRC_CPP:%.cpp=$(BUILD_DIR)/%.cpp.d)

OBJFILES := $(LIBOBJ) $(TESTOBJ) $(DERIVATOR_OBJ) $(TESTLIBOBJ)
OBJDIRS := $(sort $(dir $(OBJFILES)))

define INCFIRE
	@echo IN CASE OF FIRE
	@echo GIT COMMIT
	@echo GIT PUSH
	@echo MEOW
	@echo LEAVE BUILDING
	@echo THE TARGET IS BUILT SUCCSESSFULLY
	@echo
endef

.PHONY: build clean run test document build_test objdirs

build: $(DERIVATOR_APP) $(STATIC_LIB)
	$(INCFIRE)

derivator: $(DERIVATOR_APP)
	./$(DERIVATOR_APP)

$(OBJDIRS):
	mkdir -p $(OBJDIRS)

$(filter %.cpp.o,$(OBJFILES)): $(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(OBJDIRS)
	# $< takes only the FIRST dependency
	$(CXX) $(FLAGS) $(OBJCFLAGS) -MP -MMD -c $< -o $@

$(filter %.c.o,$(OBJFILES)): $(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(OBJDIRS)
	# $< takes only the FIRST dependency
	$(CC) $(CFLAGS) $(OBJCFLAGS) -MP -MMD -c $< -o $@

$(STATIC_LIB): $(OBJDIRS)
	$(MAKE) -C $(STATIC_LIB_TARGET)
	cp $(STATIC_LIB_TARGET)/build/tasks_lib.a $(STATIC_LIB)

ifdef USE_GTEST
$(TEST_LIB_APP): $(STATIC_LIB) $(TESTOBJ)
	$(CXX) $(FLAGS) $(LDFLAGS) $(TESTOBJ) $(STATIC_LIB) -lgtest_main -lgtest -o $(TEST_LIB_APP)
else
$(TEST_LIB_APP): $(STATIC_LIB) $(TESTOBJ) $(TESTLIBOBJ)
	$(CXX) $(FLAGS) $(LDFLAGS) $(TESTOBJ) $(TESTLIBOBJ) $(STATIC_LIB) -o $(TEST_LIB_APP)
endif


build_test: $(TEST_LIB_APP)
	$(INCFIRE)

test: build_test
	./$(TEST_LIB_APP)

$(DERIVATOR_APP): $(DERIVATOR_OBJ) $(STATIC_LIB)
	$(CXX) $(FLAGS) $(LDFLAGS) $(DERIVATOR_OBJ) $(STATIC_LIB) -o $@

document: objdirs
	doxygen doxygen.conf

clean:
	rm -rf build

distclean: clean
	$(MAKE) -C $(STATIC_LIB_TARGET) clean

-include $(incpd)
