# Protobag: A bag o' String-serialized Protobuf Messages
_With built-in support for time-series data_


## Summary 

[Protobuf](https://github.com/protocolbuffers/protobuf) is a popular data
serialization library for C++, Python, and several other languages.  In
Protobuf, you can easily define a message type, create a message instance,
and then serialize that message to a string or file.

But what if you want to store multiple messages in one "blob"?  You could 
simply use `repeated` and create one giant message, but perhaps you don't in
general have the RAM for this approach.  Well then, you could append multiple 
messages into one big file, and delimit the boundaries of each message using 
the number of bytes in the message itself.  Then you'd have something that 
looks exactly like the infamous
[TFRecords](https://www.tensorflow.org/tutorials/load_data/tfrecord)
format, which is somewhat performant for whole-file streaming reads, and has
a very long list of downsides.  For example, you can't even seek-to-message
in a `TFRecords` file, and you either need a large depenency (`tensorflow`) or
some very tricky custom code to even just do one pass over the file to count
the number of messages in it.  A substantially better solution is to simply
create a `tar` archive of string-serialized Protobuf messages-- 
*enter Protobag*.

A `Protobag` is simply an archive (e.g. a Zip or Tar file, or even just a
directory) with files that are string-serialized Protobuf messages.  You can
create a 'Protobag file,' throw away the `Protobag` library itself, and still 
have usable data.  But maybe you'll want to keep the `Protobag` library around
for the suite of tools it offers:
 * `Protobag` provides the "glue" needed to interface Proto*buf* with the 
     fileystem and/or an archive library, and `Protobag` strives to be fully
     cross-platform (in particular supporting deployment to iOS).
 * `Protobag` optionally indexes your messages and retains message Descriptors
     (employing the Protobuf 
       ["self-describing message" technique](https://developers.google.com/protocol-buffers/docs/techniques#self-description))
     so that readers of your `Protobag`s need not have your Proto*buf* message
     definitions.  One consequence is that, with this index, you can convert
     any `Protobag` file to a bunch of JSONs.
 * `Protobag` includes features for time-series data and offers a
     "(topic/channel) - time" interface to data similar to those offered in 
     [ROS](http://wiki.ros.org/rosbag) and 
     [LCM](https://lcm-proj.github.io/log_file_format.html), respectively.






TODO: "treat as a map" API


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