@echo off
setlocal

echo ================================================================
echo  MissedClick Chess Engine - Windows Compiler
echo ================================================================

REM --- Try to find g++ (MinGW-w64 in PATH or common install locations) ---
set GXX=
where g++   >nul 2>&1 && set GXX=g++
if "%GXX%"=="" (
    if exist "C:\mingw64\bin\g++.exe"    set GXX=C:\mingw64\bin\g++.exe
    if exist "C:\mingw-w64\bin\g++.exe"  set GXX=C:\mingw-w64\bin\g++.exe
    if exist "C:\msys64\mingw64\bin\g++.exe" set GXX=C:\msys64\mingw64\bin\g++.exe
    if exist "C:\msys64\ucrt64\bin\g++.exe"  set GXX=C:\msys64\ucrt64\bin\g++.exe
)

if "%GXX%"=="" (
    echo.
    echo  ERROR: g++ not found.
    echo  Please install MinGW-w64 from: https://www.mingw-w64.org/
    echo  Or use MSYS2: https://www.msys2.org/
    echo.
    pause
    exit /b 1
)

echo  Compiler: %GXX%

REM --- Collect source files ---
set SRCS=^
 src\board\types.cpp^
 src\board\board.cpp^
 src\board\hash.cpp^
 src\attacks\attacks.cpp^
 src\moves\movegen.cpp^
 src\moves\makemove.cpp^
 src\eval\evaluate.cpp^
 src\eval\evaluation_enhanced.cpp^
 src\search\tt.cpp^
 src\search\moveorder.cpp^
 src\search\search.cpp^
 src\search\search_enhanced.cpp^
 src\utils\utils.cpp^
 src\engine\uci.cpp^
 src\engine\main.cpp

set FLAGS=-std=c++17 -O2 -Isrc -static-libgcc -static-libstdc++ -static

echo  Compiling ...
"%GXX%" %FLAGS% %SRCS% -o MissedClick.exe

if exist MissedClick.exe (
    echo.
    echo  SUCCESS! MissedClick.exe has been created.
    echo  Add it to Arena: Engines - Install New Engine - Browse to MissedClick.exe
    echo.
) else (
    echo.
    echo  BUILD FAILED. Check the error messages above.
    echo.
)

pause
endlocal
