#include "radiant.h"
#include "rig.h"

namespace protocol = ::radiant::protocol;

using namespace ::radiant;
using namespace ::radiant::resources;
using ::radiant::uint;

Resource::ResourceType Rig::Type = RIG;

void Rig::Clear()
{
   _models.clear();
}

void Rig::AddModel(std::string bone, const Model &model)
{
   _models[bone].push_back(model);
}

void Rig::ForEachModel(std::function<void (std::string bone, const Model &model)> fn) const
{
   for_each(_models.begin(), _models.end(), [&](const ModelMap::value_type &e) {
      const auto &models = e.second;
      for (uint i = 0; i < models.size(); i ++) {
         fn(e.first, models[i]);
      }
   });
}

const Rig::ModelMap &Rig::GetModels() const
{
   return _models;
}

std::ostream& ::radiant::resources::operator<<(std::ostream& out, const Rig& source) {
   return (out << "[Rig " << source.GetResourceIdentifier() << "]");
}
