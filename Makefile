# Note: $(MAKE) from below can be the default 'make' or 'gmake' or 'remake' or
# other make program.
# 'gmake' was used in FreeBSD, while 'remake' was the choice in MacOS.

# Usage:
#
# 	$(MAKE) [release] 		compiles the release, without the tests
# 	$(MAKE) debug 			compiles the debug version, without the tests
# 	$(MAKE) tests 			compiles the debug version of the unit tests
# 	$(MAKE) all 			compiles release, debug and debug for the tests
# 	$(MAKE) clean_RELEASE	removes generated contents from release folders + pch
# 	$(MAKE) clean_DEBUG		removes generated contents from debug folders + pch
# 	$(MAKE) clean_TESTS		removes generated contents from tests folders + pch
# 	$(MAKE) clean_EXE		removes generated executables from the release, debug and tests folders
# 	$(MAKE) clean_OBJ		removes generated object files from the release, debug and tests folders
# 	$(MAKE) clean_DEPS		removes generated dependency files from .r, .d and .td folders
# 	$(MAKE) clean_PCH		removes generated precompiled headers from src/precompiled.h.gch/
# 	$(MAKE) clean_ALL		removes generated contents from release, debug, tests, dependency and pch folders
#	$(MAKE) boost_libs_available	ensures the required Boost libs are available/generated
#	$(MAKE) show_compiler_info		displays compiler name, version and c++ standard

# For processing the targets in parallel, call it like:
#  $(MAKE) -j[<x>] -Otarget [<t>]
# 	with <x> replaced by the number of threads to use
# 	and <t> either not provided or replaced by one of release, debug, tests or all

# CXX=.. and CPP_STANDARD=.. can be specified after $(MAKE) to use a different 
# C++ compiler or an older C++ standard

# This Makefile uses the technique described in the following article:
# http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/

SHELL != which bash

# Compiler: g++[[-]VersionMajor] or clang++[[-]VersionMajor]
CXX := g++

# Linker should be the same as the compiler, thus ignoring any CC from command line
override CC = $(CXX)

# clang++ and g++ have same suffix (g++)
private __OMIT_CLAN := $(CC:clan%=%)
private __AFTER_GPLUSPLUS := $(strip $(__OMIT_CLAN:g++%=%))
CC_TYPE := $(CC:%$(__AFTER_GPLUSPLUS)=%)# clang++ or g++ or empty

ifneq ($(CC_TYPE:clan%=%),g++)
 $(error Wrong CXX make parameter ($(CXX))! Expecting CXX=[clan]g++[[-]VersionMajor].)
endif

CC_VERSION != $(CC) --version | head -n 1
ifeq (,$(CC_VERSION))
 $(error Cannot execute '$(CC) --version'!)
endif

# Finding the latest C++ language standard available for the given compiler.
# Clang++ will provide this information while trying to compile a dummy empty file
# for an invalid value for '-std' as hint[s] about the possible valid values.
# G++ offers the information while launched with "-v --help" parameters.
ifeq (clang++,$(CC_TYPE))
CPP_STANDARD != echo "" \
	| clang++ -c -std=c++latest -x c++ - 2>&1 \
	| grep -oEe "c\+\+2[0-9a-z]" \
	| sort -r \
	| head -n1
else
CPP_STANDARD != g++ -v --help 2>/dev/null \
	| grep -e "-std=c++2" \
	| grep -oEe "c\+\+2[0-9a-z]" \
	| sort -r \
	| head -n1
endif

CPP_STANDARD_TEST != echo "" \
	| $(CC) -c -std=$(CPP_STANDARD) -x c++ - -o /dev/null 2>&1 \
	| grep -o error

ifeq ($(CPP_STANDARD_TEST),error)
 $(error Invalid provided/determined C++ standard for $(CC): $(CPP_STANDARD)!)
endif

# Determine OS(Linux or WSL/MSYS2/Cygwin or Android or FreeBSD or MacOS) name
# and whether it is arm, 32 or 64bit

# Operating system: Msys, Cygwin, GNU/Linux, FreeBSD, Darwin or Android
UNAME_O != uname -o

# Detecting WSL
# https://superuser.com/questions/1749781/how-can-i-check-if-the-environment-is-wsl-from-a-shell-script
# Considering kernel release: ...WSL... only for WSL; Otherwise some numbers
UNAME_R != uname -r

