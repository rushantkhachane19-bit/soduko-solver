@echo off
setlocal
cd /d "%~dp0"
where g++ >nul 2>nul
if errorlevel 1 (
  echo ERROR: g++ was not found.
  echo Install a C++ compiler such as MinGW-w64 or MSYS2 GCC and add it to PATH.
  echo See README.md for setup instructions.
  pause
  exit /b 1
)
echo Compiling Sudoku Solver...
g++ -std=c++17 backend\main.cpp -o sudoku_server.exe -lws2_32
if errorlevel 1 (
  echo.
  echo Compilation failed. Please review the compiler errors above.
  pause
  exit /b 1
)
echo.
echo Starting the local C++ server...
echo Open http://127.0.0.1:8080 in your browser.
echo Keep this window open while using the application.
echo Press Ctrl+C to stop the server.
echo.
sudoku_server.exe
pause
