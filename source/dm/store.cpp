#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <regex>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "radiant.h"
#include "store.h"
#include "store_trace.h"
#include "tracer_sync.h"
#include "tracer_buffered.h"
#include "record.h"
#include "object.h"
#include "protocol.h"
#include "lua_types.h"
#include "streamer.h"

#if defined GetMessage
#  undef GetMessage
#endif
#include "protocols/store.pb.h"
#include "protocols/tesseract.pb.h"

using namespace ::radiant;
using namespace ::radiant::dm;

#define STORE_LOG(level)      LOG(dm.store, level)

Store*  Store::stores_[4 + 1] = { 0 };

static const std::regex object_address_regex__("^object://(.*)/(\\d+)$");

Store& Store::GetStore(int id)
{
   if (!id) {
      STORE_LOG(0) << "object asked for store id 0.  we're about to die...";
   }
   ASSERT(id);
   return *stores_[id];
}

Store::Store(int which, std::string const& name) :
   storeId_(which),
   nextObjectId_(1),
   nextGenerationId_(1),
   name_(name),
   saving_(false),
   tracing_disabled_(false)
{
   ASSERT(storeId_);

   // ASSERT(stores_[which] == nullptr);
   stores_[which] = this;

   RegisterAllocator(NumberMapObjectType, []() -> NumberMapPtr {
      return std::make_shared<NumberMap>();
   });
}

Store::~Store(void)
{
}


// Destroy all of our traces, and ensure we don't create new ones (this is to be used during shutdown).
void Store::DisableAndClearTraces()
{
   tracing_disabled_ = true;
   store_traces_.clear();
   traces_.clear();
   tracers_.clear();
}


//
// -- Store::AddStreamer
//
// Add a streamer to the store.
//
void Store::AddStreamer(Streamer* streamer)
{
   streamers_.push_back(streamer);

   // Notify the new guy of the current state of the store.
   for (const auto& entry : dynamicObjects_) {
      ObjectPtr obj = entry.second.lock();
      if (obj) {
         streamer->OnAlloced(obj);
      }
   }

   // Give all the objects a chance to register with the streamer.  We do this
   // via a virtual function dispatch in case the object needs to invoke
   // Streamer::TraceObject(), which requires the base type as a template
   // parameter.
   for (const auto& entry : objects_) {
      entry.second->RegisterWithStreamer(streamer);
   }

}

//
// -- Store::RemoveStreamer
//
// Remove a streamer from the store.
//
void Store::RemoveStreamer(Streamer* streamer)
{
   stdutil::UniqueRemove(streamers_, streamer);
}


std::unordered_map<const char*, int> Store::GetTraceReasons()
{
   std::unordered_map<const char*, int> reasonMap;
   for (auto const& tracelist : traces_) {
      for (auto const& trace : tracelist.second) {
         auto t = trace.lock();
         if (t) {
            reasonMap[t->GetReason()]++;
         }
      }
   }

   return reasonMap;
}

ObjectPtr Store::AllocObject(ObjectType t, ObjectId id)
{
   ASSERT(allocators_[t]);
   ObjectPtr obj = allocators_[t]();
   ObjectId object_id = id ? id : GetNextObjectId();

   obj->SetObjectMetadata(object_id, *this); 
   if (id == 0) {
      obj->ConstructObject();
   }
   RegisterObject(*obj);
   OnAllocObject(obj);

   return obj;
}

void Store::RegisterAllocator(ObjectType t, ObjectAllocFn const& allocator)
{
   ASSERT(!allocators_[t]);
   allocators_[t] = allocator;
}

void Store::SaveStoreHeader(google::protobuf::io::CodedOutputStream& cos)
{
   Protocol::Store msg;

   msg.set_store_id(storeId_);
   msg.set_next_object_id(nextObjectId_);
   msg.set_next_generation(nextGenerationId_);

   cos.WriteLittleEndian32(msg.ByteSize());
   msg.SerializeToCodedStream(&cos);
}

