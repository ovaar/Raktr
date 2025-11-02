from conan import ConanFile
from conan.tools.files import get
from conan.tools.files import collect_libs


class wgpu_nativeRecipe(ConanFile):
    name = "wgpu-native"
    package_type = "shared-library"

    # Optional metadata
    license = "MIT/Apache-2.0"
    author = "gfx-rs"
    url = "https://github.com/gfx-rs/wgpu-native"
    description = "Native WebGPU implementation based on wgpu-core"
    topics = ("graphics", "wgpu", "webgpu")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"

    no_copy_source = True
    build_policy = "never"  # This package cannot be built from sources, it is always created with conan export-pkg
    virtualbuildenv = False
    virtualrunenv = False


    def get_prebuilt_url(self) -> str:
        version = str(self.version)
        arch = str(self.settings.arch)
        os = str(self.settings.os).lower()
        compiler = str(self.settings.compiler)
        build_type = str(self.settings.build_type).lower()
        arch = str(self.settings.arch)
        return f"https://github.com/gfx-rs/wgpu-native/releases/download/v{version}/wgpu-{os}-{arch}-{compiler}-{build_type}.zip"

    def package(self):
        get(self, url=self.get_prebuilt_url(), destination=self.package_folder)

    def package_info(self):
        self.cpp_info.libs = collect_libs(self)

