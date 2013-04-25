#ifndef _RADIANT_RESOURCES_RIG_H
#define _RADIANT_RESOURCES_RIG_H

#include "model.h"
#include "resource.h"
#include <unordered_map>

BEGIN_RADIANT_RESOURCES_NAMESPACE

class Rig : public Resource {
   public:
      Rig(std::string id) : identifier_(id) { }
      
     static ResourceType Type;
     ResourceType GetType() const override { return Resource::RIG; }

      std::string GetResourceIdentifier() const { return identifier_; }
         
      // xxx - don't you think this is a little much??
      typedef std::unordered_map<std::string, std::vector<Model>> ModelMap;

      void Clear();
      void AddModel(std::string bone, const Model &model);

      void ForEachModel(std::function<void (std::string bone, const Model &model)>) const; // xxx: nuke
      const ModelMap &GetModels() const;

   protected:
      std::string       identifier_;
      ModelMap          _models;
};

std::ostream& operator<<(std::ostream& out, const Rig& source);

END_RADIANT_RESOURCES_NAMESPACE


#endif //  _RADIANT_RESOURCES_RIG_H
 