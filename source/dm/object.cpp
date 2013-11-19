#include "radiant.h"
#include "object.h"
#include "store.h"

using namespace ::radiant;
using namespace ::radiant::dm;

Object::Object()
{
   id_.id = 0;
   id_.store = 0;
}

Object::Object(Object&& other)
{
   id_ = other.id_;
   other.id_.id = 0;
   other.id_.store = 0;
}

std::shared_ptr<ObjectTrace<Object>> Object::TraceObjectChanges(const char* reason, int category)
{
   return GetStore().TraceObjectChanges(reason, *this, category);
}

void Object::Initialize(Store& store, ObjectId id)
{
   // prevent stack based allocation of objects... too dangerous
#if 0
   char* stackpointer;
   __asm {
      mov stackpointer, esp;
   }
   int offset = abs(((char *)(this)) - stackpointer);
   if (offset < 64 * 1024) {
      ASSERT(false);
   }
#endif
   ASSERT(id);

   id_.id = id;
   id_.store = store.GetStoreId();

   MarkChanged();
   store.RegisterObject(*this);   
}

void Object::InitializeSlave(Store& store, ObjectId id)
{
   id_.id = id;
   id_.store = store.GetStoreId();
   store.RegisterObject(*this);   
   // A LoadValue's coming... don't worry
}

Object::~Object()
{
   if (id_.id) {
      ASSERT(id_.store);
      GetStore().UnregisterObject(*this);
   } else {
      ASSERT(!id_.store);
   }
}

ObjectId Object::GetObjectId() const
{
   ASSERT(IsValid());
   return id_.id;
}

void Object::MarkChanged()
{
   timestamp_ = GetStore().GetNextGenerationId();
}

Store& Object::GetStore() const
{
   return Store::GetStore(id_.store);
}

void Object::LoadHeader(Protocol::Object const& msg)
{
   id_.id = msg.object_id();
   timestamp_ = msg.timestamp();
}

bool Object::IsValid() const
{
   return id_.id != 0 && id_.store > 0 && id_.store < 4;
}

bool Object::WriteDbgInfoHeader(DbgInfo &info) const
{
   info.os << "[" << GetObjectClassNameLower() << " id:" << GetStoreId() << ":" << GetObjectId();
   if (info.IsVisisted(*this)) {
      info.os <<  " visisted ...]";
      return false;
   }
   info.MarkVisited(*this);
   info.os << " modified:" << GetLastModified() << "]";
   return true;
}
