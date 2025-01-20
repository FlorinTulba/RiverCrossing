:: Script for launching from Windows CMD/Powershell
:: the unit testing for RiverCrossing built using MSVC

@echo off

:: To revert PATH variable to its previous value when leaving the script
setlocal

if "%~1"=="" (
	echo There must be a libraryPath(s^) parameter^!
	echo It is the value of 'BoostBinariesDir' configured in 'LibBoost.props'^!
	exit /B 1
)

:: The unit test executable does accept arguments like:
::  '--build_info=yes'
::
:: However, handling program arguments containing the '=' sign is problematic:
::  https://stackoverflow.com/questions/4940375/how-do-i-pass-an-equal-sign-when-calling-a-batch-script-in-powershell
::
:: Luckily, there is a short form for each unit test parameter:
::	--log_level=test_suite -> -l test_suite
::	--report_level=short   -> -r short
::	--build_info=yes       -> -i yes
PATH=%~1;%PATH% && call _run.bat tests ^
	-l test_suite ^
	-r short ^
	-i yes ^
	--color_output || (
			echo The tests could not be run^!
			echo Probably the libraryPath(s^) parameter is incorrect^!
			exit /B 1
	)

exit /B 0
