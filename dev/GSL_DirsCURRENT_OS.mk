# Template for the makefiles providing the path from where
# Microsoft GSL headers can be included

# Not required for MSVC development!

# Instructions how to use it:
# 1. Copy the template to the parent folder
# 2. Rename it there by replacing 'CURRENT_OS' with the value of $(CURRENT_OS)
#    obtained inside the Makefile (Android/Cygwin/FreeBSD/Linux/MacOS/MSYS2/WSL)
# 3. Replace '# COMMENT' from below with Microsoft GSL include path.
#    For instance, when 'libmsgsl-dev' is installed, there will be a folder
#       /usr/include/gsl/
#    where the GSL headers will appear. In this case
#       INCLUDE_DIR_GSL := /usr/include/
#    provides the required path
INCLUDE_DIR_GSL := # COMMENT