void Store::SaveAllocedObjectsList(google::protobuf::io::CodedOutputStream& cos)
{
   tesseract::protocol::Update update;

   update.set_type(tesseract::protocol::Update::AllocObjects);
   auto allocated_msg = update.MutableExtension(tesseract::protocol::AllocObjects::extension);
   for (const auto& entry : dynamicObjects_) {
      dm::ObjectPtr obj = entry.second.lock();
      if (obj) {
         STORE_LOG(9) << "saving object " << entry.first << ".";
         auto msg = allocated_msg->add_objects();
         msg->set_object_id(entry.first);
         msg->set_object_type(obj->GetObjectType());
      }
   }
   uint32 size = update.ByteSize();
   STORE_LOG(8) << "writing alloced objects size:" << size;

   cos.WriteLittleEndian32(size);
   update.SerializeToCodedStream(&cos);
}

void Store::SaveObjects(google::protobuf::io::CodedOutputStream& cos)
{
   std::vector<ObjectId> keys;
   for (auto const& entry : objects_) {
      keys.emplace_back(entry.first);
   };
   SaveObjects(cos, keys);
}

void Store::SaveObjects(google::protobuf::io::CodedOutputStream& cos, std::vector<ObjectId>& objects)
{
   Protocol::Object msg;

   // sort so we save objects in the order they were created
   std::sort(objects.begin(), objects.end());

   cos.WriteLittleEndian32((int)objects.size());
   for (ObjectId id : objects) {
      Object* obj = objects_[id];

      msg.Clear();
      obj->SaveObject(PERSISTANCE, &msg);
 
      uint32 size = msg.ByteSize();
      STORE_LOG(8) << "writing object " << id << " size " << size;
      cos.WriteLittleEndian32(size);
      msg.SerializeToCodedStream(&cos);
   }
}


bool Store::Save(std::string const& filename, std::string &error)
{
   // The C++ Google Protobuf docs claim FileOutputStream avoids an extra copy of the data
   // introduced by OstreamOutputStream, so lets' use that.
   int fd = _open(filename.c_str(), O_WRONLY | O_BINARY | O_TRUNC | O_CREAT, S_IREAD | S_IWRITE);
   if (fd < 0) {
      error = "could not open save file";
      return false;
   }

   saving_ = true;

   {
      google::protobuf::io::FileOutputStream fos(fd);
      {
         google::protobuf::io::CodedOutputStream cos(&fos);

         SaveStoreHeader(cos);
         SaveAllocedObjectsList(cos);
         SaveObjects(cos);
      }
   }


   saving_ = false;
   _close(fd);
   return true;
}

std::string Store::SaveObject(ObjectId id, std::string& error)
{
   std::ostringstream stream;
   {
      google::protobuf::io::OstreamOutputStream oos(&stream);
      {
         google::protobuf::io::CodedOutputStream cos(&oos);
         {
            Protocol::Object msg;
            Object* obj = objects_[id];
            if (!obj) {
               error = BUILD_STRING("no such object id " << id);
               return std::string();
            }
            obj->SaveObject(PERSISTANCE, &msg);
            msg.SerializeToCodedStream(&cos);
         }
      }
   }

   return stream.str();
}


bool Store::LoadStoreHeader(google::protobuf::io::CodedInputStream& cis, std::string& error)
{
   Protocol::Store msg;
   google::protobuf::uint32 size, limit;

   if (!cis.ReadLittleEndian32(&size)) {
      error = "could not read size of header";
      return false;
   }
   STORE_LOG(8) << "read header size:" << size;
   limit = cis.PushLimit(size);
   if (!msg.ParseFromCodedStream(&cis)) {
      error = "could not parse header";
      return false;
   }
   cis.PopLimit(limit);

   nextObjectId_ = msg.next_object_id();
   nextGenerationId_ = msg.next_generation();

   return true;
}

