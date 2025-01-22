#!/usr/bin/env bash

# Runs Debug/Release/tests for RiverCrossing application

# IMPORTANT:
# 'run(Release|Debug|tests).sh' is the preferred method to launch the application!

usageStr="Usage:\n\t_run.sh Debug|Release|(tests testParam*) !"
if [ $# -lt 1 ]; then
	>&2 echo -e "$usageStr\nPlease specify at least 1 parameter!"
	exit 1
fi

if [ "$1" != "Release" -a "$1" != "Debug" -a "$1" != "tests" ]; then
	>&2 echo -e "$usageStr\nFirst parameter can be only Debug, Release or tests!"
	exit 1
fi

# Troubleshooting text in case './_run.sh tests' is launched for MSYS2/Cygwin
# environments without preceding it with an updated PATH, like:
# 'PATH=<libraryPath(s)>:$PATH ./_run.sh tests <testArgs>'
# This mechanism is enforced and better explained by './runTests <testArgs>'
# When './_run.sh tests' fails, the troubleshooting text possibleProblem will appear.
possibleProblem=""
if [ "$1" == "tests" ]; then
	if [[ `uname -o | grep -E "Msys|Cygwin"` ]]; then
		possibleProblem="Note: For MSYS2/Cygwin environments '_run.sh tests' is supposed to be called from 'runTests.sh'! (Ignore if this already is the case)"
	fi

elif [ $# -gt 1 ]; then
	>&2 echo -e "$usageStr\nOnly _run.sh tests allows more than 1 parameter!"
	exit 1
fi

# Machine:
# - 64 bits: x86_64, amd64
# - 32 bits: i<n>86 (n<=6)
# - arm: armv<n>..
# BASE_OUT_DIR is prefixed by 'x' to denote executables
UNAME_M=`uname -m`
if [[ "$UNAME_M" =~ ^armv ]]; then
	BASE_OUT_DIR="x_arm"

else
	BITNESS=`(echo "$UNAME_M" | grep -q "64" \
				&& echo "64") \
			|| echo "32"`
	BASE_OUT_DIR="x$BITNESS"
fi

TARGET_EXT=""
if [[ `uname -o | grep -E "Msys|Cygwin"` ]]; then
	TARGET_EXT=".exe"
fi

GCC_VERSION="./$BASE_OUT_DIR/g++/$1/RiverCrossing$TARGET_EXT"
CLANG_VERSION="./$BASE_OUT_DIR/clang++/$1/RiverCrossing$TARGET_EXT"

# Get the modification times (seconds since EPOCH) of the 2 versions.
# Set to empty if the corresponding version does not exist.
GCC_VER_EXISTS=`stat "$GCC_VERSION" 2>/dev/null`
CLANG_VER_EXISTS=`stat "$CLANG_VERSION" 2>/dev/null`

if [ -z "$GCC_VER_EXISTS" -a -z "$CLANG_VER_EXISTS" ]; then
	>&2 echo "No executable found for the $1 configuration!"
	exit 1
fi

if [ "$GCC_VER_EXISTS" -a "$CLANG_VER_EXISTS" ]; then
	echo "Gcc and Clang executable found for the $1 configuration!"
	if [ "$GCC_VERSION" -nt "$CLANG_VERSION" ]; then
		echo -n Gcc
	
	else
		echo -n Clang
	fi
	
	echo " version is newer."
	
	echo -ne "Options:\n-run <g>cc version\n-run <c>lang version\n-<l>eave\n\nPlease choose:"
	read option
	while [[ ! "$option" =~ ^[gcl]$ ]]; do
		echo -n "Invalid option: '$option'. Please choose again [g|c|l]: "
		read option
	done

	if [ "$option" == 'l' ]; then
		echo Leaving ...
		exit 0
	fi

	if [ "$option" == 'g' ]; then
		VER_TO_RUN=$GCC_VERSION
	
	else
		VER_TO_RUN=$CLANG_VERSION
	fi

# Only 1 version exists at this point
elif [ "$GCC_VER_EXISTS" ]; then
	VER_TO_RUN=$GCC_VERSION

else
	VER_TO_RUN=$CLANG_VERSION
fi

# Providing the parameters after release/debug/tests
echo "Running $VER_TO_RUN ${@:2}"
$VER_TO_RUN ${@:2} || (>&2 echo -e "\n$possibleProblem"; exit 1)

exit 0
