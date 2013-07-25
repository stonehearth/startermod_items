#pragma once
#include "types.h"
#include "guard.h"
#include "store.pb.h"
#include "namespace.h"
#include "all_objects_types.h"
#include "dm_save_impl.h"
#include "formatter.h"
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

protected:
   void MarkChanged();

public:
   virtual void SaveValue(const Store& store, Protocol::Value* msg) const = 0;
   virtual void LoadValue(const Store& store, const Protocol::Value& msg) = 0;
   virtual void CloneObject(Object* copy, CloneMapping& mapping) const = 0;
   virtual std::ostream& Log(std::ostream& os, std::string indent) const = 0;
   
   template <class T> void CloneDynamicObject(std::shared_ptr<T> obj, CloneMapping& mapping) const
   {
      CloneDynamicObjectInternal(obj, mapping);
      RestoreWeakPointers(mapping);
   }


   template <class T> void CloneDynamicObjectInternal(std::shared_ptr<T> obj, CloneMapping& mapping) const
   {
      mapping.dynamicObjects[GetObjectId()] = obj;
      mapping.dynamicObjectsInverse[obj->GetObjectId()] = GetStore().FetchObject(GetObjectId(), -1);
      CloneObject(obj.get(), mapping);
   }

private:
   // Do not allow copying or accidental copying creation.  Because objects
   // do not implement operator=, they cannot be put in stl containers, nor
   // can they be put in any storage layer containers (Set, Map, etc).  If
   // you need to create a Set of Objects, you must make a Set of ObjectRef's,
   // and dynamically allocate those objects.  This should not be a big
   // let down to you. =)
   NO_COPY_CONSTRUCTOR(Object);

   void RestoreWeakPointers(CloneMapping& mapping) const;

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
   static void CloneValue(Store& store, const std::shared_ptr<T> value, std::shared_ptr<T>* copy, CloneMapping& mapping) {
      if (value) {
         ObjectId id = value->GetObjectId();
         auto i = mapping.dynamicObjects.find(id);
         if (i == mapping.dynamicObjects.end()) {
            auto c = std::dynamic_pointer_cast<T>(store.AllocObject(value->GetObjectType()));
            *copy = c;
            mapping.objects[id] = c->GetObjectId();
            if (!mapping.filter || !mapping.filter(value, c)) {
               value->CloneDynamicObjectInternal<T>(c, mapping);
            }
         } else {
            auto c = std::dynamic_pointer_cast<T>(i->second);
            ASSERT(c && value->GetObjectType() == c->GetObjectType());
         }
      } else {
         *copy = nullptr;
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
   static void CloneValue(Store& store, const std::weak_ptr<T> value, std::weak_ptr<T>* copy, CloneMapping& mapping) {
      auto v = value.lock();
      if (v) {
         dm::ObjectId id = v->GetObjectId();
         auto restore = [=, &mapping]() {
            LOG(WARNING) << copy << " restoring referencd to " << id;
            auto i = mapping.dynamicObjects.find(id);
            if (i != mapping.dynamicObjects.end()) {
               *copy = std::static_pointer_cast<T>(i->second);
            }
         };
         LOG(WARNING) << copy << " stashing reference to " << id << " for restoration.";
         mapping.restore.push_back(restore);
      }
   }
};

template <>
struct Formatter<Object*>
{
   static std::ostream& Log(std::ostream& out, const Object* value, const std::string indent) {
      return value->Log(out, indent);
   }
};
template <>
struct Formatter<const Object*>
{
   static std::ostream& Log(std::ostream& out, const Object* value, const std::string indent) {
      return value->Log(out, indent);
   }
};

template <class T>
struct Formatter<std::weak_ptr<T>>
{
   static std::ostream& Log(std::ostream& out, const std::weak_ptr<T> value, const std::string indent) {
      out << "std::weak_ptr<" << typeid(T).name() << "> -> ";
      auto v = value.lock();
      if (!v) {
         out << "nullptr";
      } else {
         out << "object " << v->GetObjectId();
      }
      return out;
   }
};

template <class T>
struct Formatter<std::shared_ptr<T>>
{
   static std::ostream& Log(std::ostream& out, const std::shared_ptr<T> value, const std::string indent) {
      out << "std::shared_ptr<" << typeid(T).name() << "> -> ";
      if (!value) {
         out << "nullptr";
      } else {
         value->Log(out, indent);
      }
      return out;
   }
};

END_RADIANT_DM_NAMESPACE
