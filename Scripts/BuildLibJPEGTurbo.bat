@ECHO OFF
PUSHD ..\Deps\libjpeg-turbo
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release --target turbojpeg-static
cmake --build build --config Debug --target turbojpeg-static
POPD
IF %ERRORLEVEL% NEQ 0 ( PAUSE )