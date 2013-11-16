#include "radiant.h"
#include "record.h"
#include "store.h"
#include "dbg_indenter.h"

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

core::Guard Record::TraceRecordField(std::string name, const char* reason, std::function<void()> fn)
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
   return core::Guard();
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

// xxx: this will end up remoting the unit info record a 2nd time whenever something
// in the unit info changes.  arg!  what we really want to say is "hey record, install
// this trace whenever *you* change or any of *your fields* change).
core::Guard Record::TraceObjectChanges(const char* reason, std::function<void()> fn) const
{
   core::Guard guard;
   for (auto const& field : fields_) {
      guard += GetStore().FetchStaticObject(field.second)->TraceObjectChanges(reason, fn);
   }
   return guard;
}


void Record::GetDbgInfo(DbgInfo &info) const
{
   if (WriteDbgInfoHeader(info)) {
      info.os << " {" << std::endl;
      {
         Indenter indent(info.os);
         auto i = fields_.begin(), end = fields_.end();
         while (i != end) {
            info.os << '"' << i->first << '"' << " : ";
            GetStore().FetchStaticObject(i->second)->GetDbgInfo(info);
            if (++i != end) {
               info.os << ",";
            }
            info.os << std::endl;
         }
      }
      info.os << "}";
   }
}