bool Store::LoadAllocedObjectsList(google::protobuf::io::CodedInputStream& cis, std::string& error, ObjectMap& objects)
{
   google::protobuf::uint32 size, limit;
   tesseract::protocol::Update update;

   if (!cis.ReadLittleEndian32(&size)) {
      error = "could not read size of object list";
      return false;
   }
   limit = cis.PushLimit(size);
   if (!update.ParseFromCodedStream(&cis)) {
      error = "could not read object list";
      return false;
   }
   cis.PopLimit(limit);
   radiant::tesseract::protocol::AllocObjects const& alloc_msg = update.GetExtension(::radiant::tesseract::protocol::AllocObjects::extension);
   for (const tesseract::protocol::AllocObjects::Entry& entry : alloc_msg.objects()) {
      ObjectId id = entry.object_id();
      ASSERT(!FetchStaticObject(id));

      STORE_LOG(9) << "allocating object " << id << " on load.";
      ObjectPtr obj = AllocObject(entry.object_type(), id);
      objects[id] = obj;
   }
   return true;
}

bool Store::LoadObjects(google::protobuf::io::CodedInputStream& cis, std::string& error, LoadProgressCb const& cb)
{
   Protocol::Object msg;
   google::protobuf::uint32 size, limit, total_objects, i;

   if (!cis.ReadLittleEndian32(&total_objects)) {
      error = "could not total number of objects";
      return false;
   }
            
   int last_reported_percent = 0;
   for (i = 0; i < total_objects; i++) {
      if (!cis.ReadLittleEndian32(&size)) {
         error = BUILD_STRING("could not read size for object " << i << "  of " << total_objects << ".");
         return false;
      }
      int percent = i * 100 / total_objects;
      if (percent - last_reported_percent > 10) {
         last_reported_percent = percent;
         if (cb) {
            cb(percent);
         }
         STORE_LOG(1) << " load progress " << percent << "%...";
      }

      msg.Clear();
      limit = cis.PushLimit(size);
      if (!msg.ParseFromCodedStream(&cis)) {
         error = BUILD_STRING("could parse object " << i << "  of " << total_objects << " (size:" << size << ").");
         STORE_LOG(8) << "failed to parse msg size " << size << ".  aborting.";
         return false;
      }
      cis.PopLimit(limit);

      ObjectId id = msg.object_id();
      Object* obj = FetchStaticObject(id);

      STORE_LOG(8) << "read object " << id << " size " << size;

      ASSERT(obj);
      ASSERT(msg.object_type() == obj->GetObjectType());

      obj->SetObjectMetadata(id, *this);
      obj->LoadObject(PERSISTANCE, msg);
   }
   STORE_LOG(1) << " load objects finished!" << std::endl;
   return true;
}

bool Store::Load(std::string const& filename, std::string &error, ObjectMap& objects, LoadProgressCb const& cb)
{
   // xxx: should probably just pass the save state into the ctor...
   ASSERT(traces_.empty());
   ASSERT(tracers_.empty());
#if !defined(ENABLE_OBJECT_COUNTER)
   ASSERT(store_traces_.empty());
#endif
   ASSERT(objects_.empty());
   ASSERT(dynamicObjects_.empty());
   ASSERT(nextObjectId_ == 1);
   ASSERT(nextGenerationId_ == 1);

   // The C++ Google Protobuf docs claim FileOutputStream avoids an extra copy of the data
   // introduced by OstreamOutputStream, so lets' use that.
   int fd = _open(filename.c_str(), _O_RDONLY | O_BINARY);
   if (fd < 0) {
      error = "could not open save file";
      return false;
   }

   ASSERT(dynamicObjects_.size() == 0);

   google::protobuf::io::FileInputStream fis(fd);
   google::protobuf::io::CodedInputStream cis(&fis);

   // From: https://developers.google.com/protocol-buffers/docs/reference/cpp/google.protobuf.io.coded_stream
   //
   // To prevent integer overflows in the protocol buffers implementation, as well as to prevent
   // servers from allocating enormous amounts of memory to hold parsed messages, the maximum message
   // length should be limited to the shortest length that will not harm usability. The theoretical
   // shortest message that could cause integer overflows is 512MB. The default limit is 64MB
   //
   static const int limit = (512 * 1024 * 1024) - 1;
   cis.SetTotalBytesLimit(limit, limit);

   bool result = LoadStoreHeader(cis, error) &&
                 LoadAllocedObjectsList(cis, error, objects) &&
                 LoadObjects(cis, error, cb);

   _close(fd);
   return result;
}

