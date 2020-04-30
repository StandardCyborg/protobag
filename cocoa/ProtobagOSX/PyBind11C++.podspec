
Pod::Spec.new do |spec|
  spec.name          = 'PyBind11C++'
  spec.version       = '2.5.0'
  spec.license       = { :type => 'BSD' }
  spec.homepage      = 'https://github.com/pybind/pybind11'
  spec.authors       = {
    'PyBind11C++' => 'paul@standardcyborg.com',
    'pybind11' =>    'https://github.com/pybind/pybind11'
  }
  spec.summary       = 'A Cocoa Pod for PyBind11 (C++ on OSX Only)'
  spec.source        = { 
    :git => 'https://github.com/pybind/pybind11',
    :tag => 'v2.5.0'
  }
  spec.cocoapods_version = '>= 1.0'

  spec.osx.deployment_target  = '10.15'

  spec.source_files = 'include/**/*.{h,hpp}'
  spec.public_header_files = 'include/**/*.{h,hpp}'
  spec.header_mappings_dir = 'include'

  python_includes = `python3-config --includes`
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

end
