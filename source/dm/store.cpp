#include "radiant.h"
#include "store.h"
#include "store_trace.h"
#include "tracer_sync.h"
#include "tracer_buffered.h"
#include "record.h"
#include "object.h"

using namespace ::radiant;
using namespace ::radiant::dm;

#define STORE_LOG(level)      LOG(dm.store, level)

Store*  Store::stores_[4 + 1] = { 0 };

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
   name_(name)
{
   ASSERT(storeId_);

   // ASSERT(stores_[which] == nullptr);
   stores_[which] = this;
}

Store::~Store(void)
{
   Reset();
}

ObjectPtr Store::AllocObject(ObjectType t)
{
   ASSERT(allocators_[t]);

   ObjectPtr obj = allocators_[t]();
   obj->Initialize(*this, GetNextObjectId());
   OnAllocObject(obj);

   return obj;
}

ObjectPtr Store::AllocSlaveObject(ObjectType t, ObjectId id)
{
   ASSERT(nextObjectId_ == 1);
   ASSERT(allocators_[t]);

   ObjectPtr obj = allocators_[t]();
   obj->InitializeSlave(*this, id); 
   OnAllocObject(obj);

   return obj;
}

void Store::RegisterAllocator(ObjectType t, ObjectAllocFn allocator)
{
   ASSERT(!allocators_[t]);
   allocators_[t] = allocator;
}

void Store::Reset()
{
   dynamicObjects_.clear();

   ASSERT(objects_.size() == 0);
}

void Store::RegisterObject(Object& obj)
{
   ASSERT(obj.IsValid());
   if (objects_.find(obj.GetObjectId()) != objects_.end()) {
      throw std::logic_error(BUILD_STRING("data mode object " << obj.GetObjectId() << "being registered twice."));
   }

   ObjectId id = obj.GetObjectId();
   Object* pobj = &obj;
   objects_[id] = pobj;
}

void Store::SignalRegistered(Object const* pobj)
{
   stdutil::ForEachPrune<StoreTrace>(store_traces_, [=](StoreTracePtr trace) {
      trace->SignalRegistered(pobj);
   });

   // std::cout << "Store " << storeId_ << " RegisterObject(oid:" << obj.GetObjectId() << ") " << typeid(obj).name() << std::endl;
}


void Store::UnregisterObject(const Object& obj)
{
   // std::cout << "Store " << storeId_ << " UnregisterObject(oid:" << obj.GetObjectId() << ")" << std::endl;
   ValidateObjectId(obj.GetObjectId());

   ObjectId id = obj.GetObjectId();

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
   }

   stdutil::ForEachPrune<StoreTrace>(store_traces_, [=](StoreTracePtr trace) {
      trace->SignalDestroyed(id, dynamic);
   });

   destroyed_.push_back(id);
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
   dynamicObjects_[id] = DynamicObject(obj, obj->GetObjectType());

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
      const DynamicObject& entry = i->second;
      obj = entry.object.lock();
      if (obj) {
         ASSERT(obj->IsValid());
         // ASSERT(type == -1 || entry.type == type); Fails in cases where dynamic cast may succeed
      } else {
         dynamicObjects_.erase(i);
      }
   }
   return obj;
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
      ObjectPtr obj = entry.second.object.lock();
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
