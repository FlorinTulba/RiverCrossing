# Template for the makefiles providing the paths from where
# Boost headers and libraries can be used

# Not required for MSVC development!

# Instructions how to use it:
# 1. Copy the template to the parent folder
# 2. Rename it there by replacing 'CURRENT_OS' with the value of $(CURRENT_OS)
#    obtained inside the Makefile (Android/Cygwin/FreeBSD/Linux/MacOS/MinGW/WSL)
# 3. Replace '# COMMENT' from below with Boost relevant paths.
#    For instance, when 'libboost-all-dev' is installed, there will be 2 folders
#       /usr/include/boost/  and  /usr/lib/x86_64-linux-gnu/
#    where the Boost headers and libraries will appear. In this case
#       INCLUDE_DIR_BOOST := /usr/include/
#       LIB_DIR_BOOST := /usr/lib/x86_64-linux-gnu/
#    provide the required paths.
INCLUDE_DIR_GSL := # COMMENT
LIB_DIR_BOOST := # COMMENT

# 4. When necessary, uncomment and set BOOST_LIB_ENDING_DEBUG|RELEASE from below.
#    For example, for MinGW with gcc-14, multithreading[, debug], x64, Boost 1.87, set:
#       -mgw14-mt[-d]-x64-1_87
#BOOST_LIB_ENDING_DEBUG := # COMMENT
#BOOST_LIB_ENDING_RELEASE := # COMMENT

# 5. The default BoostDirs<CURRENT_OS>.mk assumes the use of Boost libraries
#    installed together with binaries for the given platform.
#    Modify the target below appropriately if the required Boost libraries
#    are not available for your environment/compiler
#    and they need to be generated first.
boost_libs_available:;
