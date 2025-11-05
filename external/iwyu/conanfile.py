from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.files import apply_conandata_patches, export_conandata_patches, get, collect_libs, copy
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
import os

required_conan_version = ">=2.1"

class IncludeWhatYouUseRecipe(ConanFile):
    name = "include-what-you-use"
    package_type = "application"

    # Optional metadata
    license = "LLVM Release License"
    author = "LLVM Team"
    url = "https://github.com/include-what-you-use/include-what-you-use"
    description = "A tool for use with clang to analyze #includes in C and C++ source files"
    topics = ("include", "what", "you", "use", "iwyu", "clang")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
    }

    implements = ["auto_shared_fpic"]
    
    def export_sources(self):
        export_conandata_patches(self)

    def validate(self):
        if self.settings.compiler != "clang":
            raise ConanInvalidConfiguration("include-what-you-use can only be built with clang compiler")
        if self.settings.os == "Windows":
            raise ConanInvalidConfiguration("include-what-you-use is not supported on Windows")

    def layout(self):
        cmake_layout(self, src_folder="src")
   
    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        apply_conandata_patches(self)
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, "*LICENSE*", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = collect_libs(self)
        self.exe = os.path.join(self.package_folder, "bin", "include-what-you-use" + (".exe" if self.settings.os == "Windows" else ""))

