#ifndef _RADIANT_RESOURCES_EFFECT_H
#define _RADIANT_RESOURCES_EFFECT_H

#include "resource.h"
#include "libjson.h"

BEGIN_RADIANT_RESOURCES_NAMESPACE

class Effect : public Resource {
   public:
      static ResourceType Type;

      Effect(const JSONNode& info) : info_(info) { }
      
      ResourceType GetType() const override { return Resource::EFFECT; }
      const JSONNode& GetJson() const { return info_; }
      std::string GetJsonString() const { return info_.write(); }

   protected:
      JSONNode          info_;
};

typedef std::shared_ptr<Effect>  EffectPtr;

std::ostream& operator<<(std::ostream& out, const Effect& source);

END_RADIANT_RESOURCES_NAMESPACE

#endif //  _RADIANT_RESOURCES_EFFECT_H
 