#pragma once
#include "types.h"
#include "guard.h"
#include "store.pb.h"
#include "namespace.h"
#include "all_objects_types.h"
#include "dm_save_impl.h"
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

   virtual Guard TraceObjectLifetime(const char* reason, std::function<void()> fn) const;
   virtual Guard TraceObjectChanges(const char* reason, std::function<void()> fn) const;

   void SaveObject(Protocol::Object* msg) const;
   void LoadObject(const Protocol::Object& msg);
   

protected:
   friend Store;

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
};

END_RADIANT_DM_NAMESPACE
