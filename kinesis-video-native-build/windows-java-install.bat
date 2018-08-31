SET "CURRENTDIR=%cd%"
SET BITWIDTH=%1
IF NOT DEFINED BITWIDTH SET BITWIDTH=64
IF %BITWIDTH%==32 (
    SET PLATFORM=x86
    SET BUILD_BITNESS_TYPE=Win32
    SET BUILD_BITNESS_NAME=x86
    echo "Platform is x86 32 bit"
) ELSE (
    SET PLATFORM=x64
    SET BUILD_BITNESS_TYPE=x64
    SET BUILD_BITNESS_NAME=x86_64
    echo "Platform is x86 64 bit"
)

if not exist "cmake-3.12.0-win%BITWIDTH%-%PLATFORM%" (
    echo "CMake not found. Installing CMake"
    if not exist "cmake-3.12.0-win%BITWIDTH%-%PLATFORM%.zip" (
        powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri https://cmake.org/files/v3.12/cmake-3.12.0-win%BITWIDTH%-%PLATFORM%.zip -OutFile cmake-3.12.0-win%BITWIDTH%-%PLATFORM%.zip"
    )
    powershell -NoP -NonI -Command "Expand-Archive '.\cmake-3.12.0-win%BITWIDTH%-%PLATFORM%.zip' '.\'"
)
echo "CMake installed"
echo "%path%"|find /i "%CURRENTDIR%\cmake-3.12.0-win%BITWIDTH%-%PLATFORM%\bin" || SET "PATH=%CURRENTDIR%\cmake-3.12.0-win%BITWIDTH%-%PLATFORM%\bin;%PATH%"

echo "%path%"|find /i "C:\BuildTools\VC\Tools\MSVC\14.14.26428" || CALL "C:\BuildTools\VC\Auxiliary\Build\vcvars%BITWIDTH%.bat"

IF %BITWIDTH%==32 (
    cmake -G "Visual Studio 15 2017"
) ELSE (
    cmake -G "Visual Studio 15 2017 Win64"
)
msbuild KinesisVideoProducerJNI.vcxproj /p:Configuration=Release /p:Platform=%BUILD_BITNESS_TYPE% /m
if ERRORLEVEL 1 goto :showerror
exit /b

:showerror
cd "%CURRENTDIR%\"
echo [91mBuild error occurred[0m
exit /b1
