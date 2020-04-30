
Pod::Spec.new do |spec|
  spec.name          = 'ProtobagPyNative'
  spec.version       = '0.0.1'
  spec.license       = { :type => 'BSD' }
  spec.homepage      = 'https://github.com/pybind/pybind11'
  spec.authors       = {
    'PyBind11C++' => 'paul@standardcyborg.com',
    'pybind11' =>    'https://github.com/pybind/pybind11'
  }
  spec.summary       = 'A Cocoa Pod for PyBind11 (C++ on OSX Only)'
  spec.source        = { 
    :git => 'git@github.com:StandardCyborg/protobag.git',
    :tag => 'v0.0.1'
  }
  spec.cocoapods_version = '>= 1.0'

  spec.osx.deployment_target  = '10.15'

  spec.source_files = 'c++/protobag_native/*.cpp'
  spec.public_header_files = ''
  spec.header_mappings_dir = ''

  # python_includes = `python3-config --includes`
  python_includes = `python3 -m pybind11 --includes`
  puts('python_includes')
  puts(python_includes)

  cpp_flags = '"$(inherited)" -undefined dynamic_lookup ' + python_includes
  spec.pod_target_xcconfig = {
    'CLANG_CXX_LANGUAGE_STANDARD' => 'c++17',
    'CLANG_CXX_LIBRARY' => 'libc++',
    'OTHER_CPLUSPLUSFLAGS' => cpp_flags,
  }

  spec.user_target_xcconfig = {
    'CLANG_CXX_LANGUAGE_STANDARD' => 'c++17',
    'CLANG_CXX_LIBRARY' => 'libc++',
    'OTHER_CPLUSPLUSFLAGS' => cpp_flags,
  }

  spec.dependencies = {'ProtobagCocoa' => '~> 0.0.1'}

  # , "PyBind11C++": "~> 2.5.0"
end