# Machine:
# - 64 bits: x86_64, amd64
# - 32 bits: i<n>86 (n<=6)
# - arm: armv<n>..
UNAME_M != uname -m

# Prefix __BASE_OUT_DIR by 'x' to denote executables (like Visual Studio does)
ifeq (armv,$(findstring armv,$(UNAME_M)))
	BITNESS := # None for arm
	__BASE_OUT_DIR := x_arm
	ARCH_LABEL := arm

else
	BITNESS != \
		(echo $(UNAME_M) | grep -q "64" && echo 64) \
		|| echo 32
	__BASE_OUT_DIR := x$(BITNESS)
	ARCH_LABEL := $(BITNESS)bit
endif

# Remove targets updated, but for recipes that eventually fail.
# Ensures the targets are not considered up to date despite a failure.
.DELETE_ON_ERROR:

# No matter the contents of the include below, the default goal is guaranteed
.DEFAULT_GOAL := release

.PHONY : show_compiler_info boost_libs_available \
	release debug tests all \
	clean_RELEASE clean_DEBUG clean_TESTS clean_ALL \
	clean_EXE clean_OBJ clean_DEPS

show_compiler_info :
	@echo -e "\n=====================" ;\
	echo -n "Using $(CPP_STANDARD) standard from compiler $(CC_VERSION) " ;\
	echo "for a $(UNAME_M) machine!" ;\
	echo -n "If there appear errors within standard headers (like 'unordered_map'), " ;\
	echo "retry providing the CPP_STANDARD=<next lower standard> parameter to make!" ;\
	echo -e "=====================\n"

# Source folders
SRC_DIR := src/
TESTS_DIR := test/

# Precompiled files
PCH_GENERATED_FROM := $(SRC_DIR)precompiled.h
PCH_STORE_FOLDER := $(PCH_GENERATED_FROM).gch

$(shell mkdir -p $(PCH_STORE_FOLDER) 2>/dev/null)

# The C++ sources of the RiverCrossing project
SOURCES := $(filter-out precompiledHeaderGenerator.cpp,\
	$(notdir $(wildcard $(SRC_DIR)*.cpp)))
TESTS_SOURCES := $(SOURCES) $(notdir $(wildcard $(TESTS_DIR)*.cpp))

# Folders containing dependency files
RELEASE_DEPDIR := .r
DEBUG_DEPDIR := .d
TESTS_DEPDIR := .td

$(shell mkdir -p $(RELEASE_DEPDIR) 2>/dev/null)
$(shell mkdir -p $(DEBUG_DEPDIR) 2>/dev/null)
$(shell mkdir -p $(TESTS_DEPDIR) 2>/dev/null)

# Folders for the generated files (objects and executables)
BASE_OUT_DIR := $(__BASE_OUT_DIR)/$(CC_TYPE)/
RELEASE_OUT_DIR := $(BASE_OUT_DIR)Release/
DEBUG_OUT_DIR := $(BASE_OUT_DIR)Debug/
TESTS_OUT_DIR := $(BASE_OUT_DIR)tests/

$(shell mkdir -p $(RELEASE_OUT_DIR) 2>/dev/null)
$(shell mkdir -p $(DEBUG_OUT_DIR) 2>/dev/null)
$(shell mkdir -p $(TESTS_OUT_DIR) 2>/dev/null)

# Flags for stripping the release executable. macOS strip command uses different flags
STRIP_FLAGS := -s

# Append -DH_PRECOMPILED to check for missing includes within individual cpp-s.
# Remove -DH_PRECOMPILED after correcting the includes from each cpp with issues.
# Similarly, -Winvalid-pch explains why pch files did not match within PCH_STORE_FOLDER.
# Also, -H shows the tried pch-s and which did match plus all included headers.
# -march=native [-mtune=...] might generate some additional speed-up,
# but limits which machines can run the binary.
# -flto is supported for .so libraries, but not .dll (thus not by MSYS2/Cygwin).
# -flto was not supported by some versions of clang in Linux/WSL, as it needs
# the LLVMgold.so plugin which was missing.
# There is a clang warning to replace '-Ofast' by '-O3 -ffast-math'.
# However, -ffast-math produces errors in some clang versions around NaN values,
# thus an extra flag '-fhonor-nans' is required.
# Gcc 8 and earlier copes with NaN-s using '-fno-finite-math-only'
# Clang can use its own implementation of the standard library: '-stdlib=libc++',
# but then Boost UnitTest has to be built using libc++ in Linux
# Clang version of the standard library has parts depending on '-fexperimental-library'.
CXX_FLAGS := \
	-std=$(CPP_STANDARD) -march=native -Ofast -fno-finite-math-only \
	-Wall -Wextra -flto=auto

