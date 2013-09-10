#include "pch.h"
#include "store.h"

using namespace ::radiant;
using namespace ::radiant::dm;

Store*  Store::stores_[4 + 1] = { 0 };

Store& Store::GetStore(int id)
{
   if (!id) {
      LOG(WARNING) << "object asked for store id 0.  we're about to die...";
   }
   ASSERT(id);
   return *stores_[id];
}

Store::Store(int which, std::string const& name) :
   storeId_(which),
   nextObjectId_(1),
   nextGenerationId_(1),
   nextTraceId_(1),
   firingCallbacks_(false),
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
   ASSERT(objects_.find(obj.GetObjectId()) == objects_.end());

   objects_[obj.GetObjectId()] = &obj;

   // std::cout << "Store " << storeId_ << " RegisterObject(oid:" << obj.GetObjectId() << ") " << typeid(obj).name() << std::endl;
}


void Store::UnregisterObject(const Object& obj)
{
   // std::cout << "Store " << storeId_ << " UnregisterObject(oid:" << obj.GetObjectId() << ")" << std::endl;
   ValidateObjectId(obj.GetObjectId());

   ObjectId id = obj.GetObjectId();

   objects_.erase(id);
   dynamicObjects_.erase(id);

   if (!firingCallbacks_) {
      //LOG(WARNING) << "adding " << id << " to destroyed.";
      destroyed_.push_back(id);
   } else {
      //LOG(WARNING) << "adding " << id << " to deferred destroyed.";
      deferredDestroyedObjects_.push_back(id);
   }      
}

Guard Store::TraceDynamicObjectAlloc(ObjectAllocCb fn)
{
   return AddTrace(allocTraces_, -1, "dynamic alloc trace", fn);
}

Guard Store::TraceObjectChanges(const Object& obj, const char* reason, ObjectChangeCb fn)
{
   return AddTrace(changeTraces_, obj.GetObjectId(), reason, fn);
}

Guard Store::TraceObjectLifetime(const Object& obj, const char* reason, ObjectDestroyCb fn)
{
   return AddTrace(destroyTraces_, obj.GetObjectId(), reason, fn);
}


Guard Store::AddTraceFn(TraceObjectMap &traceMap, ObjectId oid, const char* reason, boost::any fn)
{
   TraceId tid = nextTraceId_++;

   ValidateObjectId(oid);

   // LOG(WARNING) << "creating trace '" << reason << "'.";
   if (firingCallbacks_) {
      deferredTraces_.insert(std::make_pair(tid, TraceReservation(traceMap, oid, reason, fn)));
      //LOG(WARNING) << "deferring add trace " << reason;
   } else {
      traceMap[oid].push_back(tid);
      traceCallbacks_[tid] = Trace(reason, fn);
      //LOG(WARNING) << "adding trace " << tid << " : " << traceCallbacks_[tid].reason;
   }

   return Guard(std::bind(&Store::RemoveTrace, this, std::ref(traceMap), oid, tid));
}

void Store::RemoveTrace(TraceObjectMap &traceMap, ObjectId oid, TraceId tid)
{
   if (firingCallbacks_) {
      deadTraces_.insert(std::make_pair(tid, DeadTrace(traceMap, tid, oid)));
      return;
   }
   // Remove the trace from the global list
   auto i = traceCallbacks_.find(tid);
   ASSERT(i != traceCallbacks_.end());
   if (i != traceCallbacks_.end()) {
      // LOG(WARNING) << "removing trace " << tid << " : " << i->second.reason;
      // LOG(WARNING) << "removing trace '" << i->first << "'.";
      traceCallbacks_.erase(i);

      // Remove the trace from the TraceObjectMap
      auto j = traceMap.find(oid);
      if (j != traceMap.end()) {
         auto& v = j->second;
         ASSERT(v.size() > 0);

         stdutil::FastRemove(v, tid);
         if (v.empty()) {
            traceMap.erase(j);
         }
      }
      return;
   }

   auto j = deferredTraces_.find(tid);
   if (j != deferredTraces_.end()) {
      deferredTraces_.erase(j);
      return;
   }
   ASSERT(false); // duplicate trace removal!
}


void Store::ValidateTraceId(TraceId tid) const
{
   ASSERT(tid > 0 && tid < nextTraceId_);
   ASSERT(traceCallbacks_.find(tid) != traceCallbacks_.end());
   ASSERT(deadTraces_.find(tid) == deadTraces_.end());
}

void Store::ValidateObjectId(ObjectId oid) const
{
   ASSERT(oid == -1 || objects_.find(oid) != objects_.end());
}

int Store::GetObjectCount() const
{
   return objects_.size();
}

