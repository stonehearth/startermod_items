set CFG=%1
set DST=%2

if "%CFG%"=="RelWithDebInfo" set CFG=Release
::
:: convert / to \
set DST=%DST:/=\%

:: lua...
set LUA_ROOT=%RADIANT_ROOT%\stonehearth-dependencies\lua-5.1.4\package
xcopy /d %LUA_ROOT%\lua.exe %DST%
xcopy /d %LUA_ROOT%\lua51.dll %DST%
xcopy /d %LUA_ROOT%\lua5.1.dll %DST%

:: chromium embedded...
set CE_ROOT=%RADIANT_ROOT%\chromium-embedded\package\bin
xcopy /d /s %CE_ROOT%\common\* %DST%
xcopy /d %CE_ROOT%\%CFG%\* %DST%

:: sfml 
set SFML_ROOT=%RADIANT_ROOT%\stonehearth-dependencies\sfml-2.0\package\bin
xcopy /d /s %SFML_ROOT%\* %DST%

