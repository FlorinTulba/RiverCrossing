# Note: $(MAKE) from below can be the default 'make' or 'gmake' or 'remake' or
# other make program.
# 'gmake' was used in FreeBSD, while 'remake' was the choice in MacOS.
#
# Target list:
#
# $(MAKE) help_and_release same as "$(MAKE) help release" (DEFAULT GOAL)
# $(MAKE) help          displays the list of available goals
# $(MAKE) release       builds the release, without the tests
# $(MAKE) debug         builds the debug version, without the tests
# $(MAKE) tests         builds the tests (debug version)
# $(MAKE) all           builds release, debug and the tests
# $(MAKE) run_TESTS     builds and executes the tests
# $(MAKE) clean_RELEASE removes generated contents from release folders + pch
# $(MAKE) clean_DEBUG   removes generated contents from debug folders + pch
# $(MAKE) clean_TESTS   removes generated contents from tests folders + pch
# $(MAKE) clean_EXE     removes generated executables from the release, debug and tests folders
# $(MAKE) clean_OBJ     removes generated object files from the release, debug and tests folders
# $(MAKE) clean_DEPS    removes generated dependency files from .deps/(RELEASE|DEBUG|TESTS) folders
# $(MAKE) clean_PCH     removes generated precompiled headers from src/precompiled.h.gch/
# $(MAKE) clean_ALL     removes generated contents from release, debug, tests, dependency and pch folders
# $(MAKE) boost_libs_available ensures the required Boost libs are available/generated
# $(MAKE) show_compiler_info   displays compiler name, version and c++ standard
#
# When adding new targets:
# - add a line to the list above with the format:
#         "# $(MAKE) <target_name> <description>"
#     It will be picked and displayed by the 'help[_and_<default_target>]' targets.
#     The corresponding displayed line will replicate the spacing from the original line.
# - insert it below within one of the 3 lists:
#     DEPFILES_AVERSE_GOALS, DEPFILES_NEUTRAL_GOALS or GOALS_NEEDING_DEPFILES
#
# The default goal is designed on purpose to display also the available targets.
#
# For processing the targets in parallel, call it like:
#  "$(MAKE) [-rR] -j[<x>] -O([target]|line|none) <t>*"
# 	with <x> replaced by the number of threads to use
# 	and <t>* replaced by 0 or several targets from above
#
# CXX=.. and CPP_STANDARD=.. can be specified after $(MAKE) to use a different
# C++ compiler or an older C++ standard
#
# This Makefile uses the technique described in the following article:
#   http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/
# The auxilliary dependency files it creates/uses are in conflict with several
# clean_* targets.
# This is the reason why all targets must be classified based on the relation with
# these dependency files (DEPFILES) located in dependency folders (DEPDIR).

# Helpers for empty values and spaces
Empty :=# Used to emphasize empty values or to delimit tricky syntax matters
Space := $(Empty) $(Empty)

# Comma for variables with ',' which need to be passed as parameters
# Instead of passing a,b,c - provide a$(Comma)b$(Comma)c
Comma := ,

# In order to be able to use relative paths inside the Makefile,
# the current working directory must be (set as) the one containing this Makefile.
MAKEFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
ifneq ($(CURDIR)/,$(MAKEFILE_DIR))
  $(error Launch $(MAKE) from '$(MAKEFILE_DIR)'\
          or use '$(MAKE) -C $(MAKEFILE_DIR)'!)
endif

PROJECT_NAME := $(notdir $(CURDIR))

# Not using '.ONESHELL:' in this Makefile

# Don't keep partially created targets
.DELETE_ON_ERROR:

help_and_release: help release
.DEFAULT_GOAL := help_and_release

# Targets that delete the dependency files from .deps/(RELEASE|DEBUG|TESTS) folders.
# When any of these goals are specified in the make command,
# make must not include '*.d' files from those folders.
# Including and then deleting them changes make's internal map of the world
# and might produce errors, especially when launching make in parallel.
DEPFILES_AVERSE_GOALS :=\
  clean_DEPS clean_RELEASE clean_DEBUG clean_TESTS clean_ALL

# Targets that don't need to include .deps/(RELEASE|DEBUG|TESTS)/*.d dependency files.
# The targets for the *.pch files are appended later in the file.
DEPFILES_NEUTRAL_GOALS :=\
  help show_compiler_info boost_libs_available\
  clean_EXE clean_OBJ clean_PCH

# Targets that need to include .deps/(RELEASE|DEBUG|TESTS)/*.d dependency files.
# The targets for the *.o files and the executables are appended later in the file.
GOALS_NEEDING_DEPFILES :=\
  release help_and_release debug tests all run_TESTS

# Goals added so far to these 3 lists are .PHONY. Later additions to the lists are actual files.
.PHONY: $(DEPFILES_AVERSE_GOALS) $(DEPFILES_NEUTRAL_GOALS) $(GOALS_NEEDING_DEPFILES)

SHELL := $(shell command -v bash)
ifeq ($(Empty),$(SHELL))
  $(error Bash shell must be installed for running this Makefile.)
endif

