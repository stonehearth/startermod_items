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
   saving_(false)
{
   ASSERT(storeId_);

   // ASSERT(stores_[which] == nullptr);
   stores_[which] = this;
}

Store::~Store(void)
{
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

void Store::RegisterAllocator(ObjectType t, ObjectAllocFn allocator)
{
   ASSERT(!allocators_[t]);
   allocators_[t] = allocator;
}

bool Store::Save(std::string const& filename, std::string &error)
{
   uint size;

#if 0
   {
      std::ofstream textfile("save.txt");

      Protocol::Object msg;
      std::set<ObjectId> keys;
      for (auto const& entry : objects_) {
         keys.insert(entry.first);
      };

      for (ObjectId id : keys) {
         Object* obj = objects_[id];
         msg.Clear();
         obj->SaveObject(PERSISTANCE, &msg);
         textfile << protocol::describe(msg) << "\n\n";
      }      
   }
#endif

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

         {
            Protocol::Store msg;
            msg.set_store_id(storeId_);
            msg.set_next_object_id(nextObjectId_);
            msg.set_next_generation(nextGenerationId_);
         
            cos.WriteLittleEndian32(msg.ByteSize());
            msg.SerializeToCodedStream(&cos);
         }

         {
            tesseract::protocol::Update update;
            update.set_type(tesseract::protocol::Update::AllocObjects);
            auto allocated_msg = update.MutableExtension(tesseract::protocol::AllocObjects::extension);
            for (const auto& entry : dynamicObjects_) {
               dm::ObjectPtr obj = entry.second.lock();
               if (obj) {
                  auto msg = allocated_msg->add_objects();
                  msg->set_object_id(entry.first);
                  msg->set_object_type(obj->GetObjectType());
               }
            }
            size = update.ByteSize();
            STORE_LOG(8) << "writing alloced objects size:" << size;
            cos.WriteLittleEndian32(size);
            update.SerializeToCodedStream(&cos);
         }

         {
            // sort so we save objects in the order they were created
            Protocol::Object msg;
            std::set<ObjectId> keys;
            for (auto const& entry : objects_) {
               keys.insert(entry.first);
            };

            cos.WriteLittleEndian32(keys.size());
            for (ObjectId id : keys) {
               Object* obj = objects_[id];

               msg.Clear();
               obj->SaveObject(PERSISTANCE, &msg);

               size = msg.ByteSize();
               STORE_LOG(8) << "writing object " << id << " size " << size;
               cos.WriteLittleEndian32(size);
               msg.SerializeToCodedStream(&cos);
            }
         }
      }
   }
   saving_ = false;
   _close(fd);
   return true;
}

bool Store::Load(std::string const& filename, std::string &error, ObjectMap& objects)
{
   // xxx: should probably just pass the save state into the ctor...
   ASSERT(traces_.empty());
   ASSERT(tracers_.empty());
   ASSERT(store_traces_.empty());
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

   {
      google::protobuf::io::FileInputStream fis(fd);
      {
         google::protobuf::io::CodedInputStream cis(&fis);

         {
            Protocol::Store msg;
            google::protobuf::uint32 size, limit;

            if (!cis.ReadLittleEndian32(&size)) {
               return false;
            }
            STORE_LOG(8) << "read alloced objects size:" << size;
            limit = cis.PushLimit(size);
            if (!msg.ParseFromCodedStream(&cis)) {
               return false;
            }
            cis.PopLimit(limit);

            nextObjectId_ = msg.next_object_id();
            nextGenerationId_ = msg.next_generation();
         }

         {
            google::protobuf::uint32 size, limit;
            tesseract::protocol::Update update;

            if (!cis.ReadLittleEndian32(&size)) {
               return false;
            }
            limit = cis.PushLimit(size);
            if (!update.ParseFromCodedStream(&cis)) {
               return false;
            }
            cis.PopLimit(limit);
            radiant::tesseract::protocol::AllocObjects const& alloc_msg = update.GetExtension(::radiant::tesseract::protocol::AllocObjects::extension);
            for (const tesseract::protocol::AllocObjects::Entry& entry : alloc_msg.objects()) {
               ObjectId id = entry.object_id();
               ASSERT(!FetchStaticObject(id));

               ObjectPtr obj = AllocObject(entry.object_type(), id);
               objects[id] = obj;
            }
         }

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
                  error = BUILD_STRING("could not read object " << i << " or " << total_objects);
                  return false;
               }
               int percent = i * 100 / total_objects;
               if (percent - last_reported_percent > 10) {
                  last_reported_percent = percent;
                  LOG_(0) << " load progress " << percent << "%...";
               }

               msg.Clear();
               limit = cis.PushLimit(size);
               if (!msg.ParseFromCodedStream(&cis)) {
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
            LOG_(0) << " load objects finished!" << std::endl;
         }
      }
      {
         for (auto const& entry : dynamicObjects_) {
            ObjectPtr obj = entry.second.lock();
            if (obj) {
               obj->OnLoadObject(PERSISTANCE);
            }
         }
      }
   }
   _close(fd);
   return true;
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
   return objects_.size();
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

void Store::OnAllocObject(std::shared_ptr<Object> obj)
{
   ASSERT(obj->IsValid());

   ObjectId id = obj->GetObjectId();
   dynamicObjects_[id] = obj;

   stdutil::ForEachPrune<StoreTrace>(store_traces_, [=](StoreTracePtr trace) {
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
}

void Store::OnRecordFieldChanged(Record const& record)
{
   ForEachTrace<RecordTrace<Record>>(record.GetObjectId(), [&](RecordTrace<Record>& trace) {
      trace.NotifyRecordChanged();
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
      dm::ObjectId id = o.GetObjectId(); \
      std::shared_ptr<Cls ## Trace<Cls>> trace; \
      ADD_TRACE_TO_TRACKER_CATEGORY(trace, category, Cls); \
      traces_[id].push_back(trace); \
      return trace; \
   } \
   \
   template <typename Cls> std::shared_ptr<Cls ## Trace<Cls>> Store::Trace ## Cls ## Changes(const char* reason, Cls const& o, Tracer* tracer) \
   {  \
      dm::ObjectId id = o.GetObjectId(); \
      std::shared_ptr<Cls ## Trace<Cls>> trace; \
      ADD_TRACE_TO_TRACER(trace, tracer, Cls) \
      traces_[id].push_back(trace); \
      return trace; \
   } \

TRACE_TYPE_METHOD(Record)
TRACE_TYPE_METHOD(Boxed)
TRACE_TYPE_METHOD(Set)
TRACE_TYPE_METHOD(Array)
TRACE_TYPE_METHOD(Map)

#undef TRACE_TYPE_METHOD

template std::shared_ptr<RecordTrace<Record>> Store::TraceRecordChanges(const char*, Record const&, int);
template std::shared_ptr<RecordTrace<Record>> Store::TraceRecordChanges(const char*, Record const&, Tracer*);

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
