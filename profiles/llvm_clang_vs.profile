# llvm_clang_vs profile for Windows using MSVC Clang component (ClangCL Visual Studio toolset)
# Uses Visual Studio's bundled Clang with MSVC runtime
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

[conf]
tools.cmake.cmaketoolchain:generator=Visual Studio 17
tools.compilation:verbosity=verbose
