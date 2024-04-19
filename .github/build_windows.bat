call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
mkdir build
cd build
cmd.exe /c cmake -G "NMake Makefiles" ..
cmake -G "NMake Makefiles" -DBUILD_TEST=TRUE -DBUILD_GSTREAMER_PLUGIN=TRUE -DPKG_CONFIG_EXECUTABLE="D:\\gstreamer\\1.0\\msvc_x86_64\\bin\\pkg-config.exe" ..
nmake
