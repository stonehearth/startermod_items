#include "pch.h"
#include "data_blob.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& om::operator<<(std::ostream& os, const DataBlob& o)
{
   return (os << "[DataBlob " << o.GetObjectId() << "]");
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

DataBlob::DataBlob() :
   dm::Object(),
   cached_json_valid_(false)
{
}

void DataBlob::SetLuaObject(luabind::object obj)
{
   obj_ = obj;
}

void DataBlob::SaveValue(const dm::Store& store, Protocol::Value* msg) const
{
   // xxx: this isn't going to work for save and load.  we need to serialize something
   // which can then be unserialized (probably via eval!!), which means it will have
   // class names in it, which means it won't be valid json! 
   //
   // this is idea for remoting, though, so let's do it.  SaveValue/LoadValue should
   // probably be renamed to something which indicates they're for remoting (or
   // removed entirely from the object and moved elsewhere!!!)
   std::string json = ToJson().write();
   dm::SaveImpl<std::string>::SaveValue(store, msg, json);
}

void DataBlob::LoadValue(const dm::Store& store, const Protocol::Value& msg)
{
   std::string json;
   dm::SaveImpl<std::string>::LoadValue(store, msg, json);
   cached_json_ = libjson::parse(json);
   cached_json_valid_ = true;
}

JSONNode DataBlob::ToJson() const
{
   using namespace luabind;
   if (!cached_json_valid_) {
      cached_json_ = JSONNode();
      
      object result ;
      if (type(obj_) == LUA_TFUNCTION) {
         result = call_function<object>(obj_);
      } else if (type(obj_) == LUA_TTABLE) {
         object tojson = obj_["__tojson"];
         if (luabind::type(tojson) == LUA_TFUNCTION) {
            result = call_function<object>(tojson, obj_);
         }
      } else {
         // xxx: actually, throw an exception
         return JSONNode();
      }

      int t = type(result);
      if (t != LUA_TSTRING) {
         // xxx: actually, throw an exception
         return JSONNode();
      }
      std::string json = object_cast<std::string>(result);
      if (!libjson::is_valid(json)) {
         // xxx: actually, throw an exception
         return JSONNode();
      }
      cached_json_ = libjson::parse(json);
      // xxx: skip this until we have legitimate change tracking
      // cached_json_valid_ = true; 
   }
   return cached_json_;
}
