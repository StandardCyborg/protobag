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
  auto protobag_path = CreateTempDir("DemoTest") / "demo.zip";
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

      // TODO show how to decode, also with dynamic decode! ~~~~~~~~~~~~~~~~~~~~~~~~~~~
      // if (current.topic == "/topic1") {
      //   LOG("Read a string msg: " << UnpackedToPBTxt<StdMsg_String>(current.stamped_msg));
      // } else if (current.topic == "/topic2") {
      //   LOG("Read an int msg: " << UnpackedToPBTxt<StdMsg_Int>(current.stamped_msg));
      // } else {
      //   LOG("Got ??? " << PBToString(current.stamped_msg));
      // }

      LOG("");

    } while(still_reading);

    LOG("");
    LOG("");
    auto maybe_index = ReadSession::GetIndex(protobag_path);
    if (!maybe_index.IsOk()) {
      throw std::runtime_error(maybe_index.error);
    }
    LOG(
      "Protobag Index:" << std::endl <<
      PBToString(*maybe_index.value));
  }
}




#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/dynamic_message.h>
// #include <google/protobuf/util/type_resolver.h>
// #include <google/protobuf/util/type_resolver_util.h>
#include <google/protobuf/util/json_util.h>

// // https://github.com/protocolbuffers/protobuf/blob/7bff8393cab939bfbb9b5c69b3fe76b4d83c41ee/src/google/protobuf/util/json_util.cc#L217
// namespace detail {
//   using namespace google::protobuf;
// const char* kTypeUrlPrefix = "type.googleapis.com";
// util::TypeResolver* generated_type_resolver_ = NULL;
// ::google::protobuf::internal::once_flag generated_type_resolver_init_;

// std::string GetTypeUrl(const Message& message) {
//   return std::string(kTypeUrlPrefix) + "/" +
//          message.GetDescriptor()->full_name();
// }

// void DeleteGeneratedTypeResolver() { delete generated_type_resolver_; }

// void InitGeneratedTypeResolver() {
//   generated_type_resolver_ = util::NewTypeResolverForDescriptorPool(
//       kTypeUrlPrefix, DescriptorPool::generated_pool());
//   ::google::protobuf::internal::OnShutdown(&DeleteGeneratedTypeResolver);
// }

// util::TypeResolver* GetGeneratedTypeResolver() {
//   ::google::protobuf::internal::call_once(generated_type_resolver_init_,
//                                              InitGeneratedTypeResolver);
//   return generated_type_resolver_;
// }
// }  // namespace detail

TEST(DemoTest, TestMonkey) {

  TopicTime tt;

  tt.set_topic("my-topic");
  tt.mutable_timestamp()->set_seconds(123);

  LOG(
    "tt:" << std::endl <<
    PBToString(tt));

  ::google::protobuf::DescriptorProto p;
  tt.GetDescriptor()->CopyTo(&p);
  // LOG(
  //   "tt descriptor:" <<
  //   PBToString(p));

  ::google::protobuf::FileDescriptorSet fds;
  ::google::protobuf::FileDescriptorProto *fd = fds.add_file();
  tt.GetDescriptor()->file()->CopyTo(fd);
  LOG("containing_type " << tt.GetDescriptor()->containing_type());

  LOG("dependency_count " << tt.GetDescriptor()->file()->dependency_count());
  for (int d = 0; d < tt.GetDescriptor()->file()->dependency_count(); ++d) {
    ::google::protobuf::FileDescriptorProto *fd = fds.add_file();
    const ::google::protobuf::FileDescriptor *dep = tt.GetDescriptor()->file()->dependency(d);
    dep->CopyTo(fd);
    LOG("copied " << dep->name());
  }

  // {
  //   google::protobuf::Any any;
  //   ::google::protobuf::FileDescriptorProto *fd = fds.add_file();
  //   any.GetDescriptor()->file()->CopyTo(fd);
  // }
  // {
  //   google::protobuf::Timestamp any;
  //   ::google::protobuf::FileDescriptorProto *fd = fds.add_file();
  //   any.GetDescriptor()->file()->CopyTo(fd);
  // }
  // {
  //   google::protobuf::DescriptorProto any;
  //   ::google::protobuf::FileDescriptorProto *fd = fds.add_file();
  //   any.GetDescriptor()->file()->CopyTo(fd);
  // }

  // LOG(
  //   "tt fds:" <<
  //   PBToString(fds));


  {
    using namespace ::google::protobuf;
    const std::string msg_str = PBToString(tt);

    SimpleDescriptorDatabase db;
    DescriptorPool pool(&db);
    for (int i = 0; i < fds.file_size(); ++i) {
      db.Add(fds.file(i));
    }

    {
      std::vector<std::string> fnames;
      bool success = db.FindAllFileNames(&fnames);
      if (success) {
        for (const auto &fname : fnames) {
          LOG("db file: " << fname);
        }
      }
    }


    LOG("full name " << tt.GetDescriptor()->full_name());
    DynamicMessageFactory factory;
    const Descriptor *mt = nullptr;
    mt = pool.FindMessageTypeByName(tt.GetDescriptor()->full_name());
    LOG("mt " << mt);
  
    if (mt) {
      std::unique_ptr<Message> mp(factory.GetPrototype(mt)->New());
      LOG("value of message ptr " << mp.get());

      if (mp) {
        // NOTE! msg is owned by the factory!! might wanna do a Swap
        auto &msg = *mp;
        ::google::protobuf::TextFormat::ParseFromString(msg_str, &msg);
        LOG("debug " << msg.DebugString());


        {
          std::string out;
          auto status = ::google::protobuf::util::MessageToJsonString(msg, &out);
          if (!status.ok()) {
            LOG("status out " << status.ToString());
          }
          LOG("my jsons: " << out);
        }
      }
    }


    // using namespace google::protobuf;
    // const DescriptorPool* pool = tt.GetDescriptor()->file()->pool();
    // util::TypeResolver* resolver =
    //     pool == DescriptorPool::generated_pool()
    //         ? detail::GetGeneratedTypeResolver()
    //         : util::NewTypeResolverForDescriptorPool(detail::kTypeUrlPrefix, pool);
    
  }

}














