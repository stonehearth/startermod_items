set CFG=%1
set DST=%2
set CHROMIUM_ROOT=%RADIANT_ROOT%\cef_binary_3.1453.1255_windows\package
if "%CFG%"=="RelWithDebInfo" set CFG=Release
::
:: convert / to \
set DST=%DST:/=\%

:: lua...
set LUA_ROOT=%RADIANT_ROOT%\stonehearth-dependencies\lua-5.1.5\package
xcopy /d %LUA_ROOT%\solutions\%CFG%\lua.exe %DST%
xcopy /d %LUA_ROOT%\solutions\%CFG%\lua-5.1.5.dll %DST%

:: chromium embedded...
xcopy /d /s %CHROMIUM_ROOT%\resources %DST%
xcopy /d %CHROMIUM_ROOT%\Release\libcef.* %DST%
xcopy /d %CHROMIUM_ROOT%\Release\icudt.* %DST%

:: sfml 
set SFML_ROOT=%RADIANT_ROOT%\stonehearth-dependencies\sfml-2.0\package\bin
xcopy /d /s %SFML_ROOT%\* %DST%

