REM Build script for GBEMU
@ECHO OFF
SetLocal EnableDelayedExpansion

REM Get a list of all the .c files.
SET cFilenames=
FOR /R %%f in (*.c) do (
    SET cFilenames=!cFilenames! %%f
)

REM echo "Files:" %cFilenames%

SET assembly=gbemu
SET compilerFlags=-g -Wall -Werror -Werror=vla
REM -Wall -Werror
SET includeFlags=-Isrc -Ilib/glad/include/ -isystem"C:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/ucrt" -isystem"C:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/um" -isystem"C:/Program Files (x86)/Windows Kits/10/Include/10.0.22621.0/shared"
SET linkerFlags= -L"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/um/x64" -L"C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/ucrt/x64" -luser32 -lOpengl32 -lGdi32
SET defines=-D_DEBUG -D_CRT_SECURE_NO_WARNINGS

ECHO "Building %assembly%%..."
clang -m64 %cFilenames% %compilerFlags% -o bin/%assembly%.exe %defines% %includeFlags% %linkerFlags%