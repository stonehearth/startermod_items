#include "pch.h"
#include "destination.h"
#include "om/entity.h"

using namespace ::radiant;
using namespace ::radiant::simulation;

Destination::Destination(om::EntityRef e) :
   entity_(e),
   enabled_(true)
{
   auto entity = e.lock();
   entityId_ = entity ? entity->GetEntityId() : 0;
}
