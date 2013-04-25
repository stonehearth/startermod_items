#ifndef _RADIANT_RESOURCES_ACTION_H
#define _RADIANT_RESOURCES_ACTION_H

#include "resource.h"
#include "libjson.h"
#include <unordered_map>

BEGIN_RADIANT_RESOURCES_NAMESPACE

class Action : public Resource {
   public:
      static ResourceType Type;
      Action(std::string id, const JSONNode& info) : identifier_(id), info_(info) { }
      
      ResourceType GetType() const override { return Resource::ACTION; }
      std::string GetResourceIdentifier() const { return identifier_; }

      std::string GetCommand() const { return info_["command"].as_string(); }
      std::string GetIcon() const { return info_["icon"].as_string(); }
      std::string GetCursor() const { return FetchString("cursor"); }
      std::string GetToolTip() const { return info_["tooltip"].as_string(); }
      const JSONNode& GetArgs() const { return info_["args"]; }

   private:
      std::string FetchString(std::string key) const {
         auto i = info_.find(key);
         if (i != info_.end() && i->type() == JSON_STRING) {
            return i->as_string();
         }
         return "";
      }

   protected:
      std::string       identifier_;
      JSONNode          info_;
};

END_RADIANT_RESOURCES_NAMESPACE


#endif //  _RADIANT_RESOURCES_ACTION_H
 