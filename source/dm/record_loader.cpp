#include "radiant.h"
#include "record.h"

using namespace radiant;
using namespace radiant::dm;

void dm::LoadObject(Record& r, Protocol::Value const& msg)
{
   ASSERT(r.GetFields().empty());

   int c = msg.ExtensionSize(Protocol::Record::record_fields);
   for (int i = 0; i < c ; i++) {
      const Protocol::Record::Entry& entry = msg.GetExtension(Protocol::Record::record_fields, i);
      r.AddRecordField(entry.field(), entry.value());
   }
   r.InitializeRecordFields();
}

void dm::SaveObject(Record const& r, Protocol::Value* msg)
{
   for (const auto& field : r.GetFields()) {
      Protocol::Record::Entry* entry = msg->AddExtension(Protocol::Record::record_fields);
      entry->set_field(field.first);
      entry->set_value(field.second);
   }
}

void dm::SaveObjectDelta(Record const& record, Protocol::Value* msg)
{
   // A record gets all it's fields initialized prior to the first save and
   // never changes thereafter.  So there's nothing to do!
}
