# llvm_clang.profile for Windows using LLVM Clang with GNU-like frontend (clang++)
# Links against MSVC runtime but uses GNU-like command line syntax
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

[buildenv]
PATH=+(path)C:/Program Files/LLVM/bin

[conf]
tools.cmake.cmaketoolchain:generator=Ninja
tools.compilation:verbosity=verbose

[tool_requires]
ninja/[*]