TARGET_EXT :=# No extension except for Windows executables (MSYS2/Cygwin)

ifeq (Msys,$(UNAME_O))
 CURRENT_OS := MSYS2
 TARGET_EXT := .exe # MSYS2 executes .exe files

 ifeq (clang++,$(CC_TYPE))
  CXX_FLAGS := \
  		-stdlib=libc++ \
		$(subst -Ofast -fno-finite-math-only,-O3 -ffast-math -fhonor-nans,$(CXX_FLAGS)) \
		-fuse-ld=lld -fexperimental-library
 endif

else ifeq (Cygwin,$(UNAME_O))
 CURRENT_OS := Cygwin
 TARGET_EXT := .exe # Cygwin executes .exe files

 ifeq (clang++,$(CC_TYPE))
  CXX_FLAGS := \
  		-stdlib=libc++ \
		$(subst -Ofast -fno-finite-math-only,-O3 -ffast-math -fhonor-nans,$(CXX_FLAGS)) \
		-fuse-ld=lld -fexperimental-library
 endif

else ifeq (WSL,$(findstring WSL,$(UNAME_R)))
 CURRENT_OS := WSL

 ifeq (clang++,$(CC_TYPE))
  CXX_FLAGS := \
  		-stdlib=libc++ \
		$(subst -Ofast -fno-finite-math-only,-O3 -ffast-math -fhonor-nans,$(CXX_FLAGS)) \
  		-fexperimental-library
 endif

else ifeq (FreeBSD,$(UNAME_O))
 CURRENT_OS := FreeBSD

 ifeq (clang++,$(CC_TYPE))
  CXX_FLAGS := \
  		$(subst -Ofast -fno-finite-math-only,-O3 -ffast-math -fhonor-nans,$(CXX_FLAGS)) \
		-fexperimental-library
 
 else # GCC
  # Gcc from FreeBSD has problems using precompiled headers
  CXX_FLAGS := -stdlib=libstdc++ $(CXX_FLAGS) -DH_PRECOMPILED
 endif

else ifeq (Darwin,$(UNAME_O))
 CURRENT_OS := MacOS

 # MacOS linking should use only shared, not static libs, as pointed by:
 # https://coderedirect.com/questions/659993/statically-linked-libraries-on-mac
 
 # Mac has a different set of flags for strip
 STRIP_FLAGS :=# None OR '-u -r'

 ifeq (g++,$(CC_TYPE))
  CXX_FLAGS := -stdlib=libstdc++ $(CXX_FLAGS)
 
 else # Clang
  CXX_FLAGS := \
  		$(subst -Ofast -fno-finite-math-only,-O3 -ffast-math -fhonor-nans,$(CXX_FLAGS)) \
		-fexperimental-library
 endif

else ifeq (Android,$(UNAME_O))
 CURRENT_OS := Android

 # Termux from Android has Clang as default compiler and gcc is a link to clang.
 # It uses already 'libc++', no need to require it.
 # The experimental library appears to be unavailable,
 # thus '-fexperimental-library' was not appended.
 ifeq (clang++,$(CC_TYPE))
  CXX_FLAGS := \
		$(subst -Ofast -fno-finite-math-only,-O3 -ffast-math -fhonor-nans,$(CXX_FLAGS))
 endif

else ifeq (GNU/Linux,$(UNAME_O))
 CURRENT_OS := Linux

 ifeq (clang++,$(CC_TYPE))
  CXX_FLAGS := \
  		-stdlib=libc++ \
		$(subst -Ofast -fno-finite-math-only,-O3 -ffast-math -fhonor-nans,$(CXX_FLAGS)) \
  		-fexperimental-library
 endif

else
 UNAME_A != uname -a
 $(error OS not handled! The answer to 'uname -a' is: $(UNAME_A))
endif

# The included file below must define the path: INCLUDE_DIR_GSL
include GSL_Dirs$(CURRENT_OS).mk

ifeq (,$(INCLUDE_DIR_GSL))
 $(error GSL include folder not set!)
endif

ifeq (,$(wildcard $(INCLUDE_DIR_GSL)gsl))
 $(error GSL include folder ($(INCLUDE_DIR_GSL)) must contain a 'gsl' subfolder!)
