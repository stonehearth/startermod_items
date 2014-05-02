#include "pch.h"
#include "selection.h"
#include "entity.h"

using namespace ::radiant;
using namespace ::radiant::om;

Selection::Selection() :
   flags_(0)
{
}

bool Selection::operator==(Selection const& other) const
{
   return false;
}

void Selection::Clear()
{
   flags_ = 0;
}

void Selection::AddBlock(const csg::Point3& location)
{
   flags_ |= HAS_BLOCK;
   block_ = location;
}

bool Selection::HasBlock() const
{
   return (flags_ & HAS_BLOCK) != 0;
}

const csg::Point3& Selection::GetBlock() const
{
   return block_;
}


bool Selection::HasLocation() const
{
   return (flags_ & HAS_LOCATION) != 0;
}

csg::Point3f Selection::GetLocation() const
{
   return location_;
}

void Selection::AddLocation(const csg::Point3f& location, const csg::Point3f& normal)
{
   flags_ |= HAS_LOCATION;
   location_ = location;
   normal_ = normal;
}

void Selection::AddString(std::string const& str)
{
   ASSERT((flags_ & HAS_STRING) == 0);
   flags_ |= HAS_STRING;
   string_ = str;
}

void Selection::AddNumber(int number)
{
   ASSERT((flags_ & HAS_NUMBER) == 0);
   flags_ |= HAS_NUMBER;
   number_ = number;
}

const csg::Point3f& Selection::GetNormal()
{
   return normal_;
}

bool Selection::HasEntities()
{
   return entities_.size() > 0;
}
   
void Selection::AddEntity(const dm::ObjectId id)
{
   entities_.push_back(id);
}
   
void Selection::SetBounds(const csg::Cube3& bounds)
{
   flags_ |= HAS_BOUNDS;
   bounds_ = bounds;
}

void Selection::FromLuaObject(luabind::object o)
{
   Clear();
   // try entity...
   try {
      EntityPtr e = luabind::object_cast<om::EntityRef>(o).lock();
      if (e) {
         entities_.push_back(e->GetObjectId());
      }
      return;
   } catch (luabind::cast_failed& ) { }

   // try string...
   try {
      AddString(luabind::object_cast<std::string>(o));
      return;
   } catch (luabind::cast_failed& ) { }

   ASSERT(false);
}

void Selection::SaveValue(Protocol::Selection* msg) const
{
   if (entities_.size() > 0) {
      msg->set_entity(entities_.front());
   }
   if ((flags_ & HAS_BLOCK) != 0) {
      block_.SaveValue(msg->mutable_block());
   }
   if ((flags_ & HAS_BOUNDS) != 0) {
      bounds_.SaveValue(msg->mutable_bounds());
   }
   if ((flags_ & HAS_OBJECT) != 0) {
      msg->set_object(objectId_);
      msg->set_object_type(objectType_);
   }
   if ((flags_ & HAS_NUMBER) != 0) {
      msg->set_number(number_);
   }
   if ((flags_ & HAS_STRING) != 0) {
      msg->set_string(string_);
   }
}

void Selection::LoadValue(const Protocol::Selection& msg)
{
   Clear();

   dm::ObjectId entityId = msg.entity();
   if (entityId != 0) {
      AddEntity(entityId);
   }
   if (msg.has_block()) {
      csg::Point3 pt;
      pt.LoadValue(msg.block());
      AddBlock(pt);
   }
   if (msg.has_bounds()) {
      csg::Cube3 bounds;
      bounds.LoadValue(msg.bounds());
      SetBounds(bounds);
   }
   if (msg.has_string()) {
      AddString(msg.string());
   }
   if (msg.has_number()) {
      AddNumber(msg.number());
   }
   if (msg.has_object()) {
      objectId_ = msg.object();
      objectType_ = msg.object_type();
   }
}

std::ostream& 
Selection::Format(std::ostream& out) const
{
    return (out << "(Selection ...)");
}   // End of operator<<()
