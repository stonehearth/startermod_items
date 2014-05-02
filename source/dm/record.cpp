#include "radiant.h"
#include "record.h"
#include "record_trace.h"
#include "store.h"
#include "protocols/store.pb.h"
#include "dbg_info.h"
#include "dbg_indenter.h"

using namespace ::radiant;
using namespace ::radiant::dm;

Record::Record() : 
   slave_(false),
   registerOffset_(0),
   saveCount_(0),
   constructed_(false)
{
}   

std::shared_ptr<RecordTrace<Record>> Record::TraceChanges(const char* reason, int category) const
{
   return GetStore().TraceRecordChanges(reason, *this, category);
}


TracePtr Record::TraceObjectChanges(const char* reason, int category) const
{
   return GetStore().TraceRecordChanges(reason, *this, category);
}

TracePtr Record::TraceObjectChanges(const char* reason, Tracer* tracer) const
{
   return GetStore().TraceRecordChanges(reason, *this, tracer);
}

void Record::ConstructObject()
{
   constructed_ = true;
   InitializeRecordFields();
}
 
void Record::LoadValue(SerializationType r, Protocol::Value const& msg)
{
   ASSERT(GetFields().empty());

   int c = msg.ExtensionSize(Protocol::Record::record_fields);
   for (int i = 0; i < c ; i++) {
      const Protocol::Record::Entry& entry = msg.GetExtension(Protocol::Record::record_fields, i);
      std::string const& name = entry.field();
      fields_.emplace_back(std::make_pair(entry.field(), entry.value()));
   }
   registerOffset_ = 0;
   InitializeRecordFields();
   ASSERT(registerOffset_ == fields_.size());
}

void Record::SaveValue(SerializationType r, Protocol::Value* msg) const
{
   for (const auto& field : GetFields()) {
      Protocol::Record::Entry* entry = msg->AddExtension(Protocol::Record::record_fields);
      entry->set_field(field.first);
      entry->set_value(field.second);
   }
}

void Record::AddRecordField(std::string const& name, Object& field)
{
   auto& store = GetStore();
   ObjectId id;

   if (constructed_) {
      id = store.GetNextObjectId();
   } else {
      id = fields_[registerOffset_++].second;
   }

   field.SetObjectMetadata(id, store);
   store.RegisterObject(field);

   if (constructed_) {
      ASSERT(FieldIsUnique(name, field));
      fields_.push_back(std::make_pair(name, id));
   }
}

bool Record::FieldIsUnique(std::string const& name, Object& obj)
{
   dm::ObjectId id = obj.GetObjectId();
   for (const auto& field : fields_) {
      if ((name.size() && name == field.first) || id == field.second) {
         return false;
      }
   }
   return true;
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
