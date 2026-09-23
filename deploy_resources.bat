@echo off
REM Copies dev-time runtime resources (the auto-reference-map .rcc, the
REM offline map tiles, and the redistributable DLLs) next to the just-built
REM executable so it can run standalone straight out of Qt Creator.
REM
REM Invoked from whumag.pro's QMAKE_POST_LINK as a real .bat file (rather
REM than inlining shell commands directly in the .pro file) specifically to
REM avoid a cross-environment portability trap: qmake/mingw32-make on
REM Windows sometimes routes POST_LINK through sh.exe (if one is reachable
REM on PATH, e.g. because Git for Windows' usr/bin is on it) and sometimes
REM invokes the command directly via CreateProcess with no shell at all
REM (Qt Creator's plain MinGW kit environment) -- POSIX commands like
REM "cp -f"/"mkdir -p" only work in the former, and bare cmd.exe builtins
REM like "copy"/"mkdir" typed inline fail in the latter (CreateProcess needs
REM a real .exe, and copy/mkdir aren't ones). A .bat file sidesteps this:
REM Windows always hands .bat files to cmd.exe regardless of what launched
REM them, so this script is free to just use plain batch syntax.
REM
REM Usage: deploy_resources.bat <dest_dir> <source_root>

set "DEST=%~1"
set "SRC=%~2"

if not exist "%DEST%\resources" mkdir "%DEST%\resources"
if not exist "%DEST%\offline_tiles" mkdir "%DEST%\offline_tiles"

REM high_quality.rcc is >100MB and distributed separately (see README), so
REM it may legitimately be absent -- don't fail the build if so.
if exist "%SRC%\resources\high_quality.rcc" (
    copy /y "%SRC%\resources\high_quality.rcc" "%DEST%\resources\" >nul
)
copy /y "%SRC%\deploy\*.dll" "%DEST%\" >nul 2>nul
REM offline map tiles: <zoom>\<x>\<y>.png tree.  Tiles of the old flat layout
REM (osm-l-3-z-x-y.png) left in the destination by earlier builds are removed.
del /q "%DEST%\offline_tiles\osm-l-*.png" >nul 2>nul
xcopy "%SRC%\resources\Map\offline_tiles" "%DEST%\offline_tiles\" /e /i /y /q >nul 2>nul

exit /b 0
