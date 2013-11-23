#ifndef _RADIANT_DM_STORE_H
#define _RADIANT_DM_STORE_H

#include <unordered_map>
#include "dm.h"

struct lua_State;

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

   std::vector<ObjectId> GetModifiedSince(GenerationId when);
   bool IsDynamicObject(ObjectId id);

   void RegisterAllocator(ObjectType t, ObjectAllocFn allocator);

   ObjectPtr AllocObject(ObjectType t);
   ObjectPtr AllocSlaveObject(ObjectType t, ObjectId id);

   template<class T> std::shared_ptr<T> AllocObject() {
      auto obj = std::make_shared<T>();
      obj->Initialize(*this, GetNextObjectId());
      OnAllocObject(obj);
      return obj;
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

   template<class T> T* FetchStaticObject(ObjectId id) const {
      Object* obj = FetchStaticObject(id);
      T* result = dynamic_cast<T*>(obj);
      ASSERT((result && obj) || (!result && !obj)); // xxx - it would be nice to check the typeid here, too
      return result;
   }
   Object* FetchStaticObject(ObjectId id) const;

   void Reset();
   GenerationId GetNextGenerationId();
   GenerationId GetCurrentGenerationId();
   ObjectId GetNextObjectId();

private:
   typedef std::vector<TraceRef> TraceList;
   typedef std::vector<AllocTraceRef> AllocTraceList;
   typedef std::unordered_map<ObjectId, TraceList> TraceMap;

   typedef std::unordered_map<int, TracerPtr> TracerMap;

public:
   // New hotness.  These are the demuxes.  Fan out a single "hi, i'm a map and i've changed!"
   // to every single MapTrace (some of which are sync and some of which are buffered).

   template <typename T, typename TraceType> void MarkChangedAndFire(T& obj, std::function<void(typename TraceType&)> cb);
   template <typename T> void OnMapRemoved(T& map, typename T::Key const& key);
   template <typename T> void OnMapChanged(T& map, typename T::Key const& key, typename T::Value const& value);
   template <typename T> void OnSetRemoved(T& set, typename T::Value const& value);
   template <typename T> void OnSetAdded(T& set, typename T::Value const& value);
   template <typename T> void OnArrayChanged(T& arr, uint i, typename T::Value const& value);
   template <typename T> void OnBoxedChanged(T& boxed, typename T::Value const& value);

#define TRACE_TYPE_METHOD(Cls) \
   template <typename Cls> std::shared_ptr<Cls ## Trace<Cls>> Trace ## Cls ## Changes(const char* reason, Cls const& o, int category); \
   template <typename Cls> std::shared_ptr<Cls ## Trace<Cls>> Trace ## Cls ## Changes(const char* reason, Cls const& o, Tracer* tracer);

   TRACE_TYPE_METHOD(Object)
   TRACE_TYPE_METHOD(Record)
   TRACE_TYPE_METHOD(Boxed)
   TRACE_TYPE_METHOD(Set)
   TRACE_TYPE_METHOD(Array)
   TRACE_TYPE_METHOD(Map)

#undef TRACE_TYPE_METHOD

   AllocTracePtr TraceAlloced(const char* reason, int category);
   TracerPtr GetTracer(int category);
   void AddTracer(TracerPtr set, int category);
   void PushAllocState(AllocTrace& trace) const;

   template <typename T> void PushObjectState(ObjectTrace<T>& trace, ObjectId id) const;
   template <typename T> void PushBoxedState(BoxedTrace<T>& trace, ObjectId id) const;
   template <typename T> void PushSetState(SetTrace<T>& trace, ObjectId id) const;
   template <typename T> void PushMapState(MapTrace<T>& trace, ObjectId id) const;

protected: // Internal interface for Objects only
   friend Object;
   static Store& GetStore(int id);
   void RegisterObject(Object& obj);
   void UnregisterObject(const Object& obj);
   void OnObjectChanged(const Object& obj);
   void OnAllocObject(std::shared_ptr<Object> obj);

private:
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

   int            storeId_;
   std::string    name_;
   ObjectId       nextObjectId_;
   GenerationId   nextGenerationId_;
   TraceId        nextTraceId_;
   lua_State*     L_;

   std::vector<ObjectId>   destroyed_;

   std::unordered_map<ObjectId, Object*>                     objects_;
   std::unordered_map<ObjectType, ObjectAllocFn>             allocators_;
   mutable std::unordered_map<ObjectId, DynamicObject>       dynamicObjects_;

   TraceMap       traces_;
   TracerMap      tracers_;
};


END_RADIANT_DM_NAMESPACE

   
#endif // _RADIANT_DM_STORE_H
