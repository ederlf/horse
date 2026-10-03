"""Build Python bindings and their private C runtime through CMake."""
import os
from pathlib import Path
import shutil
import subprocess

from Cython.Build import cythonize
from setuptools import Extension, find_packages, setup
from setuptools.command.build_ext import build_ext

ROOT = Path(__file__).resolve().parent


class HorseBuildExt(build_ext):
    def run(self):
        native = Path(self.build_temp).resolve() / "cmake"
        command = ["cmake", "-S", str(ROOT), "-B", str(native),
                   "-DBUILD_TESTING=OFF", "-DCMAKE_BUILD_TYPE=Release"]
        for name in ("CFLUID", "CMOCKERY"):
            source = os.environ.get("FETCHCONTENT_SOURCE_DIR_" + name)
            if source:
                command.append("-DFETCHCONTENT_SOURCE_DIR_" + name + "=" + source)
        subprocess.check_call(command)
        subprocess.check_call(["cmake", "--build", str(native), "--target", "horse",
                               "--parallel", str(self.parallel or 2)])
        for extension in self.extensions:
            extension.library_dirs = [str(native)]
            extension.include_dirs.append(os.environ.get(
                "FETCHCONTENT_SOURCE_DIR_CFLUID", str(native / "_deps/cfluid-src")))
        super().run()
        # Bundle beside the extensions so wheels and in-place builds need no
        # LD_LIBRARY_PATH or references to the source checkout.
        destination = Path(self.get_ext_fullpath("horse.horse")).resolve().parent
        destination.mkdir(parents=True, exist_ok=True)
        shutil.copy2(native / "libhorse.so", destination / "libhorse.so")

    def get_outputs(self):
        return super().get_outputs() + [str(
            Path(self.get_ext_fullpath("horse.horse")).parent / "libhorse.so")]


extensions = [Extension("horse." + name, ["horse/" + name + ".pyx"],
                        include_dirs=["src", "vendor"], libraries=["horse"],
                        runtime_library_dirs=["$ORIGIN"])
              for name in ("horse", "router")]
setup(name="horse-project", version="0.1.0", python_requires=">=3.10",
      description="Hybrid network simulation with real control software",
      author="Eder Leao Fernandes", license="BSD-3-Clause",
      packages=find_packages(include=["horse", "horse.*"]),
      ext_modules=cythonize(extensions, build_dir="build/cython",
                           compiler_directives={"language_level": 3}),
      cmdclass={"build_ext": HorseBuildExt},
      package_data={"horse": ["*.pxd", "*.pyx", "libhorse.so"]})
