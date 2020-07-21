# Protobag: A bag o' String-serialized Protobuf Messages
_With built-in support for time-series data_

[![Build Status](https://circleci.com/gh/StandardCyborg/protobag.svg?style=svg&circle-token=ed56e2ec32789fa3e5f664bc8ea73c55e119de4b)](https://app.circleci.com/pipelines/github/StandardCyborg/protobag)

## Quickstart & Demo

See [this python noteboook](fixme link) for a demo of key features.

`pip3 install protobag`

`todo docker local start`

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

A `Protobag` file is simply an archive (e.g. a Zip or Tar file, or even just a
directory) with files that are string-serialized Protobuf messages.  You can
create a protobag, throw away the `Protobag` library itself, and still 
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
     any protobag to a bunch of JSONs.
 * `Protobag` includes features for time-series data and offers a
     "(topic/channel) - time" interface to data similar to those offered in 
     [ROS](http://wiki.ros.org/rosbag) and 
     [LCM](https://lcm-proj.github.io/log_file_format.html), respectively.


## Batteries Included

`Protobag` uses [libarchive](https://www.libarchive.org/) as an archive
back-end to interoperate with `zip`, `tar`, and other archive formats.  We
chose `libarchive` because it's highly portable and has minimal dependencies--
just `libz` for `zip` and nothing for `tar`.  `Protobag` also includes vanilla
[DirectoryArchive](c++/protobag/protobag/archive/DirectoryArchive.hpp) and
[MemoryArchive](c++/protobag/protobag/archive/MemoryArchive.hpp) back-ends for
testing and adhoc use.

If you want a simple "zip and unzip" utility, `Protobag` also includes those as
wrappers over `libarchive`.  See 
[ArchiveUtil](c++/protobag/protobag/ArchiveUtil.hpp).


## Development


## Discussion of Key Features

### Protobag indexes Protobuf message Descriptors

By default, `protobag` not only saves those messages but also 
**indexes Protobuf message descriptors** so that your `protobag` readers don't
need your proto schemas to decode your messages.  

#### Wat?
In order to deserialize a Protobuf message, typically you need
`protoc`-generated code for that message type (and you need `protoc`-generated
code for your specific programming language).  This `protoc`-generated code is
engineered for efficiency and provides a clean API for accessing message
attributes.  But what if you don't have that `protoc`-generated code?  Or you
don't even have the `.proto` message definitions to generate such code?

In Protobuf version 3.x, the authors added official support for
[the self-describing message paradigm](https://developers.google.com/protocol-buffers/docs/techniques).  
Now a user can serialize not just a message but Protobuf Descriptor data that
describes the message schema and enables deserialzing the message
*without protoc-generated code*-- all you need is the `protobuf` library itself.  
(This is a core feature of other serialization libraries 
[like Avro](http://avro.apache.org/docs/1.6.1/)).

Note: dynamic message decoding is slower than using `protoc`-generated code.  
Furthermore, the `protoc`-generated code makes defensive programming a bit 
easier.  You probably want to use the `protoc`-generated code for your 
messages if you can.

#### Protobag enables all messages to be self-describing messages
While Protobuf includes tools for using self-describing messages, the feature 
isn't simply a toggle in your `.proto` file, and the API is a bit complicated 
(because Google claims they don't use it much internally).

`protobag` automatically indexes the Protobuf Descriptor data for your messages 
at write time.  (And you can disable this indexing if so desired).  At read 
time, `protobag` automatically uses this indexed Descriptor data if the user 
reading your `protobag` file lacks the needed `protoc`-generated code to 
deserialize a message.

What if a message type evolves?  `protobag` indexes each distinct message type 
for each write session.  If you change your schema for a message type between 
write sessions, `protobag` will have indexed both schemas and will use the 
proper one for dynamic deserialization.

#### For More Detail

For Python, see:
 * `protobag.build_fds_for_msg()` -- This method collects the descriptor data
     needed for any Protobuf Message instance or class.
 * `protobag.DynamicMessageFactory::dynamic_decode()` -- This method uses 
     standard Protobuf APIs to deserialize messages given only Protobuf
     Descriptor data.

For C++, see:
 * `BagIndexBuilder::DescriptorIndexer::Observe()` -- This method collects the
     descriptor data needed for any Protobuf Message instance or class.
 * `DynamicMsgFactory` -- This utility uses uses standard Protobuf APIs to 
     deserialize messages given only Protobuf Descriptor data.


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
