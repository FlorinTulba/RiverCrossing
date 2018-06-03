# Usage:
#
# 	make [release] 					compiles the release, without the tests
# 	make debug 							compiles the debug version, without the tests
# 	make tests 							compiles the debug version of the unit tests
# 	make all 								compiles release, debug and debug for the tests
# 	make clean							removes object files from the project and the tests

# For processing the targets in parallel, call it like:
#		make -j<x> -Otarget [<t>]
# 		with <x> replaced by the number of threads to use
# 		and <t> either not provided or replaced by one of release, debug, tests, all

# This Makefile uses the technique described in the following article:
# http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/

# The C++ sources of the RiverCrossing project
SOURCES = \
	symbolsTable.cpp \
	entity.cpp \
	entitiesManager.cpp \
	configConstraint.cpp \
	configParser.cpp \
	solver.cpp \
	scenario.cpp

TEST_SOURCES = $(SOURCES) \
	testMain.cpp

COMPILER = g++
LINKER = g++

RELEASE_DEPDIR := .r
DEBUG_DEPDIR := .d
TEST_DEPDIR := .td
$(shell mkdir -p $(RELEASE_DEPDIR) >/dev/null)
$(shell mkdir -p $(DEBUG_DEPDIR) >/dev/null)
$(shell mkdir -p $(TEST_DEPDIR) >/dev/null)

COMMON_FLAGS = -MT $@ -MMD -MP -MF
RELEASE_DEPFLAGS = $(COMMON_FLAGS) $(RELEASE_DEPDIR)/$*.Td
DEBUG_DEPFLAGS = $(COMMON_FLAGS) $(DEBUG_DEPDIR)/$*.Td
TEST_DEPFLAGS = $(COMMON_FLAGS) $(TEST_DEPDIR)/$*.Td

CXX_FLAGS = -std=c++14 -m64 -Ofast -Wall
BYPASSED_WARNINGS = \
	-Wno-missing-declarations \
	-Wno-unknown-pragmas \
	-Wno-reorder

RELEASE_COMPILE_FLAGS = -c $(CXX_FLAGS) $(RELEASE_DEPFLAGS) \
	$(BYPASSED_WARNINGS) \
	-DNDEBUG
DEBUG_COMPILE_FLAGS = -c -g $(CXX_FLAGS) $(DEBUG_DEPFLAGS) \
	$(BYPASSED_WARNINGS)
TEST_COMPILE_FLAGS = -c -g $(CXX_FLAGS) $(TEST_DEPFLAGS) \
	$(BYPASSED_WARNINGS) \
	-DUNIT_TESTING

COMMON_LINK_FLAGS = $(CXX_FLAGS) -static
RELEASE_LINK_FLAGS = $(COMMON_LINK_FLAGS)
DEBUG_LINK_FLAGS = $(COMMON_LINK_FLAGS)
TEST_LINK_FLAGS = $(COMMON_LINK_FLAGS) -Wl,--allow-multiple-definition

SRC_DIR = src/
TEST_DIR = test/
BASE_OUT_DIR = x64/$(COMPILER)/
RELEASE_OUT_DIR = $(BASE_OUT_DIR)Release/
DEBUG_OUT_DIR = $(BASE_OUT_DIR)Debug/
TESTS_OUT_DIR = $(BASE_OUT_DIR)tests/
$(shell mkdir -p $(RELEASE_OUT_DIR) >/dev/null)
$(shell mkdir -p $(DEBUG_OUT_DIR) >/dev/null)
$(shell mkdir -p $(TESTS_OUT_DIR) >/dev/null)
TARGET = RiverCrossing.exe

BOOST_DIR = /cygdrive/e/Work/VS/C_Cpp/FromOthers/boost/
INCLUDES = \
	-I"$(SRC_DIR)" \
	-I"$(BOOST_DIR)"
LIBS_DIRS = \
	-L"$(BOOST_DIR)stage64/g++_std_14/lib/"
LIB_DEPS = \
	-lboost_system -lboost_filesystem
TEST_LIB_DEPS = \
	$(LIB_DEPS) -lboost_unit_test_framework

# release depends on the generation of the release target without the unit tests
.PHONY : release
release :: $(RELEASE_OUT_DIR)$(TARGET)

# debug depends on the generation of the debug target without the unit tests
.PHONY : debug
debug :: $(DEBUG_OUT_DIR)$(TARGET)

# tests depends on the generation of the unit tests target
.PHONY : tests
tests :: $(TESTS_OUT_DIR)$(TARGET)

# all depends first on the generation of the targets
.PHONY : all
all :: debug release tests

