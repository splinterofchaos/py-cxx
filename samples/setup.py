from distutils.core import setup, Extension

cxxflags = ['--std=c++14', '-I../include']

cnt = Extension('count',
                sources = ['countmodule.cpp'],
                extra_compile_args = cxxflags)

cpp = Extension('cpp',
                sources = ['cppmodule.cpp'],
                extra_compile_args = cxxflags)
vec = Extension('vec',
                sources = ['vecmodule.cpp'],
                extra_compile_args = cxxflags)

setup (name = 'cpp',
       version = '1.0',
       ext_modules = [cnt, cpp, vec])