# Variable used inside printf to produce sound notifications.
# 'printf "$(Bell)"' could announce the completion of a task or finding a certain error
Bell := \a

# When running '$(MAKE) -j -O(target|line) ...', long tasks might display their
# output & errors only when the whole recipe/line finishes.
# Redirecting stdout & stderr to /dev/tty (when available) for those tasks only
# lets them display their output immediately, while the other tasks' output is
# still batched.
# Usage: '$(REALLY_SLOW_CMD) $(TTY_REDIR)'
# Note:
# For a command 'echo "x"; $(REALLY_SLOW_CMD) $(TTY_REDIR)' the output will be:
# - 'x', then <output&errors from REALLY_SLOW_CMD>
#        for '-j -O(none|recurse)' or for no '-j' (sequential run)
# - <output&errors from REALLY_SLOW_CMD>, then 'x' for '-j -O([target]|line)'
TTY_REDIR := $(shell [ -c /dev/tty ] && echo "> /dev/tty 2>&1" || echo "")

# Space and non-space regex matching
SpRE := [ \t]
NonSpRE := [^ \t]

# Comments start like: "#"
__COMMENT_PATTERN := ^$(SpRE)*\#

# Comments with described goals start like: "# $(MAKE)"
__GOAL_HEADER_PATTERN := $(__COMMENT_PATTERN)$(SpRE)*\$$\(MAKE\)$(SpRE)*

# The described goals use 3 patterns in (...):
# first for the goal name, second for spacing and the third for the description
__GOAL_MATCH_PATTERN := ($(NonSpRE)+)($(SpRE)+)(.*)

# Like the __GOAL_MATCH_PATTERN, but prepending '- ' to the goal description
__GOAL_PATTERN := \1\2- \3

# Display the list of goals from the comments at the top of the Makefile(s).
help:
	@printf "\nAvailable make goals:\n=====================\n";\
	awk '/^$(SpRE)*$$/ {next} \
	      /$(__COMMENT_PATTERN)/ {print; next} \
	      {exit}'\
	  $(MAKEFILE_LIST) |\
	sed -nE\
	  's/$(__GOAL_HEADER_PATTERN)$(__GOAL_MATCH_PATTERN)/$(__GOAL_PATTERN)/p';\
	printf "\n################################\n"

# Compiler: g++[[-]VersionMajor] or clang++[[-]VersionMajor]
CXX := g++

# Linker should be the same as the compiler, thus ignoring any CC from command line
override CC = $(CXX)

# clang++ and g++ have same suffix (g++)
__OMIT_CLAN := $(CC:clan%=%)
__AFTER_GPLUSPLUS := $(strip $(__OMIT_CLAN:g++%=%))
CC_TYPE := $(CC:%$(__AFTER_GPLUSPLUS)=%)# clang++ or g++ or empty

ifneq ($(CC_TYPE:clan%=%),g++)
  $(error Wrong CXX make parameter ($(CXX))!\
          Expecting CXX=[clan]g++[[-]VersionMajor].)
endif

CC_VERSION := $(shell $(CC) --version | head -n 1)
ifeq ($(Empty),$(CC_VERSION))
  $(error Cannot execute '$(CC) --version'!)
endif

# Finding the latest C++ language standard available for the given compiler.
# Clang++ will provide this information while trying to compile a dummy empty file
# for an invalid value for '-std' as hint[s] about the possible valid values.
# G++ offers the information while launched with "-v --help" parameters.
ifeq (clang++,$(CC_TYPE))
  CPP_STANDARD := $(shell echo "" |\
    clang++ -c -std=c++latest -x c++ - 2>&1 |\
    grep -oEe "c\+\+2[0-9a-z]" |\
    sort -r |\
    head -n1)
else
  CPP_STANDARD := $(shell g++ -v --help 2>/dev/null |\
    grep -e "-std=c++2" |\
    grep -oEe "c\+\+2[0-9a-z]" |\
    sort -r |\
    head -n1)
endif

CPP_STANDARD_TEST := $(shell echo "" |\
  $(CC) -c -std=$(CPP_STANDARD) -x c++ - -o /dev/null 2>&1 |\
  grep -o error)

ifeq ($(CPP_STANDARD_TEST),error)
  $(error Invalid provided/determined C++ standard for $(CC): $(CPP_STANDARD)!)
endif

# Determine OS(Linux or WSL/MSYS2/Cygwin or Android or FreeBSD or MacOS) name
# and whether it is arm, 32 or 64bit

# Operating system: Msys, Cygwin, GNU/Linux, FreeBSD, Darwin or Android
UNAME_O := $(shell uname -o)

# Detecting WSL
# https://superuser.com/questions/1749781/how-can-i-check-if-the-environment-is-wsl-from-a-shell-script
# Considering kernel release: ...WSL... only for WSL; Otherwise some numbers
UNAME_R := $(shell uname -r)

# Machine:
# - 64 bits: x86_64, amd64
# - 32 bits: i<n>86 (n<=6)
# - arm: armv<n>..
UNAME_M := $(shell uname -m)

