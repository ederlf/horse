from distutils.core import setup, Extension
from Cython.Build import cythonize
import os.path


extensions = [
        Extension("*", ["horse.pyx"],
            include_dirs=[os.path.abspath("../src/"), os.path.abspath("../vendor/")],
            library_dirs=[os.path.abspath("../.libs")],
            libraries=["horse"])
        ]

setup(name='cython-test-project',
      version='1.0',
      author='Holger Peters',
      ext_modules=cythonize(extensions)
     )
