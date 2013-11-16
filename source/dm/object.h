#ifndef _RADIANT_DM_OBJECT_H
#define _RADIANT_DM_OBJECT_H

#include "dm.h"
#include "all_objects_types.h"
#include "store.pb.h"
#include "dm_save_impl.h"
#include "dbg_info.h"
#include <unordered_map>

#define IMPLEMENT_DYNAMIC_TO_STATIC_DISPATCH(Cls) \
   void Load(Protocol::Object const& msg) override \
   {  \
      dm::LoadObject(msg); \
      LoadHeader(msg); \
   } \
   \
   std::shared_ptr<Cls ## Trace<Cls>> Trace ## Cls ## Changes(const char* reason, int category) \
   { \
      return GetStore().Trace ## Cls ## Changes(reason, this, category); \
   }


BEGIN_RADIANT_DM_NAMESPACE

struct ObjectIdentifier {
#if 1
   unsigned int      id;
   unsigned int      store;
#else
   unsigned int      id:30;
   unsigned int      store:2;
#endif
};

// xxx: this is more like "object header".  it's just the metadata.
class Object
{
public:

   Object();
   Object(Object&& other);
   virtual const char *GetObjectClassNameLower() const = 0;
   virtual void GetDbgInfo(DbgInfo &info) const = 0;
   virtual void Load(Protocol::Object const& msg) = 0;

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

protected:
   void LoadHeader(Protocol::Object const& msg);

protected:
   friend Store;
   bool WriteDbgInfoHeader(DbgInfo &info) const;

public:
   void MarkChanged();

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

#endif // _RADIANT_DM_OBJECT_H