:: Script for launching from Windows CMD/Powershell
:: the release/debug/tests versions of RiverCrossing built using MSVC

:: IMPORTANT:
:: 'run(Release|Debug|tests).bat' is the preferred method to launch the application!

@echo off

:: EnableDelayedExpansion needed for concatenating several command args
setlocal EnableDelayedExpansion

:: Cygwin and MSIS have the env var SHELL defined, while CMD/Powershell don't
if NOT "%SHELL%"=="" (
	echo 'run*.bat' are not available for Cygwin, nor for MSIS^!
	exit /B 1
)

if "%~1"=="" (
	echo Usage:
	echo 	_run.bat Debug^|Release^|(tests testParam*^) ^!
	echo Please specify at least 1 parameter^!
	exit /B 1
)

:: Troubleshooting text in case '.\_run.bat tests' is launched
:: without preceding it with an updated PATH, like:
:: 'PATH=<libraryPath(s)>;$PATH && .\_run.bat tests <testArgs>'
:: This mechanism is enforced and better explained by '.\runTests.bat <testArgs>'
:: When '.\_run.bat tests' fails, the troubleshooting text possibleProblem will appear.
set possibleProblem=

:: The program to run (Release|Debug|tests)
set progToRun=

if "%~1"=="Debug" (
	rem Empty
) else if "%~1"=="Release" (
	rem Empty
) else if "%~1"=="tests" (
	set possibleProblem=Note: '_run.bat tests' is supposed to be called from 'runTests.bat'^^! (Ignore if this already is the case^)

) else (
	echo Usage:
	echo 	_run.bat Debug^|Release^|(tests testParam*^) ^!
	echo Invalid first parameter: "%~1"^!
	exit /B 1
)

set progToRun=.\x64\msvc\%1\RiverCrossing.exe

if NOT exist "%progToRun%" (
	echo No executable found for the MSVC %1 configuration^!
	exit /B 1
)

:: nextParams will be the rest of the parameters (starting from second one)
shift
set nextParams=

:CheckMoreParams
if "%~1"=="" goto NoOtherParams

set nextParams=!nextParams! %1
shift
goto CheckMoreParams

:NoOtherParams
echo Running %progToRun%!nextParams!
%progToRun%!nextParams! || (
	rem Avoid 'echo is off' message when 'possibleProblem' is empty
	if NOT "!possibleProblem!"=="" (
		echo !possibleProblem!
	)
	exit /B 1
)

exit /B 0