# Prefix __BASE_OUT_DIR by 'x' to denote executables (like Visual Studio does)
ifeq (armv,$(findstring armv,$(UNAME_M)))
  BITNESS := $(Empty)
  __BASE_OUT_DIR := x_arm
  ARCH_LABEL := arm

else
  BITNESS := $(shell (echo $(UNAME_M) | grep -q "64" && echo 64) ||\
    echo 32)
  __BASE_OUT_DIR := x$(BITNESS)
  ARCH_LABEL := $(BITNESS)bit
endif

show_compiler_info:
	@printf "\n=====================\n\
	Using $(CPP_STANDARD) standard from compiler $(CC_VERSION) \
	for a $(UNAME_M) machine!\n\
	If there appear errors within standard headers, \
	retry providing the CPP_STANDARD=<next lower standard> \
	parameter to make!\n\
	=====================\n"

# Source folders
SRC_DIR := src/
TESTS_DIR := test/

# Precompiled files
PCH_GENERATED_FROM := $(SRC_DIR)precompiled.h

# The C++ sources of the project
SOURCES := $(filter-out precompiledHeaderGenerator.cpp,\
  $(notdir $(wildcard $(SRC_DIR)*.cpp)))
TESTS_ONLY_SOURCES := $(notdir $(wildcard $(TESTS_DIR)*.cpp))
TESTS_SOURCES := $(SOURCES) $(TESTS_ONLY_SOURCES)

# Helper base paths for the generated files
BASE_OUT_DIR := $(__BASE_OUT_DIR)/$(CC_TYPE)/

BYPASSED_WARNINGS :=\
  -Wno-missing-declarations\
  -Wno-unknown-pragmas\
  -Wno-reorder

# Append -DH_PRECOMPILED to check for missing includes within individual cpp-s.
# Remove -DH_PRECOMPILED after correcting the includes from each cpp with issues.
# Similarly, -Winvalid-pch explains why pch files did not match within PCH_STORE_FOLDER.
# Also, -H flag shows the tried pch-s and which did match plus all included headers.
# -march=native [-mtune=...] might generate some additional speed-up,
# but limits which machines can run the binary.
# There is a clang warning to replace '-Ofast' by '-O3 -ffast-math'.
# However, -ffast-math produces errors in some clang versions around NaN values,
# thus an extra flag '-fhonor-nans' is required.
# Gcc 8 and earlier copes with NaN-s using '-fno-finite-math-only'
# Clang can use its own implementation of the standard library: '-stdlib=libc++',
# Clang version of the standard library has parts depending on '-fexperimental-library'.
COMMON_CXXFLAGS :=\
  -std=$(CPP_STANDARD) -march=native -Ofast -fno-finite-math-only\
  -Wall -Wextra

ifeq (clang++,$(CC_TYPE))
  __FLAGS_TO_REPLACE  := -Ofast -fno-finite-math-only
  __REPLACEMENT_FLAGS := -O3 -ffast-math -fhonor-nans
  COMMON_CXXFLAGS :=\
    $(subst $(__FLAGS_TO_REPLACE),$(__REPLACEMENT_FLAGS),$(COMMON_CXXFLAGS))
endif

# Platform/OS-specific variables/changes
ifeq (Msys,$(UNAME_O))
  CURRENT_OS := MSYS2

  ifeq (clang++,$(CC_TYPE))
    COMMON_CXXFLAGS += -stdlib=libc++ -fexperimental-library
  endif

else ifeq (Cygwin,$(UNAME_O))
  CURRENT_OS := Cygwin

  ifeq (clang++,$(CC_TYPE))
    COMMON_CXXFLAGS += -stdlib=libc++ -fexperimental-library
  endif

else ifeq (WSL,$(findstring WSL,$(UNAME_R)))
  CURRENT_OS := WSL

  ifeq (clang++,$(CC_TYPE))
    COMMON_CXXFLAGS += -stdlib=libc++ -fexperimental-library
  endif

else ifeq (FreeBSD,$(UNAME_O))
  CURRENT_OS := FreeBSD

  ifeq (clang++,$(CC_TYPE))
    COMMON_CXXFLAGS += -fexperimental-library
 
  else # GCC
    # Gcc from FreeBSD has problems using precompiled headers
    COMMON_CXXFLAGS += -stdlib=libstdc++ -DH_PRECOMPILED
  endif

else ifeq (Darwin,$(UNAME_O))
  CURRENT_OS := MacOS

  ifeq (g++,$(CC_TYPE))
    COMMON_CXXFLAGS += -stdlib=libstdc++
 
  else # AppleClang
    COMMON_CXXFLAGS += -fexperimental-library
  endif

else ifeq (Android,$(UNAME_O))
  CURRENT_OS := Android

  # Termux from Android has Clang as default compiler and gcc is a link to clang.
  # It uses already 'libc++', no need to require it.
  # The experimental library appears to be unavailable,
  # thus '-fexperimental-library' was not appended.
  ifeq (g++,$(CC_TYPE))
    $(error g++ on Termux is a link to clang++!)
  endif