bool Store::LoadObject(std::string const& input, std::string& error)
{
   google::protobuf::io::ArrayInputStream ais(input.data(), (int)input.size());
   google::protobuf::io::CodedInputStream cis(&ais);

   Protocol::Object msg;
   if (!msg.ParseFromCodedStream(&cis)) {
      error = "failed to parse save state.";
      return false;
   }
   Object* obj = FetchStaticObject(msg.object_id());
   if (!obj) {
      error = BUILD_STRING("no such object " << msg.object_id());
      return false;
   }

   ASSERT(msg.object_type() == obj->GetObjectType());

   STORE_LOG(5) << "loading individual object " << obj->GetObjectId() << " of type " << obj->GetObjectClassNameLower();
   obj->LoadObject(PERSISTANCE, msg);
   return true;
}

void Store::OnLoaded()
{
   for (auto const& entry : dynamicObjects_) {
      ObjectPtr obj = entry.second.lock();
      if (obj) {
         obj->OnLoadObject(PERSISTANCE);
      }
   }
}

void Store::RegisterObject(Object& obj)
{
   // Saving should have no side-effects, which means we shouldn't ever see an object
   // registered or unregistered.
   ASSERT(!saving_);
   ASSERT(obj.IsValid());

   if (objects_.find(obj.GetObjectId()) != objects_.end()) {
      throw std::logic_error(BUILD_STRING("data mode object " << obj.GetObjectId() << "being registered twice."));
   }

   ObjectId id = obj.GetObjectId();
   Object* pobj = &obj;
   objects_[id] = pobj;

   // Give the obj an opportunity to register with all our streamers.
   for (Streamer* s : streamers_) {
      obj.RegisterWithStreamer(s);
   }

   // Fire the OnRegistered cb on all our store traces.
   stdutil::ForEachPrune<StoreTrace>(store_traces_, [=](StoreTracePtr trace) {
      trace->SignalRegistered(pobj);
   });
}

void Store::UnregisterObject(const Object& obj)
{
   // Saving should have no side-effects, which means we shouldn't ever see an object
   // registered or unregistered.
   ASSERT(!saving_);

   ValidateObjectId(obj.GetObjectId());

   ObjectId id = obj.GetObjectId();
   STORE_LOG(9) << "unregistering object " << id;

   objects_.erase(id);
   auto j = dynamicObjects_.find(id);
   bool dynamic = j != dynamicObjects_.end();
   if (dynamic) {
      dynamicObjects_.erase(j);
   }
   auto i = traces_.find(id);
   if (i != traces_.end()) {
      stdutil::ForEachPrune<Trace>(i->second, [&](std::shared_ptr<Trace> t) {
         t->NotifyDestroyed();
      });
      traces_.erase(i);
   }

   recordFieldMap_.erase(id);

   // Let all the streamers know the object has been nuked.
   for (Streamer* s : streamers_) {
      s->OnDestroyed(id, dynamic);
   }

   // Fire all the OnDestroyed() cbs on our store traces.
   stdutil::ForEachPrune<StoreTrace>(store_traces_, [=](StoreTracePtr trace) {
      trace->SignalDestroyed(id, dynamic);
   });
}

void Store::ValidateObjectId(ObjectId oid) const
{
   ASSERT(oid == -1 || objects_.find(oid) != objects_.end());
}

int Store::GetObjectCount() const
{
   return (int)objects_.size();
}

ObjectId Store::GetNextObjectId()
{
   return nextObjectId_++;
}

GenerationId Store::GetNextGenerationId()
{
   return nextGenerationId_++;
}

