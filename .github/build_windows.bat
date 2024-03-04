call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
reg add "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\FileSystem" /v LongPathsEnabled /t REG_DWORD /d 1 /f
New-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem" ` -Name "LongPathsEnabled" -Value 1 -PropertyType DWORD -Force
mkdir build
cd build
cmd.exe /c cmake -G "NMake Makefiles" ..
cmake -G "NMake Makefiles" -DBUILD_TEST=TRUE -DBUILD_GSTREAMER_PLUGIN=TRUE ..
nmake
