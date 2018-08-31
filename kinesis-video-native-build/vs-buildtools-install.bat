echo "Installing Microsoft Visual Studio Build Tools"
if not exist "vs_buildtools.exe" (
     powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri https://aka.ms/vs/15/release/vs_buildtools.exe -OutFile vs_buildtools.exe"
)
vs_buildtools.exe --wait --quiet --nocache --installPath C:\BuildTools --add Microsoft.Component.MSBuild --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 --add Microsoft.VisualStudio.Component.VC.140 --add Microsoft.VisualStudio.Component.VC.Modules.x86.x64 --add Microsoft.VisualStudio.Component.VC.Runtimes.x86.x64.Spectre --add Microsoft.Component.VC.Runtime.UCRTSDK --add Microsoft.VisualStudio.Component.Windows10SDK --add Microsoft.VisualStudio.Component.Windows10SDK.17134 --add Microsoft.VisualStudio.Component.VC.CoreBuildTools
echo "Microsoft Visual Studio Build Tools installed. Please reboot to get the visual studio build tool environment available for building the SDK."
exit /b0
