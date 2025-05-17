@ECHO OFF
PUSHD ..\Deps\obs-studio
cmake -S . -B build -G "Visual Studio 17 2022" -DBUILD_OBS=OFF -DBUILD_PLUGINS=OFF -DBUILD_CAPTURES=OFF -DBUILD_UI=OFF -DENABLE_UI=OFF -DENABLE_SCRIPTING=OFF -DBUILD_FRONTEND=OFF -DBUILD_BROWSER=OFF
cmake --build build --config Release --target libobs
cmake --build build --config Debug --target libobs
POPD
IF %ERRORLEVEL% NEQ 0 ( PAUSE )