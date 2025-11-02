# llvm_clang_cl.profile for Windows using LLVM Clang with MSVC-like frontend (clang-cl)
# Links against MSVC runtime and uses MSVC-like command line syntax
[settings]
arch=x86_64
os=Windows
compiler=clang
build_type=Release
compiler.cppstd=23
compiler.version=20
compiler.runtime=dynamic
compiler.runtime_type=Release
compiler.runtime_version=v144

wgpu-native*:compiler=msvc
wgpu-native*:compiler.version=194
wgpu-native*:compiler.runtime=dynamic
wgpu-native*:compiler.runtime_type=Release

[buildenv]
PATH=+(path)C:/Program Files/LLVM/bin

[conf]
tools.cmake.cmaketoolchain:generator=Ninja
tools.build:compiler_executables={"c": "clang-cl", "cpp": "clang-cl"}
tools.compilation:verbosity=verbose

[tool_requires]
ninja/[*]
