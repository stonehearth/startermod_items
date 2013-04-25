#include "radiant.h"
#include "effect.h"

using namespace ::radiant;
using namespace ::radiant::resources;

Resource::ResourceType Effect::Type = EFFECT;

std::ostream& ::radiant::resources::operator<<(std::ostream& out, const Effect& source) {
   return (out << "[EffectResource]");
}
