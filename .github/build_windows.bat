call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
mkdir build
cd build
update PKG_CONFIG_EXECUTABLE in CMakeLists.txt to match the actual path to pkg-config.exe (i.e. default: PKG_CONFIG_EXECUTABLE "C:\\gstreamer\\1.0\\x86_64\\bin\\pkg-config.exe", updated: PKG_CONFIG_EXECUTABLE "C:\\gstreamer\\1.0\\msvc_x86_64\\bin\\pkg-config.exe")
cmd.exe /c cmake -G "NMake Makefiles" ..
cmake -G "NMake Makefiles" -DBUILD_TEST=TRUE -DBUILD_GSTREAMER_PLUGIN=TRUE ..
nmake
