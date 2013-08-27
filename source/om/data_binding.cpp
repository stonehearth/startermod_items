#include "pch.h"
#include "data_binding.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& om::operator<<(std::ostream& os, const DataBinding& o)
{
   return (os << "[DataBinding " << o.GetObjectId() << "]");
}

// xxx: move this into a helper or header or somewhere.  EVERYONE needs to
// do this.  actually, we can't we just inherit it in the luabind registration
// stuff???
template <class T>
std::string ToJsonUri(std::weak_ptr<T> o, luabind::object state)
{
   std::ostringstream output;
   std::shared_ptr<T> obj = o.lock();
   if (obj) {
      output << "\"/object/" << obj->GetObjectId() << "\"";
   } else {
      output << "null";
   }
   return output.str();
}

DataBinding::DataBinding() :
   dm::Object(),
   cached_json_valid_(false)
{
}

void DataBinding::SetDataObject(luabind::object data)
{
   data_ = data;
   MarkChanged();
}

luabind::object DataBinding::GetDataObject() const
{
   return data_;
}

void DataBinding::SetModelObject(luabind::object model)
{
   model_ = model;
}

luabind::object DataBinding::GetModelObject() const
{
   return model_;
}

void DataBinding::SaveValue(const dm::Store& store, Protocol::Value* msg) const
{
   // xxx: this isn't going to work for save and load.  we need to serialize something
   // which can then be unserialized (probably via eval!!), which means it will have
   // class names in it, which means it won't be valid json! 
   //
   // this is idea for remoting, though, so let's do it.  SaveValue/LoadValue should
   // probably be renamed to something which indicates they're for remoting (or
   // removed entirely from the object and moved elsewhere!!!)
   std::string json = GetJsonData().write();
   dm::SaveImpl<std::string>::SaveValue(store, msg, json);
}

void DataBinding::LoadValue(const dm::Store& store, const Protocol::Value& msg)
{
   std::string json;
   dm::SaveImpl<std::string>::LoadValue(store, msg, json);
   cached_json_ = libjson::parse(json);
   cached_json_valid_ = true;
}

JSONNode DataBinding::GetJsonData() const
{
   using namespace luabind;

   if (!cached_json_valid_) {
      cached_json_ = JSONNode();
      
      if (data_.is_valid()) {
         object coder = globals(data_.interpreter())["radiant"]["json"];
         std::string json = call_function<std::string>(coder["encode"], data_);

         if (!libjson::is_valid(json)) {
            // xxx: actually, throw an exception
            return JSONNode();
         }
         cached_json_ = libjson::parse(json);
      }
      // xxx: skip this until we have legitimate change tracking
      // cached_json_valid_ = true; 
   }
   return cached_json_;
}
