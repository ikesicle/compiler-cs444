@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "EXE=%~1"
set "TEST_TARGET=%~2"
set "EXIT_CODE=0"

echo Executable: %EXE%

if not exist "%TEST_TARGET%\NUL" (
    set "TARGET_NAME=%~nx2"
    if /I "!TARGET_NAME:~-9!"=="_marmoset" (
        echo Skipping marmoset test: %TEST_TARGET%
        exit /b 0
    ) else (
        echo Error: %TEST_TARGET% is not a directory or a .java file, and is also not a marmoset test that can be skipped
        exit /b 1
    )
)

echo Testing directory: %TEST_TARGET%
pushd "%TEST_TARGET%" || exit /b 1

if not exist "..\java_all.txt" (
    echo Error: Missing ..\java_all.txt
    popd
    exit /b 1
)

set "JAVA_ALL_FILES="
for /f "usebackq delims=" %%L in ("..\java_all.txt") do (
    if not "%%L"=="" (
        set "JAVA_ALL_FILES=!JAVA_ALL_FILES! "..\%%L""
    )
)

for %%I in (*) do (
    set "ITEM=%%~nxI"

    rem Skip helper entries that the bash script ignores.
    if /I not "!ITEM!"=="java" if /I not "!ITEM!"=="java_all.txt" (
        set "FILES="

        if exist "%%I\NUL" (
            for /r "%%I" %%F in (*.java) do (
                set "FILES=!FILES! "%%F""
            )
        ) else (
            if /I "%%~xI"==".java" (
                set "FILES="%%I""
            )
        )

        if defined FILES (
            "%EXE%" !FILES! !JAVA_ALL_FILES! >nul 2>nul
            set "RUN_STATUS=!errorlevel!"
            set "IS_JE=0"
            if /I "!ITEM:~0,2!"=="Je" set "IS_JE=1"

            if "!RUN_STATUS!"=="0" if "!IS_JE!"=="1" (
                echo Error: Test !ITEM! expected to fail but passed ^(exit code 0^)
                set "EXIT_CODE=1"
            )
            if "!RUN_STATUS!"=="42" if not "!IS_JE!"=="1" (
                echo Error: Test !ITEM! expected to pass but failed ^(exit code !RUN_STATUS!^)
                set "EXIT_CODE=1"
            )
            if not "!RUN_STATUS!"=="0" if not "!RUN_STATUS!"=="42" (
                echo Test !ITEM! failed due to an internal error, with exit code !RUN_STATUS!^)
                set "EXIT_CODE=1"
            )
        )
    )
)

popd
exit /b %EXIT_CODE%
