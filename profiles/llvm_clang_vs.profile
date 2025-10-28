# llvm_clang_vs profile for Windows using Clang with Visual Studio runtime
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
tools.compilation:verbosity=verbose
