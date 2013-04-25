#include "pch.h"
#include "record.h"
#include "store.h"

using namespace ::radiant;
using namespace ::radiant::dm;

Record::Record() : 
   slave_(false),
   registerOffset_(0),
   saveCount_(0)
{
}   

void Record::Initialize(Store& s, ObjectId id)
{
   Object::Initialize(s, id);
   InitializeRecordFields();
}

void Record::InitializeSlave(Store& s, ObjectId id)
{
   slave_ = true;
   Object::InitializeSlave(s, id);
}

dm::Guard Record::TraceRecordField(std::string name, const char* reason, std::function<void()> fn)
{
   for (const auto &field : fields_) {
      if (field.first == name) {
         Object* obj = GetStore().FetchStaticObject(field.second);

         ASSERT(obj);
         //LOG(WARNING) << "installing trace on record field " << name << " " << obj->GetObjectId();
         return obj->TraceObjectChanges(reason, fn);
      }
   }
   ASSERT(false);
   return dm::Guard();
}


void Record::AddRecordField(std::string name, Object& field)
{
   if (!slave_) {
      auto& store = GetStore();
      field.Initialize(store, store.GetNextObjectId());

      ObjectId id = field.GetObjectId();

      ASSERT(id);
      ASSERT(FieldIsUnique(name, field));
      ASSERT(GetObjectId() < id);

      fields_.push_back(std::make_pair(name, id));
   } else {
      field.Initialize(GetStore(), fields_[registerOffset_++].second);
   }
}

void Record::AddRecordField(Object& field)
{
   AddRecordField("", field);
}

void Record::SaveValue(const Store& store, Protocol::Value* msg) const
{
   ASSERT(!slave_);

   // A record gets all it's fields initialized prior to the first save and
   // never changes thereafter.  However, if someone mistakenly overrideds
   // Record::SaveValue and calls their base's classes implementation (this),
   // we could make it here again.  Just ignore it.
   if (saveCount_) {
      return;
   }
   saveCount_++;

   int i = 0;
   for (const auto& field : fields_) {
      Protocol::Record::Entry* entry = msg->AddExtension(Protocol::Record::record_fields);
      //Protocol::Record::Entry* entry = msg->MutableExtension(Protocol::Record::record_fields, i++);
      entry->set_field(field.first);
      entry->set_value(field.second);
   }
}

void Record::LoadValue(const Store& store, const Protocol::Value& msg)
{
   ASSERT(slave_);
   if (fields_.empty()) {
      int c = msg.ExtensionSize(Protocol::Record::record_fields);
      for (int i = 0; i < c ; i++) {
         const Protocol::Record::Entry& entry = msg.GetExtension(Protocol::Record::record_fields, i);
         fields_.push_back(std::make_pair(entry.field(), entry.value()));
      }
      InitializeRecordFields();
   } else {
      ASSERT(msg.ExtensionSize(Protocol::Record::record_fields) == 0);
   }
}

bool Record::FieldIsUnique(std::string name, Object& obj)
{
   dm::ObjectId id = obj.GetObjectId();
   for (const auto& field : fields_) {
      if ((name.size() && name == field.first) || id == field.second) {
         return false;
      }
   }
   return true;
}

void Record::CloneObject(Object* c, CloneMapping& mapping) const
{
   Record& copy = static_cast<Record&>(*c);
   ASSERT(copy.GetObjectType() == GetObjectType());

   mapping.objects[GetObjectId()] = copy.GetObjectId();

   for (unsigned int i = 0; i < fields_.size(); i++) {
      ASSERT(fields_[i].first == copy.fields_[i].first);
      Object* lhs = GetStore().FetchStaticObject(fields_[i].second);
      Object* rhs = copy.GetStore().FetchStaticObject(copy.fields_[i].second);

      ASSERT(lhs && rhs);
      ASSERT(lhs->GetObjectType() == rhs->GetObjectType());
      lhs->CloneObject(rhs, mapping);
   }
}

std::ostream& Record::Log(std::ostream& os, std::string indent) const
{
   os << "record [oid:" << GetObjectId() << "] {" << std::endl;
   
   std::string i2 = indent + std::string("  ");
   for (unsigned int i = 0; i < fields_.size(); i++) {
      const Object* value = GetStore().FetchStaticObject(fields_[i].second);
      os << i2 << '\"' << fields_[i].first << "\" : " << Format<const Object*>(value, i2);
   }
   os << indent << "}";
   return os;
}
