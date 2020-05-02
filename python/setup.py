import os
import re
import sys
import platform
import subprocess

from setuptools import setup, Extension, Distribution
from setuptools.command.build_ext import build_ext
from distutils.version import LooseVersion


## Based upon https://github.com/pybind/cmake_example/blob/11a644072b12ad78352b6e6649db9dfe7f406676/setup.py#L1

with open('requirements.txt') as f:
  INSTALL_REQUIRES = f.read().splitlines()

SETUP_REQUIRES = ['pytest-runner']
TESTS_REQUIRE = ['pytest']

def run_cmd(cmd):
  cmd = cmd.replace('\n', '').strip()
  subprocess.check_call(cmd, shell=True)

PROTOBAG_CXX_SRC_ROOT = os.environ.get(
  'PROTOBAG_CXX_SRC_ROOT',
  os.path.join(os.path.abspath('.'), '../c++'))

assert os.path.exists(PROTOBAG_CXX_SRC_ROOT), \
  "Couldn't find source root at %s" % PROTOBAG_CXX_SRC_ROOT

class BinaryDistribution(Distribution):
  def has_ext_modules(foo):
    return True

class CMakeExtension(Extension):
  def __init__(self, name, sourcedir=''):
    Extension.__init__(self, name, sources=[])
    self.sourcedir = os.path.abspath(sourcedir)

class CMakeBuild(build_ext):
  def run(self):
    if platform.system() != "Darwin":
      try:
        run_cmd("cmake --version")
      except OSError:
        raise RuntimeError("CMake must be installed to build the following extensions: " +
                 ", ".join(e.name for e in self.extensions))

    for ext in self.extensions:
      self.build_extension(ext)

  def build_extension(self, ext):
    extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
    # required for auto-detection of auxiliary "native" libs
    if not extdir.endswith(os.path.sep):
      extdir += os.path.sep

    cmake_args = [
      '-H' + PROTOBAG_CXX_SRC_ROOT,
      '-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
      '-DPYTHON_EXECUTABLE=' + sys.executable,
    ]

    cfg = 'Debug' if self.debug else 'Release'
    build_args = ['--config', cfg]

    if platform.system() == "Windows":
      cmake_args += ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(cfg.upper(), extdir)]
      if sys.maxsize > 2**32:
        cmake_args += ['-A', 'x64']
      build_args += ['--', '/m']
    elif platform.system() == "Darwin":
      assert False, "TODO run xcode"
    else:
      cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
      build_args += ['--', '-j%s' % os.cpu_count(), 'protobag_native']

    env = os.environ.copy()
    env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(env.get('CXXFLAGS', ''),
                                self.distribution.get_version())
    if not os.path.exists(self.build_temp):
      os.makedirs(self.build_temp)
    subprocess.check_call(['cmake', ext.sourcedir] + cmake_args, cwd=self.build_temp, env=env)
    subprocess.check_call([
      'cmake', '--build', '.'] + build_args,
      cwd=self.build_temp)

setup(
  name='protobag',
  version='0.0.1',
  author='Paul Wais',
  author_email='paul@standardcyborg.com',
  description='Protobag for python',
  long_description='',
  license='Apache License 2.0',
  python_requires=">=3.6",
  
  packages=['protobag'],
  ext_modules=[CMakeExtension('protobag.protobag_native')],
  # include_package_data=True,
  # package_dir={"": "protobag_native"},
  # package_data={
  #   'protobag': ['libprotobag.so'],
  # },
  cmdclass=dict(build_ext=CMakeBuild),
  zip_safe=False,
  distclass=BinaryDistribution,

  install_requires=INSTALL_REQUIRES,
  test_suite='protobag_test',
  setup_requires=SETUP_REQUIRES,
  tests_require=TESTS_REQUIRE,
)
