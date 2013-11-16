#pragma once
#include "object.h"
#include "tracer_sync.h"
#include "tracer_buffered.h"
#include "streamer.h"

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
   typedef std::unordered_map<ObjectId, TraceList> TraceMap;

   typedef std::unordered_map<int, TracerPtr> TracerMap;

public:
   // New hotness.  These are the demuxes.  Fan out a single "hi, i'm a map and i've changed!"
   // to every single MapTrace (some of which are sync and some of which are buffered).

   template <typename M> void OnMapRemoved(M const& map, typename M::Key const& key)
   {
      map.MarkChanged();
      auto i = traces_.find(map.GetObjectId());
      if (i != traces_.end()) {
         stdutil::ForEachPrune(i->second, [](std::shared_ptr<Trace> t) {
            std::static_pointer_cast<MapTrace<M>>(t)->OnRemoved(key);
         });
      }
   }
   template <typename M> void OnMapChanged(M const& map, typename M::Key const& key, typename M::Key const& value)
   {
      map.MarkChanged();
      auto i = traces_.find(map.GetObjectId());
      if (i != traces_.end()) {
         stdutil::ForEachPrune(i->second, [](std::shared_ptr<Trace> t) {
            std::static_pointer_cast<MapTrace<M>>(t)->OnChanged(key, value);
         });
      }
   }

#define CALL_TRACE_SET(category, invocation) \
   do { \
      auto t = GetTracer(category); \
      switch (t->GetType()) { \
      case TRACER_SYNC: \
         std::static_pointer_cast<TracerSync>(t)->invocation; \
         break; \
      case TRACER_BUFFERED: \
         std::static_pointer_cast<TracerBuffered>(t)->invocation; \
         break; \
      case TRACER_BUFFERED: \
         std::static_pointer_cast<Streamer>(t)->invocation; \
         break; \
      default: \
         throw std::logic_error(BUILD_STRING("unknown tracker set type " << t->GetType())); \
      } \
   } while (FALSE)

   // useful so we don't have to pass the tracker set all over the world.  refer to it by category
   template <typename M> std::shared_ptr<MapTrace<M>> TraceMapChanges(const char* reason, M const* map, int category)
   {
      dm::ObjectId id = map.GetObjectId();
      std::shared_ptr<MapTrace<M>> tracker;

      tracker = CALL_TRACE_SET(category, TraceMapChanges(reason, map));
      traces_[id].push_back(tracker);
      return tracker;
   }

   TracerPtr GetTracer(int category)
   {
      auto i = tracers_.find(category);
      if (i == tracers_.end()) {
         throw std::logic_error(BUILD_STRING("store has no tracker set for category " << category));
      }
      return i->second;
   }

   void AddTracer(int category, TracerPtr set)
   {
      auto entry = tracers_.insert(std::make_pair(category, set));
      if (entry.second) {
         throw std::logic_error(BUILD_STRING("duplicate tracer category " << category));
      }
   }

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

   std::vector<ObjectRef>  alloced_;
   std::vector<ObjectId>   destroyed_;

   std::unordered_map<ObjectId, Object*>                     objects_;
   std::unordered_map<ObjectType, ObjectAllocFn>             allocators_;
   mutable std::unordered_map<ObjectId, DynamicObject>       dynamicObjects_;

   TraceMap     traces_;
   TracerMap    tracers_;
};


END_RADIANT_DM_NAMESPACE
