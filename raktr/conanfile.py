import os
from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import (
    apply_conandata_patches,
    export_conandata_patches,
    collect_libs,
    copy,
)

required_conan_version = ">=2"


class RaktrConan(ConanFile):
    name = "raktr"
    url = "https://github.com/ovaar/Raktr"
    homepage = "https://github.com/ovaar/Raktr"
    description = "Raktr is a real-time 3D rendering engine written in C++"
    topics = ("graphics", "renderer", "3d")
    license = "Apache-2.0"
    settings = "os", "arch", "compiler", "build_type"

    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_tests": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_tests": False,
    }

    implements = ["auto_shared_fpic"]

    exports_sources = "CMakeLists.txt", "src/*", "include/*", "test/*"

    def export_sources(self):
        export_conandata_patches(self)

    def build_requirements(self):
        self.tool_requires("cmake/[>=4.0 <5]")
        self.tool_requires("ninja/[>=1.12.1 <2]")
        self.tool_requires("ccache/[>=4.11 <5]")

    def requirements(self):
        self.requires("mimalloc/2.2.4")
        self.requires("meshoptimizer/0.25")
        self.requires("glm/1.0.1")
        self.requires("fmt/12.0.0")
        self.requires("spdlog/1.16.0")
        self.requires("glfw/3.4")
        self.requires("wgpu-native/27.0.2.0")

        self.test_requires("gtest/1.17.0")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["USE_CCACHE"] = True
        tc.variables["BUILD_TESTS"] = bool(self.options.with_tests)
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        apply_conandata_patches(self)
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(
            self,
            pattern="LICENSE",
            dst=os.path.join(self.package_folder, "licenses"),
            src=self.source_folder,
        )
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = collect_libs(self)
