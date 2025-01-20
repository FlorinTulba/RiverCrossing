#!/usr/bin/env bash

usageStr="Usage:\n\trunTests.sh [libraryPath(s)] !"

if [[ `uname -o | grep -E "Msys|Cygwin"` ]]; then
	if [ $# -ne 1 ]; then
		>&2 echo -e "$usageStr"
		>&2 echo "Please provide a library path(s) parameter for MinGW/Cygwin environments!"
		>&2 echo "(Just copy the path from the '-L' argument used by the linker.)"
		exit 1
	fi

	# Ensure libraries can be found by prepending the path with the provided folder(s)
	PATH=$1:$PATH \
	./_run.sh tests \
		--log_level=test_suite \
		--report_level=short \
		--build_info=yes \
		--color_output

else
	# The other OS-s accept setting the <rpath> within the executable,
	# so no need to append/prepend [DY]LD_LIBRARY_PATH for them
	./_run.sh tests \
		--log_level=test_suite \
		--report_level=short \
		--build_info=yes \
		--color_output
fi

exit 0
