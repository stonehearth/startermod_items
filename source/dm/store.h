#pragma once
#include "object.h"
#include "core/guard.h"
#include "namespace.h"
#include "store.pb.h"
#include "lib/lua/bind.h"
#include <boost/any.hpp>

BEGIN_RADIANT_DM_NAMESPACE

class Store
{
public:
   Store(int id, std::string const& name);
   ~Store(void);

   typedef std::function<void()> ObjectChangeCb;
   typedef std::function<void()> ObjectDestroyCb;
   typedef std::function<void()> TracesFinishedCb;
   typedef std::function<void(ObjectPtr)> ObjectAllocCb;

   typedef std::function<ObjectPtr()> ObjectAllocFn;

   int GetStoreId() const { return storeId_; }
   
   void SetInterpreter(lua_State* L) { L_ = L; }
   lua_State* GetInterpreter() const { return L_; }
              
   std::string const& GetName() const { return name_; }
   void SetName(std::string const& name);

   int GetObjectCount() const;
   int GetTraceCount() const;

   std::vector<ObjectId> GetModifiedSince(GenerationId when);
   bool IsDynamicObject(ObjectId id);
   
   core::Guard TraceDynamicObjectAlloc(ObjectAllocCb fn);

   void RegisterAllocator(ObjectType t, ObjectAllocFn allocator);

   ObjectPtr AllocObject(ObjectType t);
   ObjectPtr AllocSlaveObject(ObjectType t, ObjectId id);

   template<class T> std::shared_ptr<T> AllocObject() {
      auto obj = std::make_shared<T>();
      obj->Initialize(*this, GetNextObjectId());
      OnAllocObject(obj);
      return obj;
   }


   template <class T>
   std::shared_ptr<T> SharedFromThis(T* that) const {
      dm::ObjectPtr obj = FetchObject(that->GetObjectId(), -1);
      std::shared_ptr<T> self = std::dynamic_pointer_cast<T>(obj);
      ASSERT(that == self.get());
      return self;
   }

   std::shared_ptr<Object> FetchObject(ObjectId id, ObjectType type) const;
   template<class T> std::shared_ptr<T> FetchObject(ObjectId id) const {
      ObjectPtr obj = FetchObject(id, T::DmType);
      if (obj) {
         std::shared_ptr<T> result = std::dynamic_pointer_cast<T>(obj);
         ASSERT(result);
         return result;
      }
      return nullptr;
   }
   template<> std::shared_ptr<Object> FetchObject(ObjectId id) const {
      return FetchObject(id, -1);
   }

   template<class T> T* FetchStaticObject(ObjectId id) {
      Object* obj = FetchStaticObject(id);
      T* result = dynamic_cast<T*>(obj);
      ASSERT((result && obj) || (!result && !obj)); // xxx - it would be nice to check the typeid here, too
      return result;
   }
   Object* FetchStaticObject(ObjectId id);

   void FireTraces();
   void FireFinishedTraces();
   void Reset();
   GenerationId GetNextGenerationId();
   GenerationId GetCurrentGenerationId();
   ObjectId GetNextObjectId();
   core::Guard TraceFinishedFiringTraces(const char* reason, TracesFinishedCb fn);

protected: // Internal interface for Objects only
   friend Object;
   static Store& GetStore(int id);
   void RegisterObject(Object& obj);
   void UnregisterObject(const Object& obj);
   void OnObjectChanged(const Object& obj);
   core::Guard TraceObjectLifetime(const Object& obj, const char* reason, ObjectDestroyCb fn);
   core::Guard TraceObjectChanges(const Object& obj, const char* reason, ObjectChangeCb fn);
   void OnAllocObject(std::shared_ptr<Object> obj);

private:
   // Types of trace functions.

   // The TraceMap is a mapping from trace id's to callback functions.  It exists
   // only because c++ function objects do not implement operator==, therefore
   // we need a unique token so that we can compare them (to handle auto unregisration
   // when the core::Guard object is destroyed).  Use a boost::any so we can store any
   // types of cb function here.
   struct Trace {
      Trace() { }
      Trace(const char *r, boost::any c) : reason(r), cb(c) { };

      const char *reason;
      boost::any cb;
   };

   typedef std::unordered_map<TraceId, Trace> TraceMap;


   // The TraceObjectMap is a map of all the notifications of a certain type on
   // a specified object.  For example, the dtors_ map contains all the trace id's
   // of callback functions registered for object destruction notification (for all
   // objects).
   typedef std::unordered_map<ObjectId, std::vector<TraceId> > TraceObjectMap;


   struct TraceReservation : public Trace {
      TraceReservation(TraceObjectMap& m, ObjectId o, const char *r, boost::any cb) : Trace(r, cb), traceMap(m), oid(o) { }

      TraceObjectMap& traceMap;
      ObjectId oid;
   };

   struct DeadTrace {
      DeadTrace(TraceObjectMap& m, TraceId t, ObjectId o) : traceMap(m), tid(t), oid(o) { }

      TraceObjectMap& traceMap;
      ObjectId oid;
      TraceId tid;
   };

   template <typename T> core::Guard AddTrace(TraceObjectMap &traceMap, ObjectId id, const char* reason, T fn) {
      return AddTraceFn(traceMap, id, reason, boost::any(fn));
   }

   core::Guard AddTraceFn(TraceObjectMap &traceMap, ObjectId oid, const char* reason, boost::any fn);
   void RemoveTrace(TraceObjectMap &traceMap, ObjectId oid, TraceId tid);
   void ValidateTraceId(TraceId tid) const;
   void ValidateObjectId(ObjectId oid) const;

private:
   NO_COPY_CONSTRUCTOR(Store);

   static Store*  stores_[5];

   struct DynamicObject {
      std::weak_ptr<Object>      object;
      ObjectType                 type;

      DynamicObject() { }
      DynamicObject(std::weak_ptr<Object> o, ObjectType t) : object(o), type(t) { }
   };

   typedef std::unordered_map<dm::TraceId, std::function<void(ObjectPtr)>> TraceAllocMap;

   int            storeId_;
   std::string    name_;
   ObjectId       nextObjectId_;
   GenerationId   nextGenerationId_;
   TraceId        nextTraceId_;
   lua_State*     L_;

   TraceMap                traceCallbacks_;
   TraceObjectMap          allocTraces_;
   TraceObjectMap          changeTraces_;
   TraceObjectMap          destroyTraces_;
   TraceObjectMap          finishedFiringTraces_;

   std::vector<ObjectId>   modifiedObjects_;
   std::vector<ObjectRef>  alloced_;
   std::vector<ObjectId>   destroyed_;

   std::vector<ObjectId>   deferredModifiedObjects_;
   std::vector<ObjectRef>  deferredAllocedObjects_;
   std::vector<ObjectId>   deferredDestroyedObjects_;
   std::unordered_map<TraceId, TraceReservation> deferredTraces_;
   std::unordered_map<TraceId, DeadTrace> deadTraces_;

   bool                    firingCallbacks_;

   std::unordered_map<ObjectId, Object*>                     objects_;
   std::unordered_map<ObjectType, ObjectAllocFn>             allocators_;
   mutable std::unordered_map<ObjectId, DynamicObject>       dynamicObjects_;
};


END_RADIANT_DM_NAMESPACE
