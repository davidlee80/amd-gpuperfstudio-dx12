# amd-gpuperfstudio-dx12
DirectX 12 plugin for AMD GPU PerfStudio tool for graphics developers

Building the DX12Server plugin for GPU PerfStudio
------------------------------------------------------
The Visual Studio 2015 solution file is located at: "GPUPerfStudio\Build\VS2015\PerfStudio-Server-2015.sln"

The PerfStudio-Server-2015.sln solution file can be used to build the following 32bit and 64bit release and debug dlls:

 - *DX12Server.dll*		(Release 32bit DX12 server)
 - *DX12Server-d.dll*		(Debug 32bit DX12 server)
 - *DX12Server-x64.dll* 	(Release 64bit DX12 server)
 - *DX12Server-x64-d.dll*	(Debug 64bit DX12 server)

These dlls can be used to replace those supplied with the publicly released version of GPU PerfStudio at:

http://developer.amd.com/tools-and-sdks/graphics-development/gpu-perfstudio/

Build Instructions
------------------------------------------------------

You will need the following:

	1) Visual Studio 2015 Update 1
	2) Windows10 SDK - Please note that the Windows Kits SDK version must be version 10.0.10586.0 
		e.g: C:\Program Files (x86)\Windows Kits\10\Lib\10.0.10586.0. 

Edit the Global-WindowsSDK.props file and set the WindowsSDKDir field to point to the location of the Windows10 SDK on your system. 

The Global-WindowsSDK.props file is located at: "Common\Lib\Ext\Windows-Kits\Global-WindowsSDK.props"

By default the WindowsSDKDir is set to: <WindowsSDKDir>C:\Program Files (x86)\Windows Kits\10</WindowsSDKDir>

Open PerfStudio-Server-2015.sln in Visual Studio 2015 and build DX12Server configuration you require. 
Currently, the solution supports 32bit, 64bit, release and debug dlls.

The Release dlls are output to the following directory:

	GPUPerfStudio\bin\Release\Plugins\DX12Server.dll
	GPUPerfStudio\bin\Release\Plugins\DX12Server-x64.dll

The Debug dlls are output to the following directory:

	GPUPerfStudio\bin\Debug\Plugins\DX12Server-d.dll
	GPUPerfStudio\bin\Debug\Plugins\DX12Server-x64-d.dll


Using the DLLs with GPU PerfStudio
------------------------------------------------------

GPU PerfStudio is available in two forms: 

	1) A zip file which can be expanded anywhere on your disk
	2) An MSI installer that will default to: C:\Program Files (x86)\AMD\GPU PerfStudio

Both versions will contain a "Plugins" directory, e.g.:

	C:\Program Files (x86)\AMD\GPU PerfStudio\Plugins

Replace the DX12Server dlls in the Plugins directory with the versions that you build from the PerfStudio-Server-2015.sln

To use the debug dlls you will need to remove the "-d" from the debug dll names in order for GPU PerfStudio to pick them up. 




