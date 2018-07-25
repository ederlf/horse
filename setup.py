from distutils.core import setup, Extension
from Cython.Build import cythonize
import os.path

include_dirs = [os.path.abspath("src/"), os.path.abspath("vendor/")]
library_dirs = [os.path.abspath(".libs"), os.path.abspath("vendor/loci")]

extensions = [
        Extension("horse.horse", ["horse/horse.pyx"],
            include_dirs=include_dirs,
            library_dirs=library_dirs,
            libraries=["horse"]),
            Extension("horse.router", ["horse/router.pyx"],
            include_dirs=include_dirs,
            library_dirs=library_dirs,
            libraries=["horse"])
        ]


setup(name='horse-project',
      version='0.1',
      author='Eder Leao Fernandes',
      ext_modules=cythonize(extensions),
      packages=["horse", "horse.bgp_peer"],
      license='BSD',
      package_data={'horse': ['__init__.pxd',
                                 'horse.pxd','router.pxd', 
                                 'router.c' 'horse.c']},
     )
