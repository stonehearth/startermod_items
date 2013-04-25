#ifndef _RADIANT_RESOURCES_EFFECT_H
#define _RADIANT_RESOURCES_EFFECT_H

#include "data_resource.h"

BEGIN_RADIANT_RESOURCES_NAMESPACE

class Effect : public DataResource {
   public:
      static ResourceType Type;

      Effect(const JSONNode& info) : DataResource(info) { }
      
      ResourceType GetType() const override { return Resource::EFFECT; }
};

typedef std::shared_ptr<Effect>  EffectPtr;

std::ostream& operator<<(std::ostream& out, const Effect& source);

END_RADIANT_RESOURCES_NAMESPACE

#endif //  _RADIANT_RESOURCES_EFFECT_H
 