int Store::GetTraceCount() const
{
   return traceCallbacks_.size();
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
   if (!firingCallbacks_) {
      // LOG(WARNING) << "adding " << id << " to alloced.";
      alloced_.push_back(obj);
   } else {
      // LOG(WARNING) << "adding " << id << " to deferred alloced.";
      deferredAllocedObjects_.push_back(obj);
   }
}

Object* Store::FetchStaticObject(ObjectId id)
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

void Store::OnObjectChanged(const Object& obj)
{
   ObjectId id = obj.GetObjectId();
   
   if (!firingCallbacks_) {
      stdutil::UniqueInsert(modifiedObjects_, id);
   } else {
      stdutil::UniqueInsert(deferredModifiedObjects_, id);
   }
}

void Store::FireTraces()
{
   firingCallbacks_ = true;

   ASSERT(deadTraces_.empty());
   ASSERT(deferredTraces_.empty());
   ASSERT(deferredModifiedObjects_.empty());
   ASSERT(deferredAllocedObjects_.empty());
   ASSERT(deferredDestroyedObjects_.empty());

   // xxx: This is quite a tremendous hack.  Each object only uses 1 datum to track
   // the deltas for both pending traces and object remoting.  This datum is cleared
   // at the end of a frame when we encode objects.  This means there cannot be any
   // outstanding traces at encode time, or we just completely miss out on firing it.
   // Loop until there are no more traces (this could cause an infinite loop, but we
   // hope clients are conservative in the implementation of their callbacks).
   //
   // A real fix would be to track trace state and object encoding state independanctly,
   // but that's quite a bit to take on right now.
   do {
      for (ObjectRef o : alloced_) {
         auto obj = o.lock();
         if (obj) {
            //LOG(WARNING) << "firing " << obj->GetObjectId() << " alloc cb.";
            for (TraceId tid : allocTraces_[-1]) {
               if (!stdutil::contains(deadTraces_, tid)) {
                  ValidateTraceId(tid);
                  ObjectAllocCb cb = boost::any_cast<ObjectAllocCb>(traceCallbacks_[tid].cb);
                  if (cb) {
                     cb(obj);
                  }
               }
            }
         }
      }

      for (ObjectId id : destroyed_) {
         auto i = destroyTraces_.find(id);
         if (i != destroyTraces_.end()) {
            //LOG(WARNING) << "firing " << id << " destroy cb.";
            for (TraceId tid : i->second) {
               if (!stdutil::contains(deadTraces_, tid)) {
                  ValidateTraceId(tid);
                  ObjectDestroyCb cb = boost::any_cast<ObjectDestroyCb>(traceCallbacks_[tid].cb);
                  if (cb) {
                     cb();
                  }
               }
            }
         }
      }

      for (ObjectId id : modifiedObjects_) {
         if (objects_.find(id) != objects_.end()) {
            //LOG(WARNING) << "firing " << id << " change cb.";
            auto i = changeTraces_.find(id);
            if (i != changeTraces_.end()) {
               for (TraceId tid : i->second) {
                  if (!stdutil::contains(deadTraces_, tid)) {
                     ValidateTraceId(tid);
                     LOG(INFO) << "firing trace " << tid << " : " << traceCallbacks_[tid].reason;
                     ObjectChangeCb cb = boost::any_cast<ObjectChangeCb>(traceCallbacks_[tid].cb);
                     if (cb) {
                        cb();
                     }
                  }
               }
            }
         } else {
            // LOG(WARNING) << "ignoring change cb on invalid object " << id;
         }
      }

      modifiedObjects_ = std::move(deferredModifiedObjects_);
      alloced_ = std::move(deferredAllocedObjects_);
      destroyed_ = std::move(deferredDestroyedObjects_);
   } while (!modifiedObjects_.empty());

   firingCallbacks_ = false;

   for (const auto& entry : deferredTraces_) {
      TraceId tid = entry.first;
      const TraceReservation& r = entry.second;
      r.traceMap[r.oid].push_back(tid);
      traceCallbacks_[tid] = Trace(r.reason, r.cb);
      // LOG(WARNING) << "adding deferred trace " << tid << " : " << traceCallbacks_[tid].reason;
   }
   for (const auto &entry : deadTraces_) {
      const DeadTrace& d = entry.second;
      RemoveTrace(d.traceMap, d.oid, d.tid);
   }
   deferredTraces_.clear();
   deadTraces_.clear();

   ASSERT(modifiedObjects_.empty());
   ASSERT(deferredModifiedObjects_.empty());
}

bool Store::IsDynamicObject(ObjectId id)
{
   return dynamicObjects_.find(id) != dynamicObjects_.end();
}
