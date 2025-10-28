# msvc_vs.profile for Windows using MSVC
[settings]
arch=x86_64
os=Windows
compiler=msvc
build_type=Release
compiler.cppstd=23
compiler.version=194
compiler.runtime=dynamic

[conf]
tools.cmake.cmaketoolchain:generator=Ninja
tools.compilation:verbosity=verbose