endif

BOOST_LIB_ENDING_RELEASE :=# None, except for MSVC and MSYS2
BOOST_LIB_ENDING_DEBUG :=# None, except for MSVC and MSYS2
REQUIRED_BOOST_LIBS :=# None, except for the 'tests' build configuration
REQUIRED_BOOST_LIBS_TESTING := boost_unit_test_framework
ALL_REQUIRED_BOOST_LIBS := $(REQUIRED_BOOST_LIBS) $(REQUIRED_BOOST_LIBS_TESTING)
EXTRA_LIBS :=#None. Example exception: MSYS2 MinGW with gcc8 requires 'stdc++fs'

# The included file below must define 2 paths: INCLUDE_DIR_BOOST and LIB_DIR_BOOST.
# It also has to define 'boost_libs_available' target which ensures that
# ALL_REQUIRED_BOOST_LIBS are created/available.
# When necessary, it has to update BOOST_LIB_ENDING_DEBUG|RELEASE to somethink like
# '-mgw14-mt[-d]-x64-1_87' (for MSYS2 MinGW with gcc-14, multithreading[, debug], x64, Boost 1.87)
include BoostDirs$(CURRENT_OS).mk

ifeq (,$(INCLUDE_DIR_BOOST))
 $(error Boost include folder not set!)
endif
ifeq (,$(LIB_DIR_BOOST))
 $(error Boost lib folder not set!)
endif

ifeq (,$(wildcard $(INCLUDE_DIR_BOOST)boost))
 $(error Boost include folder ($(INCLUDE_DIR_BOOST)) must contain a 'boost' subfolder!)
endif

PCH_PREFIX := $(PCH_STORE_FOLDER)/$(CURRENT_OS)_$(ARCH_LABEL)_$(CC_TYPE)_
PCH_RELEASE := $(PCH_PREFIX)Release
PCH_DEBUG := $(PCH_PREFIX)Debug
PCH_TESTS := $(PCH_PREFIX)tests

# Gcc uses implicitly the pch 'X.h.gch' found in the same folder as the 'X.h' or
# within 'X.h.gch' folder. Thus, nothing to do for Gcc.
USE_PCH_RELEASE :=
USE_PCH_DEBUG :=
USE_PCH_TESTS :=

# Clang however, needs an explicit request to use the pch, not the header:
ifeq ($(CC_TYPE),clang++)
CLANG_INCL_PCH := -include-pch
USE_PCH_RELEASE := $(CLANG_INCL_PCH) $(PCH_RELEASE)
USE_PCH_DEBUG := $(CLANG_INCL_PCH) $(PCH_DEBUG)
USE_PCH_TESTS := $(CLANG_INCL_PCH) $(PCH_TESTS)
endif

PROJECT_INCLUDE_DIRS := "$(SRC_DIR)" "$(TESTS_DIR)"
FOREIGN_INCLUDE_DIRS := \
	"$(INCLUDE_DIR_BOOST)" \
	"$(INCLUDE_DIR_GSL)"

# idirafter and isystem can apply to any include, but they mark those as system,
# thus no warnings are generated from such includes.
# idirafter ensures the actual system includes are searched before idirafter ones.
# Using isystem for default Boost headers installation in
# /usr/include (which is already a system include folder)
# generates file not found error for standard headers like <stdlib.h>,
# while idirafter does the job.
# iquote applies only to #include "".
INCLUDES := \
	$(PROJECT_INCLUDE_DIRS:%=-iquote %) \
	$(FOREIGN_INCLUDE_DIRS:%=-idirafter %)

REQUIRED_NON_STD_LIBS_RELEASE := \
	$(REQUIRED_BOOST_LIBS:%=%$(BOOST_LIB_ENDING_RELEASE))
REQUIRED_LIBS_RELEASE := \
	$(REQUIRED_NON_STD_LIBS_RELEASE) \
	$(EXTRA_LIBS)
LIB_DEPS_RELEASE := $(REQUIRED_LIBS_RELEASE:%=-l%)
NON_STD_LIB_DEPS_RELEASE := $(REQUIRED_NON_STD_LIBS_RELEASE:%=-l%)

REQUIRED_NON_STD_LIBS_DEBUG := \
	$(REQUIRED_BOOST_LIBS:%=%$(BOOST_LIB_ENDING_DEBUG))
