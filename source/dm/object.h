#pragma once
#include "types.h"
#include "core/guard.h"
#include "store.pb.h"
#include "namespace.h"
#include "all_objects_types.h"
#include "dm_save_impl.h"
#include "lib/lua/bind.h"
#include "dbg_info.h"
#include <unordered_map>

BEGIN_RADIANT_DM_NAMESPACE

class Store;
class Object;

typedef std::weak_ptr<Object>  ObjectRef;
typedef std::shared_ptr<Object>  ObjectPtr;

// xxx: this is more like "object header".  it's just the metadata.
class Object
{
public:
   Object();
   Object(Object&& other);

   virtual const char *GetObjectClassNameLower() const = 0;
   virtual void GetDbgInfo(DbgInfo &info) const = 0;

   virtual void Initialize(Store& s, ObjectId id);
   virtual void InitializeSlave(Store& s, ObjectId id);
   //virtual void Initialize(Store& s, const Protocol::Object& msg);
   virtual ~Object();

   Store& GetStore() const;
   int GetStoreId() const { return id_.store; }
   ObjectId GetObjectId() const;
   GenerationId GetLastModified() const { return timestamp_; }

   virtual ObjectType GetObjectType() const = 0;

   bool IsValid() const;

   virtual core::Guard TraceObjectLifetime(const char* reason, std::function<void()> fn) const;
   virtual core::Guard TraceObjectChanges(const char* reason, std::function<void()> fn) const;

   template <class T>
   class LuaPromise
   {
   public:
      LuaPromise(const char* reason, T const& c) {
         auto changed = [=]() {
            for (auto& cb : cbs_) {
               try {
                  luabind::call_function<void>(cb);
               } catch (std::exception const& e) {
                  LOG(WARNING) << "lua error firing trace: " << e.what();
               }
            }
         };
         callback_thread_ = c.GetStore().GetInterpreter();
         guard_ = c.TraceObjectChanges(reason, changed);
      }

   public:
      static luabind::scope RegisterLuaType(struct lua_State* L) {
         using namespace luabind;
         const char* name = typeid(T).name();
         const char* last = strrchr(name, ':');
         static std::string tname = std::string(last ? last + 1 : name) + "Trace";

         return
            class_<LuaPromise<T>>(tname.c_str())
               .def(tostring(const_self))               
               .def("on_changed",    &LuaPromise::PushChangedCb)
               .def("destroy",       &LuaPromise::Destroy)
            ;
      }

      void Destroy() {
         guard_.Clear();
      }

      LuaPromise* PushChangedCb(luabind::object cb) {
         luabind::object fn(callback_thread_, cb);
         cbs_.push_back(fn);
         return this;
      }

   private:
      lua_State*                      callback_thread_;
      core::Guard                     guard_;
      std::vector<luabind::object>    cbs_;
   };

   void SaveObject(Protocol::Object* msg) const;
   void LoadObject(const Protocol::Object& msg);
   

protected:
   friend Store;
   bool WriteDbgInfoHeader(DbgInfo &info) const;

public:
   void MarkChanged();

public:
   virtual void SaveValue(const Store& store, Protocol::Value* msg) const = 0;
   virtual void LoadValue(const Store& store, const Protocol::Value& msg) = 0;
   
private:
   // Do not allow copying or accidental copying creation.  Because objects
   // do not implement operator=, they cannot be put in stl containers, nor
   // can they be put in any storage layer containers (Set, Map, etc).  If
   // you need to create a Set of Objects, you must make a Set of ObjectRef's,
   // and dynamically allocate those objects.  This should not be a big
   // let down to you. =)
   NO_COPY_CONSTRUCTOR(Object);

private:
   ObjectIdentifier     id_;
   GenerationId         timestamp_;
};

template <typename T>
static std::ostream& operator<<(std::ostream& os, const Object::LuaPromise<T>& in)
{
   const char* name = typeid(T).name();
   const char* last = strrchr(name, ':');
   name = last ? last + 1 : name;
   os << "[" << name << " Promise]";
   return os;
}

// TODO: only for T's which are Objects!
template <class T>
struct SaveImpl<std::shared_ptr<T>>
{
   static void SaveValue(const Store& store, Protocol::Value* msg, const std::shared_ptr<T>& value) {
      ObjectId id = value ? value->GetObjectId() : 0;
      msg->SetExtension(Protocol::Ref::ref_object_id, id);
   }
   static void LoadValue(const Store& store, const Protocol::Value& msg, std::shared_ptr<T>& value) {
      ObjectId id = msg.GetExtension(Protocol::Ref::ref_object_id);
      value = store.FetchObject<T>(id);
   }
   static void GetDbgInfo(std::shared_ptr<T> obj, DbgInfo &info) {
      if (obj) {
         info.os << "[shared_ptr ";
         SaveImpl<T>::GetDbgInfo(*obj, info);
         info.os << "]";
      } else {
         info.os << "[shared_ptr nullptr]";
      }
   }
};

// TODO: only for T's which are DYNAMIC!! Objects!
template <class T>
struct SaveImpl<std::weak_ptr<T>>
{
   static void SaveValue(const Store& store, Protocol::Value* msg, const std::weak_ptr<T>& value) {
      SaveImpl<std::shared_ptr<T>>().SaveValue(store, msg, value.lock());
   }
   static void LoadValue(const Store& store, const Protocol::Value& msg, std::weak_ptr<T>& value) {
      ObjectId id = msg.GetExtension(Protocol::Ref::ref_object_id);
      value = store.FetchObject<T>(id);
   }
   static void GetDbgInfo(std::weak_ptr<T> o, DbgInfo &info) {
      auto obj = o.lock();
      if (obj) {
         info.os << "[weak_ptr ";
         SaveImpl<T>::GetDbgInfo(*obj, info);
         info.os << "]";
      } else {
         info.os << "[weak_ptr nullptr]";
      }
   }
};

END_RADIANT_DM_NAMESPACE