GenerationId Store::GetCurrentGenerationId()
{
   return nextGenerationId_;
}

void Store::OnAllocObject(std::shared_ptr<Object> const& obj)
{
   ASSERT(obj->IsValid());

   ObjectId id = obj->GetObjectId();
   dynamicObjects_[id] = obj;

   // Notify all the streamers that there's a new dynamic object.
   for (Streamer* s : streamers_) {
      s->OnAlloced(obj);
   }

   // Fire the OnAlloced callback on all our store traces.
   stdutil::ForEachPrune<StoreTrace>(store_traces_, [&obj](StoreTracePtr trace) {
      trace->SignalAllocated(obj);
   });
}

Object* Store::FetchStaticObject(ObjectId id) const
{
   auto i = objects_.find(id);
   return i == objects_.end() ? nullptr : i->second;
}


std::shared_ptr<Object> Store::FetchObject(int id, ObjectType type) const
{
   std::shared_ptr<Object> obj;

   auto i = dynamicObjects_.find(id);
   if (i != dynamicObjects_.end()) {
      obj = i->second.lock();
      if (obj) {
         ASSERT(obj->IsValid());
         // ASSERT(type == -1 || entry.type == type); Fails in cases where dynamic cast may succeed
      } else {
         dynamicObjects_.erase(i);
      }
   }
   return obj;
}  

std::shared_ptr<Object> Store::FetchObject(std::string const& addr, ObjectType type) const
{
   std::smatch match;
   if (std::regex_match(addr, match, object_address_regex__)) {
      if (name_ == match[1]) {
         dm::ObjectId id = std::stoi(match[2]);
         return FetchObject(id, type);
      }
   }
   return ObjectPtr();
}  

bool Store::IsValidStoreAddress(std::string const& addr) const
{
   std::smatch match;
   if (std::regex_match(addr, match, object_address_regex__)) {
      return name_ == match[1];
   }
   return false;
}

std::vector<ObjectId> Store::GetModifiedSince(GenerationId when)
{
   std::vector<ObjectId> result;

   for (auto &entry : objects_) {
      Object* obj = entry.second;
      if (obj->GetLastModified() >= when) {
         ASSERT(obj->IsValid());
         result.push_back(obj->GetObjectId());
      }
   }
   return result;
}

bool Store::IsDynamicObject(ObjectId id)
{
   return dynamicObjects_.find(id) != dynamicObjects_.end();
}


StoreTracePtr Store::TraceStore(const char* reason)
{
   ASSERT(!tracing_disabled_);
   StoreTracePtr trace = std::make_shared<StoreTrace>(*this);
   store_traces_.push_back(trace);
   return trace;
}

void Store::PushStoreState(StoreTrace& trace) const
{
   for (const auto& entry : dynamicObjects_) {
      ObjectPtr obj = entry.second.lock();
      if (obj) {
         trace.SignalAllocated(obj);
      }
   }
   for (const auto& entry : objects_) {
      trace.SignalRegistered(entry.second);
   }
}
TracerPtr Store::GetTracer(int category)
{
   auto i = tracers_.find(category);
   if (i == tracers_.end()) {
      throw std::logic_error(BUILD_STRING("store has no trace set for category " << category));
   }
   return i->second;
}

void Store::AddTracer(TracerPtr set, int category)
{
   ASSERT(!tracing_disabled_);
   auto entry = tracers_.insert(std::make_pair(category, set));
   if (!entry.second) {
      throw std::logic_error(BUILD_STRING("duplicate tracer category " << category));
   }
}

template <typename TraceType>
void Store::ForEachTrace(ObjectId id, std::function<void(typename TraceType&)> cb)
{
   stdutil::ForEachPrune<StoreTrace>(store_traces_, [=](StoreTracePtr trace) {
      trace->SignalModified(id);
   });

   auto i = traces_.find(id);
   if (i != traces_.end()) {
      stdutil::ForEachPrune<Trace>(i->second, [&](std::shared_ptr<Trace> t) {
         TraceType *trace = static_cast<TraceType*>(t.get());
         cb(*trace);
      });
   }
}


