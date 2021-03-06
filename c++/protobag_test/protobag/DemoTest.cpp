/*
Copyright 2020 Standard Cyborg

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "gtest/gtest.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <vector>

#include "protobag/Utils/PBUtils.hpp"
#include "protobag/Utils/StdMsgUtils.hpp"
#include "protobag/ReadSession.hpp"
#include "protobag/WriteSession.hpp"

#include "protobag_test/Utils.hpp"

using namespace protobag;
using namespace protobag_test;

#define LOG(x) do { \
  std::cout << x << std::endl; \
} while(0)

inline 
WriteSession::Ptr OpenWriterAndCheck(const WriteSession::Spec &spec) {
  auto result = WriteSession::Create(spec);
  if (!result.IsOk()) {
    throw std::runtime_error(result.error);
  }

  auto w = *result.value;
  if (!w) {
    throw std::runtime_error("Null pointer exception: bad result object");
  }

  return w;
}

inline 
ReadSession::Ptr OpenReaderAndCheck(const ReadSession::Spec &spec) {
  auto result = ReadSession::Create(spec);
  if (!result.IsOk()) {
    throw std::runtime_error(result.error);
  }

  auto r = *result.value;
  if (!r) {
    throw std::runtime_error("Null pointer exception: bad result object");
  }

  return r;
}

inline
void ExpectWriteOk(WriteSession &w, const Entry &entry) {
  OkOrErr result = w.WriteEntry(entry);
  if (!result.IsOk()) {
    throw std::runtime_error(result.error);
  }
}

template <typename MT>
std::string UnpackedToPBTxt(const StampedMessage &s) {
  auto maybe_msg = PBFactory::UnpackFromAny<MT>(s.msg());
  if (!maybe_msg.IsOk()) {
    throw std::runtime_error(maybe_msg.error);
  }
  return PBToString(*maybe_msg.value);
}

TEST(DemoTest, TestDemo) {
  // We'll put our demo protobag here:
  auto protobag_path = CreateTestTempdir("DemoTest") / "demo.zip";
  LOG("Writing ==============================================================");
  LOG("Writing protobag to: " << protobag_path);

  // First we're going to create a protobag.  Here's the data we'll write:
  std::vector<Entry> entries_to_write = {
    // (topic, time, msg) tuples.  Boxed for easier interop with the writer.
    Entry::CreateStamped("/topic1", 1, 0, ToStringMsg("foo")),
    Entry::CreateStamped("/topic1", 2, 0, ToStringMsg("bar")),
    Entry::CreateStamped("/topic2", 1, 0, ToIntMsg(1337)),
  };

  // Now create the protobag:
  {
    auto wp = OpenWriterAndCheck({
      .archive_spec = {
        .mode="write",
        .path=protobag_path,
        .format="zip",
      }
    });
    auto &writer = *wp;

    for (const Entry &entry : entries_to_write) {
      ExpectWriteOk(writer, entry);

      LOG("Wrote: " << entry.ToString());
    }

    // writer auto-closes and writes meta
  }
  LOG("");
  LOG("");

  LOG("Audit ================================================================");
  std::string cmd = std::string("unzip -l ") + protobag_path.string();
  LOG("Running: " << cmd);
  std::system(cmd.c_str());
  LOG("");
  LOG("");
  

  // Now let's read
  LOG("Reading ==============================================================");
  {
    auto rp = OpenReaderAndCheck(
      ReadSession::Spec::ReadAllFromPath(protobag_path));

    auto &reader = *rp;
    bool still_reading = true;
    do {
      MaybeEntry maybe_next = reader.GetNext();
      if (maybe_next.IsEndOfSequence()) {
        still_reading = false;
        break;
      }

      ASSERT_TRUE(maybe_next.IsOk()) << maybe_next.error;
      
      Entry current = *maybe_next.value;
      LOG(
        "Read entry:" << std::endl <<
        current.ToString());

      LOG("");

    } while(still_reading);

    LOG("");
    LOG("");
    auto maybe_index = ReadSession::GetIndex(protobag_path);
    if (!maybe_index.IsOk()) {
      throw std::runtime_error(maybe_index.error);
    }
    // This is super noisy
    // LOG(
    //   "Protobag Index:" << std::endl <<
    //   PBToString(*maybe_index.value));
  }
}


// TODO: create a demo for protobag::DynamicMsgFactory

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/dynamic_message.h>
// #include <google/protobuf/util/type_resolver.h>
// #include <google/protobuf/util/type_resolver_util.h>
#include <google/protobuf/util/json_util.h>


// TEST(DemoTest, TestMonkey) {

//   TopicTime tt;

//   tt.set_topic("my-topic");
//   tt.mutable_timestamp()->set_seconds(123);

//   LOG(
//     "tt:" << std::endl <<
//     PBToString(tt));

//   ::google::protobuf::DescriptorProto p;
//   tt.GetDescriptor()->CopyTo(&p);
//   // LOG(
//   //   "tt descriptor:" <<
//   //   PBToString(p));

//   ::google::protobuf::FileDescriptorSet fds;
//   ::google::protobuf::FileDescriptorProto *fd = fds.add_file();
//   tt.GetDescriptor()->file()->CopyTo(fd);
//   LOG("containing_type " << tt.GetDescriptor()->containing_type());

//   LOG("dependency_count " << tt.GetDescriptor()->file()->dependency_count());
//   for (int d = 0; d < tt.GetDescriptor()->file()->dependency_count(); ++d) {
//     ::google::protobuf::FileDescriptorProto *fd = fds.add_file();
//     const ::google::protobuf::FileDescriptor *dep = tt.GetDescriptor()->file()->dependency(d);
//     dep->CopyTo(fd);
//     LOG("copied " << dep->name());
//   }

//   // {
//   //   google::protobuf::Any any;
//   //   ::google::protobuf::FileDescriptorProto *fd = fds.add_file();
//   //   any.GetDescriptor()->file()->CopyTo(fd);
//   // }
//   // {
//   //   google::protobuf::Timestamp any;
//   //   ::google::protobuf::FileDescriptorProto *fd = fds.add_file();
//   //   any.GetDescriptor()->file()->CopyTo(fd);
//   // }
//   // {
//   //   google::protobuf::DescriptorProto any;
//   //   ::google::protobuf::FileDescriptorProto *fd = fds.add_file();
//   //   any.GetDescriptor()->file()->CopyTo(fd);
//   // }

//   // LOG(
//   //   "tt fds:" <<
//   //   PBToString(fds));


//   {
//     using namespace ::google::protobuf;
//     const std::string msg_str = PBToString(tt);

//     SimpleDescriptorDatabase db;
//     DescriptorPool pool(&db);
//     for (int i = 0; i < fds.file_size(); ++i) {
//       db.Add(fds.file(i));
//     }

//     {
//       std::vector<std::string> fnames;
//       bool success = db.FindAllFileNames(&fnames);
//       if (success) {
//         for (const auto &fname : fnames) {
//           LOG("db file: " << fname);
//         }
//       }
//     }


//     LOG("full name " << tt.GetDescriptor()->full_name());
//     DynamicMessageFactory factory;
//     const Descriptor *mt = nullptr;
//     mt = pool.FindMessageTypeByName(tt.GetDescriptor()->full_name());
//     LOG("mt " << mt);
  
//     if (mt) {
//       std::unique_ptr<Message> mp(factory.GetPrototype(mt)->New());
//       LOG("value of message ptr " << mp.get());

//       if (mp) {
//         // NOTE! msg is owned by the factory!! might wanna do a Swap
//         auto &msg = *mp;
//         ::google::protobuf::TextFormat::ParseFromString(msg_str, &msg);
//         LOG("debug " << msg.DebugString());


//         {
//           std::string out;
//           auto status = ::google::protobuf::util::MessageToJsonString(msg, &out);
//           if (!status.ok()) {
//             LOG("status out " << status.ToString());
//           }
//           LOG("my jsons: " << out);
//         }
//       }
//     }


//     // using namespace google::protobuf;
//     // const DescriptorPool* pool = tt.GetDescriptor()->file()->pool();
//     // util::TypeResolver* resolver =
//     //     pool == DescriptorPool::generated_pool()
//     //         ? detail::GetGeneratedTypeResolver()
//     //         : util::NewTypeResolverForDescriptorPool(detail::kTypeUrlPrefix, pool);
    
//   }

// }














#undef LOG