REQUIRED_LIBS_DEBUG := \
	$(REQUIRED_NON_STD_LIBS_DEBUG) \
	$(EXTRA_LIBS)
LIB_DEPS_DEBUG := $(REQUIRED_LIBS_DEBUG:%=-l%)
NON_STD_LIB_DEPS_DEBUG := $(REQUIRED_NON_STD_LIBS_DEBUG:%=-l%)

# Testing needs Boost test library additionally to those required by Release/Debug
REQUIRED_NON_STD_LIBS_TESTING := \
	$(REQUIRED_BOOST_LIBS_TESTING:%=%$(BOOST_LIB_ENDING_DEBUG)) \
	$(REQUIRED_BOOST_LIBS:%=%$(BOOST_LIB_ENDING_DEBUG))
REQUIRED_LIBS_TESTING := \
	$(REQUIRED_NON_STD_LIBS_TESTING) \
	$(EXTRA_LIBS)
LIB_DEPS_TESTS := $(REQUIRED_LIBS_TESTING:%=-l%)
NON_STD_LIB_DEPS_TESTS := $(REQUIRED_NON_STD_LIBS_TESTING:%=-l%)

LIB_DIR_BOOST_NO_TRAILING_SLASH := $(LIB_DIR_BOOST:%/=%)

# Whenever the required libraries are not within the expected folders,
# the generated executable needs to be launched within an environment indicating
# where else to look for libraries.
# PATH (for MSYS2/Cygwin environments), DYLD_LIBRARY_PATH for MacOS
# and LD_LIBRARY_PATH for the rest are the relevant environment variables to prepend/append:
# <libPath>=ColonSeparatedPathsForFoldersOfUnusualLibs:$<libPath> <generatedExe>
# This becomes the <rpath> where to look first for libraries.
# MSYS2/Cygwin environments cannot use the <rpath> mechanism
# and still need an updated environment before launching
COMMON_LINK_FLAGS := $(CXX_FLAGS) -Wl,-rpath,"$(LIB_DIR_BOOST_NO_TRAILING_SLASH)"
RELEASE_LINK_FLAGS := $(COMMON_LINK_FLAGS) -L"$(LIB_DIR_BOOST_NO_TRAILING_SLASH)"
DEBUG_LINK_FLAGS := $(COMMON_LINK_FLAGS) -L"$(LIB_DIR_BOOST_NO_TRAILING_SLASH)"
TEST_LINK_FLAGS := $(COMMON_LINK_FLAGS) -L"$(LIB_DIR_BOOST_NO_TRAILING_SLASH)"

BYPASSED_WARNINGS := \
	-Wno-missing-declarations \
	-Wno-unknown-pragmas \
	-Wno-reorder

# Keep these as recursive variables, to defer substitution of $@ and $*.
# Using initially a temporary file for the generated dependency file.
# See why in the comment for compileSrcForRelease from below
COMMON_DEP_FLAGS = -MT $@ -MMD -MP -MF
RELEASE_DEPFLAGS = $(COMMON_DEP_FLAGS) $(RELEASE_DEPDIR)/$*.Td
DEBUG_DEPFLAGS = $(COMMON_DEP_FLAGS) $(DEBUG_DEPDIR)/$*.Td
TEST_DEPFLAGS = $(COMMON_DEP_FLAGS) $(TESTS_DEPDIR)/$*.Td

RELEASE_PCH_COMPILE_FLAGS = -c $(CXX_FLAGS) \
	$(BYPASSED_WARNINGS) \
	-DNDEBUG
RELEASE_COMPILE_FLAGS = $(RELEASE_PCH_COMPILE_FLAGS) $(RELEASE_DEPFLAGS)

DEBUG_PCH_COMPILE_FLAGS = -c -g $(CXX_FLAGS) \
	$(BYPASSED_WARNINGS)
DEBUG_COMPILE_FLAGS = $(DEBUG_PCH_COMPILE_FLAGS) $(DEBUG_DEPFLAGS)

TEST_PCH_COMPILE_FLAGS = -c -g $(CXX_FLAGS) \
	$(BYPASSED_WARNINGS) \
	-DUNIT_TESTING \
	-DBOOST_TEST_DYN_LINK
TEST_COMPILE_FLAGS = $(TEST_PCH_COMPILE_FLAGS) $(TEST_DEPFLAGS)

TARGET := RiverCrossing$(TARGET_EXT)

