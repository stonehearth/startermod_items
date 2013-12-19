#include "radiant.h"
#include "nav_grid.h"
#include "tracker.h"

using namespace radiant;
using namespace radiant::phys;

Tracker::Tracker(NavGrid& ng, om::EntityPtr entity) :
   ng_(ng),
   entity_(entity)
{
}

void Tracer::MarkChanged()
{
   ng_.MarkChanged(shared_from_this());
}

NavGrid& Tracker::GetNavGrid() const
{
   return ng_;
}

om::EntityPtr Tracker::GetEntity() const
{
   return entity_.lock();
}
