#include "pch.h"
#include "om/components/data_store.ridl.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, DataStore const& o)
{
   return (os << "[DataStore]");
}

luabind::object DataStore::GetData() const
{
   return (*data_).GetDataObject();
}

JSONNode DataStore::GetJsonNode() const
{
   return (*data_).GetJsonNode();
}

void DataStore::SetData(luabind::object o)
{
   data_.Modify([o](lua::DataObject& obj) {
      obj.SetDataObject(o);
   });
}

void DataStore::MarkDataChanged()
{
   data_.Modify([](lua::DataObject& obj) {
      obj.MarkDirty();
   });
}
