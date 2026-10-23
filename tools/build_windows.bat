@echo off
setlocal

cmake -S . -B build
if errorlevel 1 exit /b 1

cmake --build build --config Debug
if errorlevel 1 exit /b 1

ctest --test-dir build -C Debug --output-on-failure
exit /b %errorlevel%