# Before running these targets make sure the required boost libs are available.
# Since these targets might involve compiling/linking, display also the compiler information.
# However, don't relink the executables unless boost_libs_available changes
# relevant libraries
debug release tests : | boost_libs_available show_compiler_info

# release depends on the generation of the release target without the unit tests
release : $(RELEASE_OUT_DIR)$(TARGET)

# debug depends on the generation of the debug target without the unit tests
debug : $(DEBUG_OUT_DIR)$(TARGET)

# tests depends on the generation of the unit tests target
tests : $(TESTS_OUT_DIR)$(TARGET)

all : debug release tests

# Expecting only '.so', '.dylib' or '.dll' shared libraries.
.LIBPATTERNS = lib%.so lib%.dylib lib%.dll %.dll cyg%.dll

# Allows locating prerequisite Boost libraries provided in the '-lboost_X' form
# for the link targets below, such as $(NON_STD_LIB_DEPS_RELEASE|DEBUG|TESTS).
# The search will locate then the libs $(LIB_DIR_BOOST)[lib|cyg]boost_X.(so|dylib|dll).
# [lib|cyg]*.(so|dylib|dll) are checked based on .LIBPATTERNS variable
VPATH := $(VPATH):$(LIB_DIR_BOOST_NO_TRAILING_SLASH)

# .ONESHELL: plus .SHELLFLAGS = -e technique reduces the choices about which
# commands to display and which not

# The targets depend on the `o` files from RELEASE|DEBUG|TESTS_OUT_DIR
# from the corresponding `cpp` among [TESTS_]SOURCES.
# RELEASE|DEBUG|TESTS_LINK_FLAGS contain the '-L"lib_dir"' information.
# Relinking is required also for newly compiled Boost libraries.
$(RELEASE_OUT_DIR)$(TARGET) :\
		$(SOURCES:%.cpp=$(RELEASE_OUT_DIR)%.o) \
		$(NON_STD_LIB_DEPS_RELEASE)
	@echo; \
	echo ==== Linking the release object files ====
	$(CC) $(RELEASE_LINK_FLAGS) -o $@ \
		$(RELEASE_OUT_DIR)*.o $(LIB_DEPS_RELEASE) && \
	strip $(STRIP_FLAGS) $@

$(DEBUG_OUT_DIR)$(TARGET) :\
		$(SOURCES:%.cpp=$(DEBUG_OUT_DIR)%.o) \
		$(NON_STD_LIB_DEPS_DEBUG)
	@echo; \
	echo ==== Linking the debug object files ====
	$(CC) $(DEBUG_LINK_FLAGS) -o $@ \
		$(DEBUG_OUT_DIR)*.o $(LIB_DEPS_DEBUG)

$(TESTS_OUT_DIR)$(TARGET) :\
		$(TESTS_SOURCES:%.cpp=$(TESTS_OUT_DIR)%.o) \
		$(NON_STD_LIB_DEPS_TESTS)
	@echo; \
	echo ==== Linking the unit test object files ====
	$(CC) $(TEST_LINK_FLAGS) -o $@ \
		$(TESTS_OUT_DIR)*.o $(LIB_DEPS_TESTS)

# Commands for compiling each *.cpp ($< - the first prerequisite, ignoring *.d).
# The dependency file generated during compilation needs to be older than the
# generated object file, as the %.o:%.d rules from below state.
# It can also be corrupt if the make is interrupted while generating it.
# Solution:
# 1. generate the dep to a temporary file during compilation
# 2. after a successful compilation, rename the temporary as the dependency file
# to be used and set the object file as older than the dependency file.
# The commands use deferred expansion (=) because of $@, $< and $*
define compileSrcForRelease =
	@echo; \
	echo ---- Compiling release \'$*\' ----
	$(CXX) $(RELEASE_COMPILE_FLAGS) $(INCLUDES) $(USE_PCH_RELEASE) -o $@ $< && \
	mv -f $(RELEASE_DEPDIR)/$*.Td $(RELEASE_DEPDIR)/$*.d && \
	touch $@
endef

define compileSrcForDebug =
	@echo; \
	echo ---- Compiling debug \'$*\' ----
	$(CXX) $(DEBUG_COMPILE_FLAGS) $(INCLUDES) $(USE_PCH_DEBUG) -o $@ $< && \
	mv -f $(DEBUG_DEPDIR)/$*.Td $(DEBUG_DEPDIR)/$*.d && \
	touch $@
endef

