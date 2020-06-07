// #include "protobag/Utils/TimeSync.hpp"

// #include <unordered_map>

// #include "protobag/archive/Archive.hpp"

// namespace protobag {

// bool MaybeBundle::IsNotFound() const {
//   return error == archive::Archive::ReadStatus::EntryNotFound().error;
// }


// struct MaxSlopTimeSync::Impl {
//   struct TopicQ {
//     std::unordered_map<::google::protobuf::Timestamp, Entry> q;

//     void PopOldest() {
//       ::google::protobuf::Timestamp t = 
//         ::google::protobuf::util::TimeUtil::kTimestampMaxSeconds;
//       for (const auto &qe : q) {
//         t = std::min(t, qe.first);
//       }
//       Pop(t);
//     }
    
//     std::optional<Entry> Pop(const ::google::protobuf::Timestamp &t) {
//       auto it = q.find(t);
//       if (it == q.end()) {
//         return std::nullopt;
//       } else {
//         Entry entry = std::move(it->second);
//         q.erase(it);
//         return std::move(entry);
//       }
//     }

//     void Push(
//       const ::google::protobuf::Timestamp &t,
//       Entry &&entry) {
//         q.insert({t, entry});
//     }
    
//     size_t Size() const { return q.size(); }

//   };


//   MaxSlopTimeSync::Spec spec;
//   std::unordered_map<std::string, TopicQ> topic_to_q;

//   explicit Impl(const MaxSlopTimeSync::Spec &s) {
//     spec = s;
//     for (const auto &topic : s.topics) {
//       topic_to_q[topic] = {};
//     }
//   }

//   void Enqueue(Entry &&entry) {
//     const auto &maybeTT = entry.GetTopicTime();
//     if (!maybeTT.has_value()) {
//       return;
//     }
//     const TopicTime &tt = *maybeTT;

//     if (topic_to_q.find(tt.topic()) != topic_to_q.end()) {
//       auto &topic_q = topic_to_q[tt.topic()];
//       if (topic_q.Size() >= spec.max_queue_size) {
//         topic_q.PopOldest();
//       }
//       topic_q.Push(tt.timestamp(), entry);
//     }
//   }


// };

// Result<TimeSync::Ptr> MaxSlopTimeSync::Create(
//     const ReadSession::Ptr &rs,
//     const std::vector<std::string> &topics,
//     const Spec &spec) {

//   if (!rs) {
//     return {.error = "Null read session; nothing to read"};
//   }

//   auto *sync = new MaxSlopTimeSync();
//   TimeSync::Ptr p(sync);

//   sync->_read_sess = rs;
//   sync->_spec = spec;
//   sync->_impl.reset(new Impl(topics));

//   return {.value = p};
// }

// MaybeBundle MaxSlopTimeSync::GetNext() override {

// }

// } /* namespace protobag */
