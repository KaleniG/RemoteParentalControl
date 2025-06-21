@ECHO OFF
PUSHD ..\Deps\libjpeg-turbo

:ECHOCHOICE
ECHO.
ECHO Select build option:
ECHO  1. Build 64-bit only
ECHO  2. Build 32-bit only
ECHO  3. Exit
CHOICE /C:1234 /N /M "Enter choice [1-3]: "
IF ERRORLEVEL 3 GOTO End
IF ERRORLEVEL 2 GOTO Build32
IF ERRORLEVEL 1 GOTO Build64

:Build64
ECHO.
ECHO -- 64-bit build --
cmake -S . -B build64 -G "Visual Studio 17 2022" -A x64 -DWITH_SIMD=ON
cmake --build build64 --config Debug --target turbojpeg-static
cmake --build build64 --config Release --target turbojpeg-static
IF %ERRORLEVEL% NEQ 0 ( PAUSE )
GOTO Done

:Build32
ECHO.
ECHO -- 32-bit build --
cmake -S . -B build32 -G "Visual Studio 17 2022" -A Win32 -DWITH_SIMD=ON
cmake --build build32 --config Debug --target turbojpeg-static
cmake --build build32 --config Release --target turbojpeg-static
IF %ERRORLEVEL% NEQ 0 ( PAUSE )
GOTO Done


:Done
ECHO.
ECHO Build process finished.
IF %ERRORLEVEL% NEQ 0 ( PAUSE )
POPD
EXIT /B

:End
POPD
ECHO.
ECHO No builds performed. Exiting.
IF %ERRORLEVEL% NEQ 0 ( PAUSE )
EXIT /B
