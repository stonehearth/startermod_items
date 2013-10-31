set CFG=%1
set DST=%2
set CHROMIUM_ROOT=%RADIANT_ROOT%\stonehearth\modules\chromium-embedded\package\cef_binary_3.1547.1412_windows32
if "%CFG%"=="RelWithDebInfo" set CFG=Release
::
:: convert / to \
set DST=%DST:/=\%

:: lua...
set LUA_ROOT=%RADIANT_ROOT%\stonehearth\modules\lua\package\lua
echo xcopy /d /y %LUA_ROOT%\solutions\%CFG%\lua.* %DST%
xcopy /d /y %LUA_ROOT%\solutions\%CFG%\lua.* %DST%
xcopy /d /y %LUA_ROOT%\solutions\%CFG%\lua-5.1.5.* %DST%

:: chromium embedded...
xcopy /d /y /s %CHROMIUM_ROOT%\resources %DST%
xcopy /d /y %CHROMIUM_ROOT%\Release\libcef.* %DST%
xcopy /d /y %CHROMIUM_ROOT%\Release\icudt.* %DST%

:: sfml 
set SFML_ROOT=%RADIANT_ROOT%\stonehearth\modules\sfml\build\lib\%CFG%
xcopy /d /y /s %SFML_ROOT%\* %DST%
xcopy /d /y /s %RADIANT_ROOT%\stonehearth\modules\sfml\package\SFML-2.1\extlibs\bin\x86\openal32.dll %DST%
xcopy /d /y /s %RADIANT_ROOT%\stonehearth\modules\sfml\package\SFML-2.1\extlibs\bin\x86\libsndfile-1.dll %DST%

:: crash_reporter
set CRASH_REPORTER_ROOT=%RADIANT_ROOT%\stonehearth\build\source\lib\crash_reporter
xcopy /d /y %CRASH_REPORTER_ROOT%\%CFG%\crash_reporter.exe %DST%
