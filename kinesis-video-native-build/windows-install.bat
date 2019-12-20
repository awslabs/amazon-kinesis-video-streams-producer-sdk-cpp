if "%1" == "" (
    echo [91m"Please run install command with bitwidth (32/64): windows-install.bat 64"[0m
    exit /b
)
SET BITWIDTH=%1
IF %BITWIDTH%==32 (
    SET PLATFORM=x86
    SET BUILD_BITNESS_TYPE=Win32
    SET BUILD_BITNESS_NAME=x86
    echo "Platform is x86 32 bit"
) ELSE (
    IF %BITWIDTH%==64 (
        SET PLATFORM=x64
        SET BUILD_BITNESS_TYPE=x64
        SET BUILD_BITNESS_NAME=x86_64
        echo "Platform is x86 64 bit"
    ) ELSE (
        echo [91m"Please run install command with bitwidth (32/64): windows-install.bat 64"[0m
        exit /b
    )
)

if not exist "Release" mkdir Release
if not exist "downloads" mkdir downloads
cd downloads
SET "CURRENTDIR=%cd%"

set VS150COMNTOOLS=C:\BuildTools\Common7\Tools
set VCTargetsPath=C:\BuildTools\Common7\IDE\VC\VCTargets


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
echo "%path%"|find /i "C:\BuildTools\VC\Tools\MSVC\14.14.26428"
if ERRORLEVEL 1 (
    IF %BITWIDTH%==32 (
        set "path=%path%;C:\BuildTools\VC\Tools\MSVC\14.14.26428\bin\Hostx86\x86"
    ) ELSE (
        set "path=%path%;C:\BuildTools\VC\Tools\MSVC\14.14.26428\bin\Hostx64\x64"
    )
)

if not exist "jsmn-1.0.0" (
    echo "Jsmn doesn't exist, installing into %CURRENTDIR%"
    if not exist "jsmn-1.0.0.zip" (
        powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri https://github.com/zserge/jsmn/archive/v1.0.0.zip -OutFile jsmn-1.0.0.zip"
    )
    powershell -NoP -NonI -Command "Expand-Archive '.\jsmn-1.0.0.zip' '.\'"
    copy "%CURRENTDIR%\..\projectFiles\jsmn\CMakeLists.txt" "%CURRENTDIR%\jsmn-1.0.0\" /y
    cd jsmn-1.0.0
    IF %BITWIDTH%==32 (
        cmake -G "Visual Studio 15 2017"
    ) ELSE (
        cmake -G "Visual Studio 15 2017 Win64"
    )
    msbuild jsmn.vcxproj  /p:Configuration=Release /p:Platform=%BUILD_BITNESS_TYPE% /m
    cd "%CURRENTDIR%\"
)
echo "Jsmn installed"

if not exist "curl-7.60.0" (
    echo "Curl doesn't exist, installing into %CURRENTDIR%"
    if not exist "curl-7.60.0.zip" (
        powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri https://curl.haxx.se/download/curl-7.60.0.zip -OutFile curl-7.60.0.zip"
    )
    powershell -NoP -NonI -Command "Expand-Archive '.\curl-7.60.0.zip' '.\'"
    cd "%CURRENTDIR%\curl-7.60.0\winbuild"
    nmake /f Makefile.vc mode=static RTLIBCFG=static VC=15 MACHINE=%PLATFORM% DEBUG=no ENABLE_IPV6=no clean
    nmake /f Makefile.vc mode=static RTLIBCFG=static VC=15 MACHINE=%PLATFORM% DEBUG=no ENABLE_IPV6=no 
    if ERRORLEVEL 1 goto :showerror

    cd "%CURRENTDIR%\"
)
echo "Curl installed"

if not exist "googletest-release-1.8.0" (
    echo "GoogleTest doesn't exist, installing into %CURRENTDIR%"
    if not exist "google-test-1.8.0.zip" (
        powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri https://github.com/google/googletest/archive/release-1.8.0.zip -OutFile google-test-1.8.0.zip"
    )
    powershell -NoP -NonI -Command "Expand-Archive '.\google-test-1.8.0.zip' '.\'"
    copy "%CURRENTDIR%\..\projectFiles\googletest\*.*" "%CURRENTDIR%\googletest-release-1.8.0\googletest\msvc" /y
    cd "%CURRENTDIR%\googletest-release-1.8.0\googletest\msvc"
    msbuild gtest.sln /p:Configuration=Release /p:Platform=%PLATFORM%
    if ERRORLEVEL 1 goto :showerror

    cd "%CURRENTDIR%\"
)
echo "GoogleTest installed"

if not exist "log4cplus-1.2.1" (
    echo "Log4Cplus doesn't exist, installing into %CURRENTDIR%"
    if not exist "log4cplus-1.2.1.zip" (
        powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri https://github.com/log4cplus/log4cplus/releases/download/REL_1_2_1/log4cplus-1.2.1.zip -OutFile log4cplus-1.2.1.zip"
    )
    powershell -NoP -NonI -Command "Expand-Archive '.\log4cplus-1.2.1.zip' '.\'"
    copy "%CURRENTDIR%\..\projectFiles\log4cplus\*.*" "%CURRENTDIR%\log4cplus-1.2.1\msvc10" /y
    copy "%CURRENTDIR%\..\projectFiles\log4cplus\tests\*.*" "%CURRENTDIR%\log4cplus-1.2.1\msvc10\tests" /y
    cd "%CURRENTDIR%\log4cplus-1.2.1\msvc10"
    msbuild log4cplus.sln /p:Configuration=Release /p:Platform=%BUILD_BITNESS_TYPE% /m
    if ERRORLEVEL 1 goto :showerror

    cd "%CURRENTDIR%\"
)
echo "Log4Cplus installed"


if not exist "openssl-OpenSSL_1_1_0f" (
    echo "Openssl doesn't exist, installing into %CURRENTDIR%"
    if not exist "openssl-1.1.0f.zip" (
        powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest https://github.com/openssl/openssl/archive/OpenSSL_1_1_0f.zip -OutFile openssl-1.1.0f.zip"
    )
    if not exist "strawberry-perl-5.28.2.1-%BITWIDTH%bit.msi" (
        powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri http://strawberryperl.com/download/5.28.2.1/strawberry-perl-5.28.2.1-%BITWIDTH%bit.msi -OutFile strawberry-perl-5.28.2.1-%BITWIDTH%bit.msi"
    )
    if not exist "nasm-2.13-win%BITWIDTH%.zip" (
        powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri https://www.nasm.us/pub/nasm/releasebuilds/2.13/win%BITWIDTH%/nasm-2.13-win%BITWIDTH%.zip -OutFile nasm-2.13-win%BITWIDTH%.zip"
    )

    powershell -NoP -NonI -Command "Expand-Archive '.\openssl-1.1.0f.zip' '.\'"
    powershell -NoP -NonI -Command "Expand-Archive '.\nasm-2.13-win%BITWIDTH%.zip' '.\'"

    msiexec /x strawberry-perl-5.28.2.1-%BITWIDTH%bit.msi /qn
    msiexec /i strawberry-perl-5.28.2.1-%BITWIDTH%bit.msi INSTALLDIR="%CURRENTDIR%\StrawberryPerl" /qb

    cd "%CURRENTDIR%\openssl-OpenSSL_1_1_0f"

    set "path=%CURRENTDIR%\nasm-2.13;%path%"

    IF %BITWIDTH%==32 (
        "%CURRENTDIR%\StrawberryPerl\perl\bin\perl" Configure VC-WIN32 --openssldir="%CURRENTDIR%\openssl-OpenSSL_1_1_0f\OpenSSL" --prefix="%CURRENTDIR%\openssl-OpenSSL_1_1_0f\OpenSSL"
    ) ELSE (
        "%CURRENTDIR%\StrawberryPerl\perl\bin\perl" Configure VC-WIN64A --openssldir="%CURRENTDIR%\openssl-OpenSSL_1_1_0f\OpenSSL" --prefix="%CURRENTDIR%\openssl-OpenSSL_1_1_0f\OpenSSL"
    )
    nmake
    if ERRORLEVEL 1 goto :showerror
    nmake install
    if ERRORLEVEL 1 goto :showerror

    cd "%CURRENTDIR%\"
)
copy openssl-OpenSSL_1_1_0f\libcrypto-1_1-x64.dll ..\Release\ /Y
copy openssl-OpenSSL_1_1_0f\libssl-1_1-x64.dll ..\Release\ /Y
echo "Openssl installed"


if not exist "gstreamer" (
    echo "Installing GStreamer into %CURRENTDIR%"
    if not exist "gstreamer-1.0-devel-%BUILD_BITNESS_NAME%-1.14.1.msi" (
        powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri https://gstreamer.freedesktop.org/data/pkg/windows/1.14.1/gstreamer-1.0-devel-%BUILD_BITNESS_NAME%-1.14.1.msi -OutFile gstreamer-1.0-devel-%BUILD_BITNESS_NAME%-1.14.1.msi"
    )
    if not exist "gstreamer-1.0-%BUILD_BITNESS_NAME%-1.14.1.msi" (
        powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri https://gstreamer.freedesktop.org/data/pkg/windows/1.14.1/gstreamer-1.0-%BUILD_BITNESS_NAME%-1.14.1.msi -OutFile gstreamer-1.0-%BUILD_BITNESS_NAME%-1.14.1.msi"
    )
    msiexec /x gstreamer-1.0-devel-%BUILD_BITNESS_NAME%-1.14.1.msi /qn
    msiexec /x gstreamer-1.0-%BUILD_BITNESS_NAME%-1.14.1.msi /qn
    msiexec /i gstreamer-1.0-devel-%BUILD_BITNESS_NAME%-1.14.1.msi TARGETDIR="%CURRENTDIR%\" ADDLOCAL=ALL /qb
    msiexec /i gstreamer-1.0-%BUILD_BITNESS_NAME%-1.14.1.msi TARGETDIR="%CURRENTDIR%\" ADDLOCAL=ALL /qb
)
set "path=%path%;%CURRENTDIR%\gstreamer\1.0\%BUILD_BITNESS_NAME%\bin"
echo "GStreamer installed"


echo "Building Producer SDK"
cd "%CURRENTDIR%"
cd ..
IF %BITWIDTH%==32 (
    cmake -G "Visual Studio 15 2017"
) ELSE (
    cmake -G "Visual Studio 15 2017 Win64"
)

msbuild pic_test.vcxproj  /p:Configuration=Release /p:Platform=%BUILD_BITNESS_TYPE% /m
if ERRORLEVEL 1 goto :showerror
msbuild cproducer.vcxproj  /p:Configuration=Release /p:Platform=%BUILD_BITNESS_TYPE% /m
if ERRORLEVEL 1 goto :showerror
msbuild cproducer_test.vcxproj  /p:Configuration=Release /p:Platform=%BUILD_BITNESS_TYPE% /m
if ERRORLEVEL 1 goto :showerror
msbuild producer.vcxproj  /p:Configuration=Release /p:Platform=%BUILD_BITNESS_TYPE% /m
if ERRORLEVEL 1 goto :showerror
msbuild producer_test.vcxproj  /p:Configuration=Release /p:Platform=%BUILD_BITNESS_TYPE% /m
if ERRORLEVEL 1 goto :showerror
msbuild kinesis_video_cproducer_video_only_sample.vcxproj  /p:Configuration=Release /p:Platform=%BUILD_BITNESS_TYPE% /m
if ERRORLEVEL 1 goto :showerror
msbuild kinesis_video_gstreamer_sample_app.vcxproj  /p:Configuration=Release /p:Platform=%BUILD_BITNESS_TYPE% /m
if ERRORLEVEL 1 goto :showerror
msbuild kinesis_video_gstreamer_audio_video_sample_app.vcxproj  /p:Configuration=Release /p:Platform=%BUILD_BITNESS_TYPE% /m
if ERRORLEVEL 1 goto :showerror
msbuild kinesis_video_gstreamer_sample_multistream_app.vcxproj  /p:Configuration=Release /p:Platform=%BUILD_BITNESS_TYPE% /m
if ERRORLEVEL 1 goto :showerror
msbuild gstkvssink.vcxproj /p:Configuration=Release /p:Platform=%BUILD_BITNESS_TYPE% /m
if ERRORLEVEL 1 goto :showerror
msbuild kvs_producer_plugin_demo.vcxproj /p:Configuration=Release /p:Platform=%BUILD_BITNESS_TYPE% /m
if ERRORLEVEL 1 goto :showerror
msbuild kvs_producer_plugin_rtsp_demo.vcxproj /p:Configuration=Release /p:Platform=%BUILD_BITNESS_TYPE% /m
if ERRORLEVEL 1 goto :showerror

copy kvs_log_configuration Release\
if not exist "release\samples" (
    xcopy samples release\samples /E /I
)
exit /b

:showerror
cd "%CURRENTDIR%\..\"
echo [91mBuild error occurred! Please delete related directories in downloads\ to start a clean build. [0m
exit /b1