else ifeq (GNU/Linux,$(UNAME_O))
  CURRENT_OS := Linux

  ifeq (clang++,$(CC_TYPE))
    COMMON_CXXFLAGS += -stdlib=libc++ -fexperimental-library
  endif

else
  UNAME_A := $(shell uname -a)
  $(error OS not handled! The answer to 'uname -a' is: $(UNAME_A))
endif

# Flags for stripping the release executable ('-s').
# MacOS strip command uses None OR '-u -r'
STRIP_FLAGS := $(if $(filter-out MacOS,$(CURRENT_OS)),-s,$(Empty))

# Executables have '.exe' extensions (on MSYS2/Cygwin[/MSVC]) or none.
TARGET_EXT := $(if $(filter MSYS2 Cygwin,$(CURRENT_OS)),.exe,$(Empty))

# The included file below must define the path: INCLUDE_DIR_GSL
include GSL_Dirs$(CURRENT_OS).mk

ifeq ($(Empty),$(INCLUDE_DIR_GSL))
  $(error GSL include folder not set!)
endif

ifeq ($(Empty),$(wildcard $(INCLUDE_DIR_GSL)gsl))
  $(error GSL include folder ($(INCLUDE_DIR_GSL))\
          must contain a 'gsl' subfolder!)
endif

# Configuration for necessary Boost libraries
REQUIRED_BOOST_LIBS_SKIP_TEST := $(Empty)# None yet; Insert any apart from boost_unit_test_framework
ALL_REQUIRED_BOOST_LIBS :=\
  $(REQUIRED_BOOST_LIBS_SKIP_TEST) boost_unit_test_framework

# Other necessary libraries.None for now.
# Example exception: MSYS2 MinGW with gcc8 requires 'stdc++fs'
EXTRA_LIBS := $(Empty)

# The included file below must define 2 paths: INCLUDE_DIR_BOOST and LIB_DIR_BOOST.
# It also has to define 'boost_libs_available' (.PHONY) target which ensures that
# ALL_REQUIRED_BOOST_LIBS are created/available.
# When necessary, it has to set BOOST_LIB_ENDING_DEBUG|RELEASE to somethink like
# '-mgw14-mt[-d]-x64-1_87' (for MSYS2 MinGW with gcc-14, multithreading[, debug], x64, Boost 1.87)
include BoostDirs$(CURRENT_OS).mk

ifeq ($(Empty),$(INCLUDE_DIR_BOOST))
  $(error Boost include folder not set!)
endif
ifeq ($(Empty),$(LIB_DIR_BOOST))
  $(error Boost lib folder not set!)
endif

ifeq ($(Empty),$(wildcard $(INCLUDE_DIR_BOOST)boost))
  $(error Boost include folder ($(INCLUDE_DIR_BOOST))\
          must contain a 'boost' subfolder!)
endif

LIB_DIR_BOOST_NO_TRAILING_SLASH := $(LIB_DIR_BOOST:%/=%)

PROJECT_INCLUDE_DIRS := "$(SRC_DIR)"
TESTS_ONLY_INCLUDE_DIR := "$(TESTS_DIR)"
FOREIGN_INCLUDE_DIRS :=\
  "$(INCLUDE_DIR_BOOST)"\
  "$(INCLUDE_DIR_GSL)"

# idirafter and isystem can apply to any include, but they mark those as system,
# thus no warnings are generated from such includes.
# idirafter ensures the actual system includes are searched before idirafter ones.
# Using isystem for default Boost headers installation in
# /usr/include (which is already a system include folder)
# generates file not found error for standard headers like <stdlib.h>,
# while idirafter does the job.
# iquote applies only to #include "".
COMMON_INCLUDES :=\
  $(PROJECT_INCLUDE_DIRS:%=-iquote %)\
  $(FOREIGN_INCLUDE_DIRS:%=-idirafter %)
TESTS_ONLY_INCLUDES := -iquote $(TESTS_ONLY_INCLUDE_DIR)

COMMON_LINKFLAGS := -flto=auto -L"$(LIB_DIR_BOOST_NO_TRAILING_SLASH)"

# Whenever the required libraries are not within the expected folders,
# the generated executable needs to be launched within an environment indicating
# where else to look for libraries.
# -Wl,-rpath is not supported for .dll (thus not by MSYS2/Cygwin).
# This is how to prepend/append the PATH in MSYS2/Cygwin:
# 	PATH=ColonSeparatedPathsForFoldersOfUnusualLibs:$PATH <generatedExe>
# This becomes the <rpath> where to look first for libraries.
ifeq ($(Empty),$(filter MSYS2 Cygwin,$(CURRENT_OS)))
  COMMON_LINKFLAGS += -Wl,-rpath,"$(LIB_DIR_BOOST_NO_TRAILING_SLASH)"
endif

# Clang++ should use libc++, compiler-rt, libunwind and lld (defaults on MacOS,
#  FreeBSD, Android and MSYS2-Clang64).
ifeq ($(Empty),$(filter MacOS FreeBSD Android,$(CURRENT_OS)))
  ifeq ($(CC_TYPE),clang++)
    __MSYS2_ENV := $(shell echo "$(MSYSTEM)" | grep -oie "^[a-z]*")
    ifneq ($(CURRENT_OS)_$(__MSYS2_ENV),MSYS2_CLANG)
      COMMON_LINKFLAGS +=\
        -rtlib=compiler-rt -unwindlib=libunwind -fuse-ld=lld
    endif
  endif
