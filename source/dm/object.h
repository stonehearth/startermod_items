#ifndef _RADIANT_DM_OBJECT_H
#define _RADIANT_DM_OBJECT_H

#include "dm.h"
#include "object_macros.h"
#include <unordered_map>

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

class Object
{
public:
   Object();
   bool operator==(Object const &rhs) const;
   virtual const char *GetObjectClassNameLower() const = 0;
   virtual void GetDbgInfo(DbgInfo &info) const = 0;
   virtual void LoadValue(SerializationType r, Protocol::Value const& msg) = 0;
   virtual void SaveValue(SerializationType r, Protocol::Value* msg) const = 0;
   virtual TracePtr TraceObjectChanges(const char* reason, int category) const = 0;
   virtual TracePtr TraceObjectChanges(const char* reason, Tracer* tracer) const = 0;
   void LoadObject(SerializationType r, Protocol::Object const& msg);
   void SaveObject(SerializationType r, Protocol::Object* msg) const;   
   virtual void OnLoadObject(SerializationType r) { }

   virtual void LoadFromJson(json::Node const& obj);
   virtual void SerializeToJson(json::Node& node) const;
   std::string GetStoreAddress() const;

   //virtual void Initialize(Store& s, const Protocol::Object& msg);
   virtual ~Object();

   Store& GetStore() const;
   int GetStoreId() const { return id_.store; }
   ObjectId GetObjectId() const;
   GenerationId GetLastModified() const { return timestamp_; }

   virtual ObjectType GetObjectType() const = 0;

   virtual void ConstructObject() { };

   bool IsValid() const;

protected:
   friend Store;
   bool WriteDbgInfoHeader(DbgInfo &info) const;

private:
   friend Record;
   void SetObjectMetadata(ObjectId id, Store& store);

private:  
   friend Store;
   void MarkChanged(); // This thing doesn't actually fire traces!   Use the typed fns (e.g. OnMapChanged)

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


END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_OBJECT_H
