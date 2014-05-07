#include "radiant.h"
#include "object.h"
#include "store.h"
#include "dbg_info.h"
#include "lib/json/node.h"
#include "protocols/store.pb.h"

using namespace ::radiant;
using namespace ::radiant::dm;

Object::Object()
{
   id_.id = 0;
   id_.store = 0;
}

// Object::operator==
//
// This is a bizarre little function.  It exists soley for the implementation of
// == in lua.  We know objects can't be copied, so they're only equal if their pointers
// are equal.  This also happens to work if this is NULL or &rhs is NULL, which means
// it works for invalid references (e.g. std::weak_ptr<Object> which has expired.
bool Object::operator==(Object const &rhs) const
{
   return this == &rhs;
}

void Object::SetObjectMetadata(ObjectId id, Store& store)
{
   id_.id = id;
   id_.store = store.GetStoreId();
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

void Object::LoadObject(SerializationType r, Protocol::Object const& msg)
{
   id_.id = msg.object_id();
   timestamp_ = msg.timestamp();
   LoadValue(r, msg.value());
}

void Object::SaveObject(SerializationType r, Protocol::Object* msg) const
{
   msg->set_object_id(id_.id);
   msg->set_object_type(GetObjectType());
   msg->set_timestamp(timestamp_);
   SaveValue(r, msg->mutable_value());
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

std::string Object::GetStoreAddress() const
{
   return BUILD_STRING("object://" << GetStore().GetName() << "/" << GetObjectId());
}

void Object::LoadFromJson(json::Node const& obj)
{
}

void Object::SerializeToJson(json::Node& node) const
{
   node.set("__self", GetStoreAddress());
   node.set("__type", GetObjectClassNameLower());
}

