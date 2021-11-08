# Splashes Of Skyrim

[SKSE64 plugin](https://www.nexusmods.com/skyrimspecialedition/mods/47710?tab=description) that adds projectile based splashes and explosions to water surfaces not covered by vanilla game.

[SKSEVR version](https://www.nexusmods.com/skyrimspecialedition/mods/58104)

## Requirements
* [CMake](https://cmake.org/)
	* Add this to your `PATH`
* [PowerShell](https://github.com/PowerShell/PowerShell/releases/latest)
* [Vcpkg](https://github.com/microsoft/vcpkg)
	* Add the environment variable `VCPKG_ROOT` with the value as the path to the folder containing vcpkg
* [Visual Studio Community 2019](https://visualstudio.microsoft.com/)
	* Desktop development with C++
* [CommonLibSSE](https://github.com/powerof3/CommonLibSSE/tree/dev)
	* You need to build from the powerof3/dev branch
	* Add this as as an environment variable `CommonLibSSEPath`
* [CommonLibVR](https://github.com/alandtse/CommonLibVR/tree/vr)
	* Add this as as an environment variable `CommonLibVRPath`

## Register Visual Studio as a Generator
* Open `x64 Native Tools Command Prompt`
* Run `cmake`
* Close the cmd window

## Building

```
git clone https://github.com/powerof3/Splashes-of-Skyrim.git
cd Splashes-of-Skyrim
```

### SSE
```
cmake -B build -S .
```
Open build/po3_SplashesOfSkyrim.sln in Visual Studio to build dll.

### VR
```
cmake -B build2 -S . -DBUILD_SKYRIMVR=On
```
Open build/po3_SplashesOfSkyrim.sln in Visual Studio to build dll.


## License
[MIT](LICENSE)
