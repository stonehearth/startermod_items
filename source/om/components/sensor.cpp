#include "radiant.h"
#include "sensor.ridl.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::om;

std::ostream& operator<<(std::ostream& os, const Sensor& o)
{
   os << "[Sensor " << o.GetObjectId() << " name:" << o.GetName() << "]";
   return os;
}

void Sensor::LoadFromJson(json::Node const& node)
{
}

void Sensor::SerializeToJson(json::Node& node) const
{
}

dm::Map<dm::ObjectId, std::weak_ptr<Entity>>& Sensor::GetContainer()
{
   return contents_;
}
