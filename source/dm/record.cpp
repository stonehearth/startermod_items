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
   saveCount_(0)
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

void Record::Initialize(Store& s, ObjectId id)
{
   SetObjectMetadata(id, s);
   InitializeRecord();
}
 
void Record::InitializeSlave(Store& s, ObjectId id)
{
   slave_ = true;
   SetObjectMetadata(id, s);
   // A LoadValue's coming... don't worry
}

void Record::LoadValue(Protocol::Value const& msg)
{
   ASSERT(slave_);
   ASSERT(GetFields().empty());

   int c = msg.ExtensionSize(Protocol::Record::record_fields);
   for (int i = 0; i < c ; i++) {
      const Protocol::Record::Entry& entry = msg.GetExtension(Protocol::Record::record_fields, i);
      AddRecordField(entry.field(), entry.value());
   }
   InitializeRecord();
}

void Record::SaveValue(Protocol::Value* msg) const
{
   for (const auto& field : GetFields()) {
      Protocol::Record::Entry* entry = msg->AddExtension(Protocol::Record::record_fields);
      entry->set_field(field.first);
      entry->set_value(field.second);
   }
}

void Record::AddRecordField(std::string name, Object& field)
{
   if (!slave_) {
      auto& store = GetStore();
      field.Initialize(store, store.GetNextObjectId());

      ASSERT(FieldIsUnique(name, field));
      AddRecordField(name, field.GetObjectId());
   } else {
      field.Initialize(GetStore(), fields_[registerOffset_++].second);
   }
}

void Record::AddRecordField(std::string name, ObjectId id)
{
   ASSERT(id);
   ASSERT(GetObjectId() < id);

   fields_.push_back(std::make_pair(name, id));
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

void Record::InitializeRecord()
{
   InitializeRecordFields();
   if (!slave_) {
      ConstructObject();
   }
   MarkChanged();
   GetStore().SignalRegistered(this);
}
