# msvc_vs.profile for Windows using MSVC
[settings]
arch=x86_64
os=Windows
compiler=msvc
build_type=Release
compiler.cppstd=23
compiler.version=194
compiler.runtime=dynamic

wgpu-native*:compiler=msvc
wgpu-native*:compiler.version=194
wgpu-native*:compiler.runtime=dynamic
wgpu-native*:compiler.runtime_type=Release
wgpu-native*:build_type=Release


[conf]
tools.cmake.cmaketoolchain:generator=Visual Studio 17
tools.compilation:verbosity=verbose