define compileTestSrc =
	@echo; \
	echo ---- Compiling test \'$*\' ----
	$(CXX) $(TEST_COMPILE_FLAGS) $(INCLUDES) $(USE_PCH_TESTS) -o $@ $< && \
	mv -f $(TESTS_DEPDIR)/$*.Td $(TESTS_DEPDIR)/$*.d && \
	touch $@
endef

$(RELEASE_OUT_DIR)%.o : $(SRC_DIR)%.cpp $(RELEASE_DEPDIR)/%.d $(PCH_RELEASE)
	$(compileSrcForRelease)

$(DEBUG_OUT_DIR)%.o : $(SRC_DIR)%.cpp $(DEBUG_DEPDIR)/%.d $(PCH_DEBUG)
	$(compileSrcForDebug)

# The tests need sources from 'src' and 'test' and a file appears in only 1 of those folders
# That's why there are 2 rules, one for 'src' and another one for 'test'
$(TESTS_OUT_DIR)%.o : $(SRC_DIR)%.cpp $(TESTS_DEPDIR)/%.d $(PCH_TESTS)
	$(compileTestSrc)
$(TESTS_OUT_DIR)%.o : $(TESTS_DIR)%.cpp $(TESTS_DEPDIR)/%.d $(PCH_TESTS)
	$(compileTestSrc)

# Generating PCH files except for FreeBSD & g++, which don't support them yet
$(PCH_RELEASE) : $(PCH_GENERATED_FROM)
ifneq (FreeBSDg++,$(CURRENT_OS)$(CC_TYPE))
	@echo; \
	echo ---- Compiling release pch file \'$@\' ----
	$(CXX) $(RELEASE_PCH_COMPILE_FLAGS) $(INCLUDES) -o $@ -x c++-header $<
endif

$(PCH_DEBUG) : $(PCH_GENERATED_FROM)
ifneq (FreeBSDg++,$(CURRENT_OS)$(CC_TYPE))
	@echo; \
	echo ---- Compiling debug pch file \'$@\' ----
	$(CXX) $(DEBUG_PCH_COMPILE_FLAGS) $(INCLUDES) -o $@ -x c++-header $<
endif

$(PCH_TESTS) : $(PCH_GENERATED_FROM)
ifneq (FreeBSDg++,$(CURRENT_OS)$(CC_TYPE))
	@echo; \
	echo ---- Compiling tests pch file \'$@\' ----
	$(CXX) $(TEST_PCH_COMPILE_FLAGS) $(INCLUDES) -o $@ -x c++-header $<
endif

# Dependency files targets must force .o recompilation:
# - if they are missing, compiling .o generates also the corresponding .d
# - if they are created/changed (based on .o recompilation or not),
#   the recompilation of the .o ensures the .o is stamped as newer than the .d,
#   to prevent an infinite loop of updating .o and .d as they get alternatively
#   updated by/due to the other
$(RELEASE_DEPDIR)/%.d : ;
$(DEBUG_DEPDIR)/%.d : ;
$(TESTS_DEPDIR)/%.d : ;

# Adding the generated additional prerequisites for the rules for compiling *.o
include $(SOURCES:%.cpp=$(RELEASE_DEPDIR)/%.d $(DEBUG_DEPDIR)/%.d)
include $(TESTS_SOURCES:%.cpp=$(TESTS_DEPDIR)/%.d)

clean_RELEASE clean_DEBUG clean_TESTS : clean_% :
	rm -f $($*_OUT_DIR)$(TARGET) $($*_OUT_DIR)*.o $($*_DEPDIR)/*.*d $(PCH_$*)

clean_EXE :
	rm -f $(RELEASE_OUT_DIR)$(TARGET) \
		$(DEBUG_OUT_DIR)$(TARGET) \
		$(TESTS_OUT_DIR)$(TARGET)

clean_OBJ :
	rm -f $(RELEASE_OUT_DIR)*.o \
		$(DEBUG_OUT_DIR)*.o \
		$(TESTS_OUT_DIR)*.o

clean_DEPS :
	rm -f $(RELEASE_DEPDIR)/*.*d \
		$(DEBUG_DEPDIR)/*.*d \
		$(TESTS_DEPDIR)/*.*d

clean_PCH :
	rm -f $(PCH_RELEASE) \
		$(PCH_DEBUG) \
		$(PCH_TESTS)

# clean_ALL performs also clean_DEPS and clean_PCH
clean_ALL : clean_RELEASE clean_DEBUG clean_TESTS