template <typename T, typename TraceType>
void Store::MarkChangedAndFire(T& obj, std::function<void(typename TraceType&)> cb)
{
   ObjectId id = obj.GetObjectId();
   obj.MarkChanged();

   ForEachTrace<TraceType>(id, cb);

   auto i = recordFieldMap_.find(id);
   if (i != recordFieldMap_.end()) {
      ForEachTrace<RecordTrace<Record>>(i->second, [](RecordTrace<Record>& trace) {
         trace.NotifyRecordChanged();
      });
   }
}

template <typename T>
void Store::OnMapRemoved(T& map, typename T::Key const& key)
{
   MarkChangedAndFire<T, MapTrace<T>>(map, [&](MapTrace<T>& trace) {
      trace.NotifyRemoved(key);
   });
}
template <typename T>
void Store::OnMapChanged(T& map, typename T::Key const& key, typename T::Value const& value)
{
   MarkChangedAndFire<T, MapTrace<T>>(map, [&](MapTrace<T>& trace) {
      trace.NotifyChanged(key, value);
   });
}
template <typename T>
void Store::OnSetRemoved(T& set, typename T::Value const& value)
{
   MarkChangedAndFire<T, SetTrace<T>>(set, [&](SetTrace<T>& trace) {
      trace.NotifyRemoved(value);
   });
}
template <typename T>
void Store::OnSetAdded(T& set, typename T::Value const& value)
{
   MarkChangedAndFire<T, SetTrace<T>>(set, [&](SetTrace<T>& trace) {
      trace.NotifyAdded(value);
   });
}
template <typename T>
void Store::OnArrayChanged(T& arr, uint i, typename T::Value const& value)
{
   MarkChangedAndFire<T, ArrayTrace<T>>(arr, [&](ArrayTrace<T>& trace) {
      trace.NotifySet(i, value);
   });
}
template <typename T>
void Store::OnBoxedChanged(T& boxed, typename T::Value const& value)
{
   MarkChangedAndFire<T, BoxedTrace<T>>(boxed, [&](BoxedTrace<T>& trace) {
      trace.NotifyChanged(value);
   });

   // Signal all the streamers that the box has been modified.
   for (Streamer* s : streamers_) {
      s->OnModified(&boxed);
   }
}

void Store::OnRecordFieldChanged(Record const& record)
{
   // Simply signal that all the records have changed.
   dm::ObjectId id = record.GetObjectId();
   stdutil::ForEachPrune<StoreTrace>(store_traces_, [id](StoreTracePtr trace) {
      trace->SignalModified(id);
   });
}

#define ADD_TRACE_TO_TRACER(trace, tracer, Cls) \
      switch (tracer->GetType()) { \
      case Tracer::SYNC: \
         trace = static_cast<TracerSync*>(tracer)->Trace ## Cls ## Changes(reason, *this, o); \
         break; \
      case Tracer::BUFFERED: \
         trace = static_cast<TracerBuffered*>(tracer)->Trace ## Cls ## Changes(reason, *this, o); \
         break; \
      default: \
         throw std::logic_error(BUILD_STRING("unknown tracer type " << tracer->GetType())); \
      } \

#define ADD_TRACE_TO_TRACKER_CATEGORY(trace, category, Cls) \
   do { \
      auto tracer = GetTracer(category); \
      ADD_TRACE_TO_TRACER(trace, tracer.get(), Cls) \
   } while (FALSE)