# The targets depend on the `o` files from RELEASE_OUT_DIR / DEBUG_OUT_DIR / TESTS_OUT_DIR
# from the corresponding `cpp` among SOURCES / TEST_SOURCES
$(RELEASE_OUT_DIR)$(TARGET) : $(patsubst %.cpp,$(RELEASE_OUT_DIR)%.o,$(SOURCES))
	@echo
	@echo ==== Linking the release object files ====
	$(LINKER) $(RELEASE_LINK_FLAGS) -o $@ $(RELEASE_OUT_DIR)*.o $(LIBS_DIRS) $(LIB_DEPS)

$(DEBUG_OUT_DIR)$(TARGET) : $(patsubst %.cpp,$(DEBUG_OUT_DIR)%.o,$(SOURCES))
	@echo
	@echo ==== Linking the debug object files ====
	$(LINKER) $(DEBUG_LINK_FLAGS) -o $@ $(DEBUG_OUT_DIR)*.o $(LIBS_DIRS) $(LIB_DEPS)

$(TESTS_OUT_DIR)$(TARGET) : $(patsubst %.cpp,$(TESTS_OUT_DIR)%.o,$(TEST_SOURCES))
	@echo
	@echo ==== Linking the unit test object files ====
	$(LINKER) $(TEST_LINK_FLAGS) -o $@ $(TESTS_OUT_DIR)*.o $(LIBS_DIRS) $(TEST_LIB_DEPS)

# Commands for compiling each unit. It displays the name of that unit.
define compileSrcForRelease =
	@echo
	@echo ---- Compiling release \'$(notdir $(basename $<))\' ----
	$(COMPILER) $(RELEASE_COMPILE_FLAGS) $(INCLUDES) -o $@ $<
	@mv -f $(RELEASE_DEPDIR)/$*.Td $(RELEASE_DEPDIR)/$*.d && touch $@
endef

define compileSrcForDebug =
	@echo
	@echo ---- Compiling debug \'$(notdir $(basename $<))\' ----
	$(COMPILER) $(DEBUG_COMPILE_FLAGS) $(INCLUDES) -o $@ $<
	@mv -f $(DEBUG_DEPDIR)/$*.Td $(DEBUG_DEPDIR)/$*.d && touch $@
endef

define compileTestSrc =
	@echo
	@echo ---- Compiling test \'$(notdir $(basename $<))\' ----
	$(COMPILER) $(TEST_COMPILE_FLAGS) $(INCLUDES) -o $@ $<
	@mv -f $(TEST_DEPDIR)/$*.Td $(TEST_DEPDIR)/$*.d && touch $@
endef

$(RELEASE_OUT_DIR)%.o : $(SRC_DIR)%.cpp
$(RELEASE_OUT_DIR)%.o : $(SRC_DIR)%.cpp $(RELEASE_DEPDIR)/%.d
	$(compileSrcForRelease)

$(DEBUG_OUT_DIR)%.o : $(SRC_DIR)%.cpp
$(DEBUG_OUT_DIR)%.o : $(SRC_DIR)%.cpp $(DEBUG_DEPDIR)/%.d
	$(compileSrcForDebug)

$(TESTS_OUT_DIR)%.o : $(SRC_DIR)%.cpp
$(TESTS_OUT_DIR)%.o : $(SRC_DIR)%.cpp $(TEST_DEPDIR)/%.d
	$(compileTestSrc)
$(TESTS_OUT_DIR)%.o : $(TEST_DIR)%.cpp
$(TESTS_OUT_DIR)%.o : $(TEST_DIR)%.cpp $(TEST_DEPDIR)/%.d
	$(compileTestSrc)

$(RELEASE_DEPDIR)/%.d: ;
$(DEBUG_DEPDIR)/%.d: ;
$(TEST_DEPDIR)/%.d: ;
.PRECIOUS: $(RELEASE_DEPDIR)/%.d $(DEBUG_DEPDIR)/%.d $(TEST_DEPDIR)/%.d

include $(wildcard $(patsubst %,$(RELEASE_DEPDIR)/%.d,$(basename $(SOURCES))))
include $(wildcard $(patsubst %,$(DEBUG_DEPDIR)/%.d,$(basename $(SOURCES))))
include $(wildcard $(patsubst %,$(TEST_DEPDIR)/%.d,$(basename $(TEST_SOURCES))))

.PHONY : clean
clean :
	rm -f $(RELEASE_OUT_DIR)*.exe $(RELEASE_OUT_DIR)*.o
	rm -f $(DEBUG_OUT_DIR)*.exe $(DEBUG_OUT_DIR)*.o
	rm -f $(TESTS_OUT_DIR)*.exe $(TESTS_OUT_DIR)*.o
