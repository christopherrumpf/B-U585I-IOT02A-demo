from distutils.core import setup, Extension

module1 = Extension('coreio',
                    sources = ['coreiomodule.c', 'coreio.c', 'corehdl-base.c'],
                    extra_compile_args = ['-std=c11', '-D_POSIX_C_SOURCE=199309L', '-Wno-unused-result']
                    )

setup (name = 'Corellium CoreIO',
       version = '0.1',
       description = 'vMMIO CoreIO Module',
       ext_modules = [module1])
