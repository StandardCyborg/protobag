coming soon

```
indocker % cd /opt/protobag/cxx
indocker % mkdir -p build && cd build
indocker % cmake -DCMAKE_BUILD_TYPE=DEBUG ..
indocker % make -j `nproc` && ./protobag_test --gtest_filter=DemoTest*
```

```
 pod repo push  SCCocoaPods ProtobagCocoa.podspec.json  --use-libraries --verbose --allow-warnings
```


in python subdir:
```
python3 setup.py bdist_wheel
```
for both linux and xcode