#define TRACE_TYPE_METHOD(Cls) \
   template <typename Cls> std::shared_ptr<Cls ## Trace<Cls>> Store::Trace ## Cls ## Changes(const char* reason, Cls const& o, int category) \
   {  \
      ASSERT(!tracing_disabled_); \
      dm::ObjectId id = o.GetObjectId(); \
      std::shared_ptr<Cls ## Trace<Cls>> trace; \
      ADD_TRACE_TO_TRACKER_CATEGORY(trace, category, Cls); \
      traces_[id].push_back(trace); \
      return trace; \
   } \
   \
   template <typename Cls> std::shared_ptr<Cls ## Trace<Cls>> Store::Trace ## Cls ## Changes(const char* reason, Cls const& o, Tracer* tracer) \
   {  \
      ASSERT(!tracing_disabled_); \
      dm::ObjectId id = o.GetObjectId(); \
      std::shared_ptr<Cls ## Trace<Cls>> trace; \
      ADD_TRACE_TO_TRACER(trace, tracer, Cls) \
      traces_[id].push_back(trace); \
      return trace; \
   } \

TRACE_TYPE_METHOD(Boxed)
TRACE_TYPE_METHOD(Set)
TRACE_TYPE_METHOD(Array)
TRACE_TYPE_METHOD(Map)

#undef TRACE_TYPE_METHOD

template <> std::shared_ptr<RecordTrace<Record>> Store::TraceRecordChanges(const char* reason, Record const& o, int category)
{
   ASSERT(!tracing_disabled_);
   dm::ObjectId id = o.GetObjectId();
   std::shared_ptr<RecordTrace<Record>> trace;
   ADD_TRACE_TO_TRACKER_CATEGORY(trace, category, Record);
   traces_[id].push_back(trace);
   for (const auto& field : o.GetFields()) {
      recordFieldMap_[field.second] = o.GetObjectId();
   }
   return trace;
}

template <> std::shared_ptr<RecordTrace<Record>> Store::TraceRecordChanges(const char* reason, Record const& o, Tracer*tracer)
{
   ASSERT(!tracing_disabled_);
   dm::ObjectId id = o.GetObjectId();
   std::shared_ptr<RecordTrace<Record>> trace;
   ADD_TRACE_TO_TRACER(trace, tracer, Record)
   traces_[id].push_back(trace);
   for (const auto& field : o.GetFields()) {
      recordFieldMap_[field.second] = o.GetObjectId();
   }
   return trace;
}


#define CREATE_MAP(M)    template std::shared_ptr<MapTrace<M>> Store::TraceMapChanges(const char*, M const&, int); \
                         template std::shared_ptr<MapTrace<M>> Store::TraceMapChanges(const char*, M const&, Tracer*); \
                         template void Store::MarkChangedAndFire(M&, std::function<void(MapTrace<M>&)>); \
                         template void Store::OnMapRemoved(M&, M::Key const&); \
                         template void Store::OnMapChanged(M&, M::Key const&, M::Value const&);

#define CREATE_SET(S)    template std::shared_ptr<SetTrace<S>> Store::TraceSetChanges(const char*, S const&, int); \
                         template std::shared_ptr<SetTrace<S>> Store::TraceSetChanges(const char*, S const&, Tracer*); \
                         template void Store::MarkChangedAndFire(S&, std::function<void(SetTrace<S>&)>); \
                         template void Store::OnSetRemoved(S&, S::Value const&); \
                         template void Store::OnSetAdded(S&, S::Value const&); \

#define CREATE_BOXED(B)  template std::shared_ptr<BoxedTrace<B>> Store::TraceBoxedChanges(const char*, B const&, int); \
                         template std::shared_ptr<BoxedTrace<B>> Store::TraceBoxedChanges(const char*, B const&, Tracer*); \
                         template void Store::MarkChangedAndFire(B&, std::function<void(BoxedTrace<B>&)>); \
                         template void Store::OnBoxedChanged(B&, B::Value const&);

#define CREATE_ARRAY(A)  template std::shared_ptr<ArrayTrace<A>> Store::TraceArrayChanges(const char*, A const&, int); \
                         template std::shared_ptr<ArrayTrace<A>> Store::TraceArrayChanges(const char*, A const&, Tracer*); \
                         template void Store::MarkChangedAndFire(A&, std::function<void(ArrayTrace<A>&)>); \
                         template void Store::OnArrayChanged(A&, A::Value const&);

#include "types/all_types.h"
ALL_DM_TYPES
