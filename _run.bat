:: Script for launching from Windows CMD/Powershell
:: the release/debug/tests versions of RiverCrossing built using MSVC/ClangCL

:: IMPORTANT:
:: 'run(Release|Debug|Tests).bat' is the preferred method to launch the application!

@echo off

:: EnableDelayedExpansion needed for concatenating several command args
setlocal EnableDelayedExpansion

:: Cygwin and MSYS2 have the env var SHELL defined, while CMD/Powershell don't
if NOT "%SHELL%"=="" (
	echo 'run*.bat' are not available for Cygwin, nor for MSYS2^! 1>&2
	exit /B 1
)

if "%~1"=="" (
	echo Usage: 1>&2
	echo 	_run.bat Debug^|Release^|(tests testParam*^) ^! 1>&2
	echo Please specify at least 1 parameter^! 1>&2
	exit /B 1
)

:: Troubleshooting text in case '.\_run.bat tests' is launched
:: without preceding it with an updated PATH, like:
:: 'PATH=<libraryPath(s)>;$PATH && .\_run.bat tests <testArgs>'
:: This mechanism is enforced and better explained by '.\runTests.bat <testArgs>'
:: When '.\_run.bat tests' fails, the troubleshooting text possibleProblem will appear.
set "possibleProblem="

:: The program to run (Release|Debug|tests)
set "progToRun="

if "%~1"=="Debug" (
	rem Empty
) else if "%~1"=="Release" (
	rem Empty
) else if "%~1"=="tests" (
	set "possibleProblem=Note: '_run.bat tests' is supposed to be called from 'runTests.bat'^! (Ignore if this is already the case^)"

) else (
	echo Usage: 1>&2
	echo 	_run.bat Debug^|Release^|(tests testParam*^) ^! 1>&2
	echo Invalid first parameter: "%~1"^! 1>&2
	exit /B 1
)

:: The possible executables
set "msvcFlavorExe=.\x64\msvc\%1\RiverCrossing.exe"
set "clangFlavorExe=.\x64\clang++\%1\RiverCrossing.exe"

set msvcFlavorExists=0
set clangFlavorExists=0
if exist "%msvcFlavorExe%" set msvcFlavorExists=1
if exist "%clangFlavorExe%" set clangFlavorExists=1

:: Logic for selecting the executable
if !msvcFlavorExists!==0 if !clangFlavorExists!==0 (
    echo No executable found for the %1 configuration^! 1>&2
    exit /B 1
)

if !msvcFlavorExists!==1 if !clangFlavorExists!==1 (
    echo MSVC and ClangCL executable found for the %1 configuration^^!

    set newestFlavorExe=ClangCL
    rem Check if MSVC is newer than Clang by simulating an 'overwrite if newer'
    rem If MSVC is newer, the output is '...1 File(s)'. Otherwise '0 File(s)'.
    rem /D compares dates, /L just reports if newer (doesn't copy), /Y suppresses prompts
    for /F "tokens=1" %%i in ('xcopy /D /L /Y "%msvcFlavorExe%" "%clangFlavorExe%" 2^>nul ^| findstr /C:"1 File(s)"') do (
        if "%%i"=="1" set newestFlavorExe=MSVC
    )

    echo !newestFlavorExe! version is newer.
    call :PickOneFlavorExe

    if "!progToRun!"=="" (
        rem No flavor picked. Instead the user decides to abandon
        echo Leaving ...
        exit /B 0
    )

) else if !msvcFlavorExists!==1 (
    echo Entered MSVC, no Clang
    set "progToRun=%msvcFlavorExe%"

) else (
    echo Entered Clang, no MSVC
    set "progToRun=%clangFlavorExe%"
)

:: Ensure progToRun was actually set before continuing
if "!progToRun!"=="" (
    echo Despite there is at least one executable for the %1 configuration, progToRun is empty^! 1>&2
    exit /B 1
)

:: nextParams will be the rest of the parameters (starting from second one)
shift
set "nextParams="

:CheckMoreParams
if "%~1"=="" goto NoOtherParams

set "nextParams=!nextParams! %1"
shift
goto CheckMoreParams

:NoOtherParams
echo Running "!progToRun!"!nextParams!
"!progToRun!"!nextParams! || (
    rem Avoid 'echo is off' message when 'possibleProblem' is empty
	if NOT "!possibleProblem!"=="" (
		echo !possibleProblem! 1>&2
	)
	exit /B 1
)

exit /B 0

:PickOneFlavorExe
echo Options:
echo -run ^<m^>svc version
echo -run ^<c^>lang version
echo -^<l^>eave
set /P "choice=Please choose: "
goto PerformChoice

:RetryPickingValidOption
set /P "choice=Invalid option: '!choice!'. Please choose again [m|c|l]: "

:PerformChoice
:: The user decides to abandon, instead of picking an option
if /I "!choice!"=="l" goto :EOF

if /I "!choice!"=="m" (
    set "progToRun=%msvcFlavorExe%"
    goto :EOF

) else if /I "!choice!"=="c" (
    set "progToRun=%clangFlavorExe%"
    goto :EOF

) else goto RetryPickingValidOption