endif

# List of required folders (to be created if missing). Appended later
REQUIRED_FOLDERS := $(Empty)

# Defines a variable for a required folder and adds its value to REQUIRED_FOLDERS.
# Parameters:
# - 1: the variable for the required folder
# - 2: the path of the required folder
define requiredFolder
  $(1) := $(2)
  REQUIRED_FOLDERS += $$($(1))
endef

# Add PCH_STORE_FOLDER as required folder (stores PCH files)
$(eval $(call requiredFolder,PCH_STORE_FOLDER,$$(PCH_GENERATED_FROM).gch))

# Dependency files '*.d'targets are created/overwritten by each compilation of
# a '*.cpp' source.
# Adds fallback rules for '*.d' dependency files (when they need to be created);
# Updates clean_DEPS target.
# Parameters:
# - 1: the build type
define configDeps
  # Normally, intermediary files like '*.d' get discarded at the end of make.
  # Setting them as '.PRECIOUS' ensures they are kept.
  # They cannot be corrupt, because they result from a simple rename operation
  # at the end of a successful compilation and sed transformation.
  # '.SECONDARY' didn't work ('*.d' files got automatically removed)
  # and, as explained above, it couldn't be safer in this case than '.PRECIOUS'.
  .PRECIOUS: $$($(1)_DEPDIR)/%.d

  # Fallback rule for dependencies not generated yet
  $$($(1)_DEPDIR)/%.d: ;

  # Appends a recipe line for removing the '*.d' files for the build type
  clean_DEPS::
	rm -f $$($(1)_DEPDIR)/*.d
endef

__PCH_PREFIX := $(PCH_STORE_FOLDER)/$(CURRENT_OS)_$(ARCH_LABEL)_$(CC_TYPE)_

# Creates a variable pointing the PCH file for the build type;
# Sets target-specific variables for the mentioned PCH file;
# Updates clean_PCH target.
# Parameters:
# - 1: the build type
# - 2: specific compile flags
define configPCH
  PCH_$(1) := $$(__PCH_PREFIX)$(1)

  $$(PCH_$(1)): BUILD_TYPE := $(1)
  $$(PCH_$(1)): CXXFLAGS := $(COMMON_CXXFLAGS) $(2)

  # The header to be precompiled contains no headers from TESTS_DIR,
  # thus there's no need to add TESTS_ONLY_INCLUDES
  $$(PCH_$(1)): INCLUDES := $(COMMON_INCLUDES)

  # Appends a recipe line for removing the PCH file for the build type
  clean_PCH::
	rm -f $$(PCH_$(1))
endef

# Sets target-specific variables for the '*.o' files for the build type;
# Updates clean_OBJ target.
# Parameters:
# - 1: the build type
# - 2: specific compile flags
define configObjFiles
  $$($(1)_OUT_DIR)/%.o: BUILD_TYPE := $(1)
  $$($(1)_OUT_DIR)/%.o: DEPDIR := $$($(1)_DEPDIR)
  $$($(1)_OUT_DIR)/%.o: PCH_FILE := $$(PCH_$(1))
  $$($(1)_OUT_DIR)/%.o: CXXFLAGS := $(COMMON_CXXFLAGS) $(2)

  # TESTS build includes also '*.hpp' files from TESTS_DIR
  $$($(1)_OUT_DIR)/%.o: INCLUDES :=\
    $(if $(filter TESTS,$(1)),$(TESTS_ONLY_INCLUDES))\
    $(COMMON_INCLUDES)

  # Appends a recipe line for removing the '*.o' files for the build type
  clean_OBJ::
	rm -f $$($(1)_OUT_DIR)/*.o
endef

TARGET := $(PROJECT_NAME)$(TARGET_EXT)

# Creates the top goal for the build type;
# Sets target-specific variables for the executable for the build type;
# Updates clean_EXE target.
# Parameters:
# - 1: the build type
# - 2: name of the top-level goal for the build type
# - 3: specific compile flags
define configExeAndTopGoal
  # The top goal for the build type makes sure the required boost libs are available.
  # It displays also the compiler information.
  $(2): $$($(1)_OUT_DIR)/$$(TARGET) | show_compiler_info boost_libs_available

  $$($(1)_OUT_DIR)/$$(TARGET): BUILD_TYPE := $(1)
  $$($(1)_OUT_DIR)/$$(TARGET): CXXFLAGS := $(COMMON_CXXFLAGS) $(3)

  # Appends a recipe line for removing the executable for the build type
  clean_EXE::
	rm -f $$($(1)_OUT_DIR)/$$(TARGET)
endef

# Start empty; Appended by the 'newBuildType' calls
BUILD_TYPES := $(Empty)
TOP_GOALS_PER_BUILD_TYPE := $(Empty)

# Sets the specific settings for a build type:
# * updates TOP_GOALS_PER_BUILD_TYPE and BUILD_TYPES.
# * creates the cleaning target clean_<p_1>.
# * uses other functions to:
#   - update REQUIRED_FOLDERS
#   - configure the dependency ('*.d'), PCH, object ('*.o') and the executable files
# Parameters:
# - p_1: the build type
# - p_2: name of the top-level goal for the build type
# - p_3: name of the folder containing the resulted object and executable files
# - p_4: specific compile flags
define newBuildType
  $(eval p_1 := $(strip $(1)))
  $(eval p_2 := $(strip $(2)))
  $(eval p_3 := $(strip $(3)))
  $(eval p_4 := $(strip $(4)))

  BUILD_TYPES += $(p_1)
  TOP_GOALS_PER_BUILD_TYPE += $(p_2)

  # Create and set variables for required folders *_OUT_DIR and *_DEPDIR
  $$(eval $$(call requiredFolder,$$(p_1)_OUT_DIR,$$(BASE_OUT_DIR)$$(p_3)))
  $$(eval $$(call requiredFolder,$$(p_1)_DEPDIR,.deps/$$(p_1)))

  # Add fallback rules for '*.d' dependency files; Update clean_DEPS target
  $$(eval $$(call configDeps,$$(p_1)))

  # Create a variable pointing the PCH file; Update clean_PCH target
  $$(eval $$(call configPCH,$$(p_1),$$(p_4)))

  # Update clean_OBJ target
  $$(eval $$(call configObjFiles,$$(p_1),$$(p_4)))

  # Create the top goal for the build type; Update clean_EXE target
  $$(eval $$(call configExeAndTopGoal,$$(p_1),$$(p_2),$$(p_4)))

  # Clean rule for the build type
  clean_$(p_1):
	rm -f $$($(p_1)_OUT_DIR)/$$(TARGET)
	rm -f $$($(p_1)_OUT_DIR)/*.o
	rm -f $$($(p_1)_DEPDIR)/*.*d
	rm -f $$(PCH_$(p_1))
endef

# Sets the specific settings for each build type
$(eval $(call newBuildType,RELEASE,release,Release,-DNDEBUG))
$(eval $(call newBuildType,DEBUG,debug,Debug,-g))
$(eval $(call newBuildType,TESTS,tests,tests,\
  -g -DUNIT_TESTING -DBOOST_TEST_DYN_LINK -DBOOST_TEST_NO_LIB))

all: $(TOP_GOALS_PER_BUILD_TYPE)

clean_ALL: $(BUILD_TYPES:%=clean_%)

# Command for running the tests
__CMD_RUN_TESTS := $(TESTS_OUT_DIR)/$(TARGET)\
  --log_level=test_suite\
  --report_level=short\
  --build_info=yes\
  --color_output

# Boost libraries use suffixes on MSYS2 [and MSVC].
# The corresponding included BoostDirs*.mk must have set these values already.
BOOST_LIB_ENDING_RELEASE ?= $(Empty)
BOOST_LIB_ENDING_DEBUG ?= $(Empty)

# Builds and runs the tests.
# MSYS2 and Cygwin must prepend PATH with the folder containing Boost binaries
# because the executable generated by them does not hold RPATH values.
run_TESTS: $(TESTS_OUT_DIR)/$(TARGET) |\
  $(ALL_REQUIRED_BOOST_LIBS:%=-l%$(BOOST_LIB_ENDING_DEBUG)) $(EXTRA_LIBS)
	@printf "\n ~~~~ Running the tests ~~~~\n"
ifneq ($(Empty),$(filter MSYS2 Cygwin,$(CURRENT_OS)))
	PATH=$(LIB_DIR_BOOST_NO_TRAILING_SLASH):$$PATH $(__CMD_RUN_TESTS)
else
	$(__CMD_RUN_TESTS)
endif

# The executable targets depend on the corresponding *.o files.
# LINKFLAGS contain the '-L"lib_dir"' information.
# Relinking is required also for newly compiled Boost libraries.
# Parameters (stripped, to allow line breaks between call parameters):
# - p_1: the build type
# - p_2: the variable containing the set of sources to use
# - p_3: the variable containing the set of Boost libraries to use
# - p_4: the variable containing the suffix of used Boost libraries
# - p_5: extra link flags with any ',' replaced by $(Comma)
define addRuleForExecutable
  $(eval p_1 := $(strip $(1)))
  $(eval p_2 := $(strip $(2)))
  $(eval p_3 := $(strip $(3)))
  $(eval p_4 := $(strip $(4)))
  $(eval p_5 := $(strip $(5)))

  LIB_DEPS_$(p_1) := $$($(p_3):%=-l%$$($(p_4))) $$(EXTRA_LIBS)
  LINKFLAGS_$(p_1) := $$(COMMON_LINKFLAGS) $(p_5)

  $$($(p_1)_OUT_DIR)/$$(TARGET): $$($(p_2):%.cpp=$($(p_1)_OUT_DIR)/%.o) |\
      $$(LIB_DEPS_$(p_1))
	@printf "\n[$$(BUILD_TYPE)] ==== Linking '$$(notdir $$@)' ====\n"
	$$(CC) $$(CXXFLAGS) $$(LINKFLAGS_$(p_1)) -o $$@ $$($(p_1)_OUT_DIR)/*.o\
	  $$(LIB_DEPS_$(p_1)) &&\
	$$(if $$(filter RELEASE,$(p_1)),strip $$(STRIP_FLAGS) $$@,true)

  GOALS_NEEDING_DEPFILES += $($(p_1)_OUT_DIR)/$(TARGET)
endef

$(eval $(call addRuleForExecutable,RELEASE,SOURCES,\
  REQUIRED_BOOST_LIBS_SKIP_TEST,BOOST_LIB_ENDING_RELEASE,$(Empty)))

$(eval $(call addRuleForExecutable,DEBUG,SOURCES,\
  REQUIRED_BOOST_LIBS_SKIP_TEST,BOOST_LIB_ENDING_DEBUG,$(Empty)))

$(eval $(call addRuleForExecutable,TESTS,TESTS_SOURCES,\
  ALL_REQUIRED_BOOST_LIBS,BOOST_LIB_ENDING_DEBUG,$(Empty)))

# Allows locating prerequisite Boost libraries provided in the '-lboost_X' form
# for the link targets below.
# The search will locate then the libs $(LIB_DIR_BOOST)[lib|cyg]boost_X.(so|dylib|dll[.a]).
# [lib|cyg]*.(so|dylib|dll[.a]) are checked based on .LIBPATTERNS variable
VPATH := $(VPATH):$(LIB_DIR_BOOST_NO_TRAILING_SLASH)

# Expecting only '.so', '.dylib' or '.dll' shared libraries.
.LIBPATTERNS = lib%.so lib%.dylib lib%.dll %.dll cyg%.dll

# Declare the Boost libraries order-dependent on boost_libs_available
# Wait for their generation ($(LIB_DIR_BOOST)[lib|cyg]boost_X.(so|dylib|dll[.a])).
# `which ls` was used because ls might be an alias with flags for displaying extra characters.
-lboost_%: | boost_libs_available
	@__FOUND_BOOST_LIBS_COUNT=$$(\
	  `which ls` $(LIB_DIR_BOOST){lib,cyg,}boost_$*.{so,dylib,dll,dll.a} 2>/dev/null |\
	  wc -l);\
	if [ $$__FOUND_BOOST_LIBS_COUNT -eq 0 ]; then\
	  printf "Couldn't find library:\
	  $(LIB_DIR_BOOST)[lib|cyg]boost_$*.(so|dylib|dll[.a])!\n" >&2;\
	  exit 1;\
	fi

# Commands for compiling each '.cpp' ($< - the first prerequisite).
# The dependency file '.d' generated during compilation might also be corrupt
# if the make is interrupted while generating it.
# Solution:
# 1. generate the dep to a temporary file during compilation '.d.raw'.
#    Use another temporary file '.d.fixed' to transform all absolute paths
#    from '.d.raw' into relative ones, which allow reuse between Cygwin/MSYS2/WSL.
# 2. after a successful compilation, rename '.d.fixed' as the dependency '.d' file
#    to be used and remove the '.d.raw' file.
# 
# This guarantees '.d' files are not incomplete/corrupt and can be set as '.PRECIOUS'.
#
# 'compileUnit' uses deferred expansion (= or no sign) because of:
# - $@, $<, $*
# - used target-specific variables: BUILD_TYPE, CXXFLAGS, PCH_FILE and DEPDIR
#
# Inplace sed (-i) needs special handling in macOS/FreeBSD and wasn't used here.
# That explains the 2 files '.d.raw' and '.d.fixed' instead of just one.
define compileUnit
	@printf "\n[$(BUILD_TYPE)] ---- Compiling '$(notdir $<)' ----\n"
	$(CXX) -c $(CXXFLAGS) $(BYPASSED_WARNINGS) $(INCLUDES)\
	  $(if $(filter clang++,$(CC_TYPE)),-include-pch $(PCH_FILE))\
	  -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d.raw -o $@ $< &&\
	sed -E 's@(^|$(SpRE))([a-zA-Z]:)?/$(NonSpRE)+/$(PROJECT_NAME)/@\1@g'\
	  $(DEPDIR)/$*.d.raw > $(DEPDIR)/$*.d.fixed &&\
	mv -f $(DEPDIR)/$*.d.fixed $(DEPDIR)/$*.d &&\
	rm -f $(DEPDIR)/$*.d.raw
endef

# Adds a compile rule *.cpp -> *.o for a build type.
# Do not place $($(p_1)_DEPDIR)/%.d among the normal prerequisites!
# They needs to remain among order-only prerequisites (after the '|') to avoid
# recompilations due to '.d' rewrites caused by the compilation process itself.
# Takes a single parameter - p_1 (the build type).
define addCompileRule
  $(eval p_1 := $(strip $(1)))

  $($(p_1)_OUT_DIR)/%.o: $(SRC_DIR)%.cpp $(PCH_$(p_1)) |\
      $($(p_1)_OUT_DIR) $($(p_1)_DEPDIR) $($(p_1)_DEPDIR)/%.d
	$(value compileUnit)
endef

$(foreach buildType,$(BUILD_TYPES),$(eval $(call addCompileRule,$(buildType))))

# A part of the sources for the 'tests' target can be found only in TESTS_DIR
$(TESTS_OUT_DIR)/%.o: $(TESTS_DIR)%.cpp $(PCH_TESTS) |\
  $(TESTS_OUT_DIR) $(TESTS_DEPDIR) $(TESTS_DEPDIR)/%.d
	$(compileUnit)

__ALL_OBJ_FILES :=\
  $(foreach buildType,$(BUILD_TYPES),$(SOURCES:%.cpp=$($(buildType)_OUT_DIR)/%.o))\
  $(TESTS_ONLY_SOURCES:%.cpp=$(TESTS_OUT_DIR)/%.o)

GOALS_NEEDING_DEPFILES += $(__ALL_OBJ_FILES)

PCH_FILES :=\
  $(foreach buildType,$(BUILD_TYPES),$(PCH_$(buildType)))
DEPFILES_NEUTRAL_GOALS += $(PCH_FILES)

# Generating PCH files except for FreeBSD when using g++
$(PCH_FILES): $(PCH_GENERATED_FROM) | $(PCH_STORE_FOLDER)
ifneq (FreeBSD_g++,$(CURRENT_OS)_$(CC_TYPE))
	@printf "\n[$(BUILD_TYPE)] **** Compiling PCH '$(notdir $@)' ****\n"
	$(CXX) -c $(CXXFLAGS) $(BYPASSED_WARNINGS) $(INCLUDES) -o $@ -x c++-header $<
endif

################################################################################
# The following variables should not be modified below this region!
# - REQUIRED_FOLDERS
# - DEPFILES_AVERSE_GOALS
# - DEPFILES_NEUTRAL_GOALS
# - GOALS_NEEDING_DEPFILES
################################################################################

# Reporting invalid targets (typos, etc)
PROVIDED_GOALS := $(if $(MAKECMDGOALS),$(MAKECMDGOALS),$(.DEFAULT_GOAL))
__VALID_GOALS :=\
  $(DEPFILES_AVERSE_GOALS) $(DEPFILES_NEUTRAL_GOALS) $(GOALS_NEEDING_DEPFILES)
INVALID_GOALS := $(filter-out $(__VALID_GOALS),$(PROVIDED_GOALS))
ifneq ($(Empty),$(INVALID_GOALS))
  $(error Invalid target(s): "$(INVALID_GOALS)"!)
endif

# Adding the generated additional prerequisites for the rules for compiling *.o
# except when make has to build only targets from
# DEPFILES_NEUTRAL_GOALS OR DEPFILES_AVERSE_GOALS.
USED_DEPFILES_AVERSE_GOALS := $(filter $(DEPFILES_AVERSE_GOALS),$(MAKECMDGOALS))
USED_GOALS_NEEDING_DEPFILES :=\
  $(filter-out $(DEPFILES_NEUTRAL_GOALS) $(DEPFILES_AVERSE_GOALS),$(MAKECMDGOALS))

ifneq ($(Empty),$(USED_DEPFILES_AVERSE_GOALS))
  ifneq ($(Empty),$(USED_GOALS_NEEDING_DEPFILES))
    $(info ERROR:)
    $(info The included dependency files '*.d' already parsed for\
           "$(USED_GOALS_NEEDING_DEPFILES)" are deleted by\
           "$(USED_DEPFILES_AVERSE_GOALS)".)
    $(info ("$(USED_DEPFILES_AVERSE_GOALS)" might change make's internal\
           map of the world established when preparing for\
           "$(USED_GOALS_NEEDING_DEPFILES)"!))
    $(info (Expecting errors like 'No rule to make target ... needed by include'.))
    $(info )
    $(info If this doesn't seem ok, make sure any target mentioned above is:)
    $(info - included in DEPFILES_AVERSE_GOALS if it deletes '*.d' dependencies)
    $(info - included in DEPFILES_NEUTRAL_GOALS if it doesn't need '*.d' dependencies)
    $(info - included in GOALS_NEEDING_DEPFILES if it does need '*.d' dependencies.)
    $(info )
    $(error The recommended rebuild command is 'make clean_ALL && make all'.)
  endif
endif

# Creating required folders:
$(REQUIRED_FOLDERS):
	@mkdir -p $@ 2>/dev/null

ifneq ($(Empty),$(USED_GOALS_NEEDING_DEPFILES))
  __ALL_DEP_FILES :=\
    $(SOURCES:%.cpp=$(RELEASE_DEPDIR)/%.d $(DEBUG_DEPDIR)/%.d)\
    $(TESTS_SOURCES:%.cpp=$(TESTS_DEPDIR)/%.d)

  # The '-' before 'include' cancels errors about missing '*_DEPDIR' folders
  -include $(__ALL_DEP_FILES)
endif
