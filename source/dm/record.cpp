#include "radiant.h"
#include "record.h"
#include "record_trace.h"
#include "store.h"
#include "dbg_indenter.h"

using namespace ::radiant;
using namespace ::radiant::dm;

DEFINE_STATIC_DISPATCH(Record)

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
