#!/usr/bin/python3.3
# -*- coding: utf-8 -*-
"""setup script for pexe37.
"""

import os
import sys

if sys.version_info < (3, 3):
    raise RuntimeError("This package requires Python 3.3 or later")

############################################################################

from setuptools import setup
##from distutils.core import setup

from pexe37.pexe37_distutils import Dist, Interpreter, BuildInterpreters

############################################################################

def _is_debug_build():
    import imp
    for ext, _, _ in imp.get_suffixes():
        if ext == "_d.pyd":
            return True
    return False

if _is_debug_build():
    macros = [("PYTHONDLL", '\\"python%d%d_d.dll\\"' % sys.version_info[:2]),
##              ("PYTHONCOM", '\\"pythoncom%d%d_d.dll\\"' % sys.version_info[:2]),
              ("_CRT_SECURE_NO_WARNINGS", '1')]
else:
    macros = [("PYTHONDLL", '\\"python%d%d.dll\\"' % sys.version_info[:2]),
##              ("PYTHONCOM", '\\"pythoncom%d%d.dll\\"' % sys.version_info[:2]),
              ("_CRT_SECURE_NO_WARNINGS", '1'),]

macros.append(("Py_BUILD_CORE", '1'))

extra_compile_args = []
extra_link_args = []

extra_compile_args.append("-IC:\\Program Files\\Microsoft SDKs\\Windows\\v7.0\\Include")
extra_compile_args.append("-IC:\\Program Files (x86)\\Microsoft Visual Studio 14.0\\VC\\include")
extra_compile_args.append("-IC:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.10586.0\\ucrt")

if 0:
    extra_compile_args.append("/Od")
    extra_compile_args.append("/Z7")
    extra_link_args.append("/DEBUG")
    macros.append(("VERBOSE", "1"))

run_ctypes_dll = Interpreter("pexe37.run_ctypes_dll",
                             ["source/run_ctypes_dll.c",
                              "source/start.c",
                              "source/icon.rc",

                              "source/MemoryModule.c",
                              "source/MyLoadLibrary.c",
                              "source/_memimporter.c",
                              "source/actctx.c",

                              "source/python-dynload.c",
                              ],
                             libraries=["user32", "shell32"],
                             export_symbols=["DllCanUnloadNow,PRIVATE",
                                             "DllGetClassObject,PRIVATE",
                                             "DllRegisterServer,PRIVATE",
                                             "DllUnregisterServer,PRIVATE",
                                             ],
                             target_desc = "shared_library",
                             define_macros=macros,
                             extra_compile_args=extra_compile_args,
                             extra_link_args=extra_link_args + ["/DLL"],
                             )

run = Interpreter("pexe37.run",
                  ["source/run.c",
                   "source/start.c",
                   "source/icon.rc",

                   "source/MemoryModule.c",
                   "source/MyLoadLibrary.c",
                   "source/_memimporter.c",
                   "source/actctx.c",

                   "source/python-dynload.c",
                   ],
                  libraries=["user32", "shell32"],
                  define_macros=macros,
                  extra_compile_args=extra_compile_args,
                  extra_link_args=extra_link_args,
                  )

run_w = Interpreter("pexe37.run_w",
                    ["source/run_w.c",
                     "source/start.c",
                     "source/icon.rc",

                     "source/MemoryModule.c",
                     "source/MyLoadLibrary.c",
                     "source/_memimporter.c",
                     "source/actctx.c",

                     "source/python-dynload.c",
                     ],
                    libraries=["user32", "shell32"],
                    define_macros=macros,
                    extra_compile_args=extra_compile_args,
                    extra_link_args=extra_link_args,
                    )


resource_dll = Interpreter("pexe37.resources",
                           ["source/dll.c",
                            "source/icon.rc"],
                           target_desc = "shared_library",
                           extra_link_args=["/DLL"],
                           )

interpreters = [run, run_w, resource_dll,
                run_ctypes_dll]


if __name__ == "__main__":
    import pexe37

    cmdclass = {'build_interpreters': BuildInterpreters}

    setup(name="pexe37",
          version=pexe37.__version__,
          description="Python to Exe",
          author="darkArp",
          author_email="marionascimento@itsec.bz",
          license="MIT/X11",
          platforms="Windows",
          download_url=f"https://github.com/darkarp/pexe37/archive/v{pexe37.__version__}.zip",

          classifiers=[
              "Development Status :: 4 - Beta",
              "Environment :: Console",
              "License :: OSI Approved :: MIT License",
              "License :: OSI Approved :: Mozilla Public License 2.0 (MPL 2.0)",
              "Operating System :: Microsoft :: Windows",
              "Programming Language :: C",
              "Programming Language :: Python :: 3",
              "Programming Language :: Python :: 3.7",
              "Programming Language :: Python :: Implementation :: CPython",
              "Topic :: Software Development",
              "Topic :: Software Development :: Libraries",
              "Topic :: Software Development :: Libraries :: Python Modules",
              "Topic :: System :: Software Distribution",
              "Topic :: Utilities",
              ],

          distclass = Dist,
          cmdclass = cmdclass,
          entry_points = {
              'console_scripts': ['build_exe = pexe37.build_exe:main'],
              },
          interpreters = interpreters,
          py_modules=['zipextimporter'],
          packages=['pexe37'],
          include_package_data=True
          )