#undef LOG











// TEST(WriteSessionDirectory, TestBasic) {
//   auto testdir = CreateTempDir("WriteSessionDirectory.TestBasic");

  

//   // Now check what we wrote
//   {
//     auto dar = OpenAndCheck({
//       .mode="read",
//       .path=testdir,
//       .format="directory",
//     });

//     {
//       auto namelist = dar->GetNamelist();
//       std::vector<std::string> actual;
//       for (auto name : namelist) {
//         name = std::string("/") + name;  // FIXME want leading '/' ? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//         if (!IsProtoBagIndexTopic(name)) {
//           actual.push_back(name);
//         }
//       }
      
//       std::vector<std::string> expected = {
//         "/topic1/1.protobin",
//         "/topic1/2.protobin",
//         "/topic2/1.protobin",  // FIXME want leading '/' ? ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//       };
//       EXPECT_SORTED_SEQUENCES_EQUAL(expected, actual);
//     }

//     {
//       auto res = dar->ReadAsStr("topic1/1.protobin");
//       EXPECT_TRUE(res.IsOk()) << res.error;
//       auto maybe_msg = PBFactory::LoadFromContainer<StampedMessage>(*res.value);
//       EXPECT_TRUE(maybe_msg.IsOk()) << maybe_msg.error;
//       const StampedMessage &m = *maybe_msg.value;
//       EXPECT_EQ(m.timestamp().seconds(), 0);
//       EXPECT_EQ(m.timestamp().nanos(), 0);
//       EXPECT_EQ(m.msg().type_url(), "type.googleapis.com/protobag.StdMsg.String");

//       {
//         auto maybe_msg = PBFactory::UnpackFromAny<StdMsg_String>(m.msg());
//         EXPECT_TRUE(maybe_msg.IsOk()) << maybe_msg.error;
//         EXPECT_EQ(maybe_msg.value->value(), "foo");
//       }
//     }
    
//     {
//       auto res = dar->ReadAsStr("topic1/2.protobin");
//       EXPECT_TRUE(res.IsOk()) << res.error;
//       auto maybe_msg = PBFactory::LoadFromContainer<StampedMessage>(*res.value);
//       EXPECT_TRUE(maybe_msg.IsOk()) << maybe_msg.error;
//       const StampedMessage &m = *maybe_msg.value;
//       EXPECT_EQ(m.timestamp().seconds(), 1);
//       EXPECT_EQ(m.timestamp().nanos(), 0);
//       EXPECT_EQ(m.msg().type_url(), "type.googleapis.com/protobag.StdMsg.String");

//       {
//         auto maybe_msg = PBFactory::UnpackFromAny<StdMsg_String>(m.msg());
//         EXPECT_TRUE(maybe_msg.IsOk()) << maybe_msg.error;
//         EXPECT_EQ(maybe_msg.value->value(), "bar");
//       }
//     }

//     {
//       auto res = dar->ReadAsStr("topic2/1.protobin");
//       EXPECT_TRUE(res.IsOk()) << res.error;
//       auto maybe_msg = PBFactory::LoadFromContainer<StampedMessage>(*res.value);
//       EXPECT_TRUE(maybe_msg.IsOk()) << maybe_msg.error;
//       const StampedMessage &m = *maybe_msg.value;
//       EXPECT_EQ(m.timestamp().seconds(), 0);
//       EXPECT_EQ(m.timestamp().nanos(), 0);
//       EXPECT_EQ(m.msg().type_url(), "type.googleapis.com/protobag.StdMsg.Int");

//       {
//         auto maybe_msg = PBFactory::UnpackFromAny<StdMsg_Int>(m.msg());
//         EXPECT_TRUE(maybe_msg.IsOk()) << maybe_msg.error;
//         EXPECT_EQ(maybe_msg.value->value(), 1337);
//       }
//     }

//     {
//       // TODO check bag meta ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//     }
//   }

// }
