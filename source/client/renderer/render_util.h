#ifndef _RADIANT_CLIENT_RENDER_UTIL_H
#define _RADIANT_CLIENT_RENDER_UTIL_H

#include <map>
#include "om/om.h"
#include "dm/dm.h"
#include "csg/color.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

template <typename BoxedRegion>
void CreateRegionDebugShape(om::EntityRef entityRef,
                            H3DNodeUnique& shape,
                            std::shared_ptr<om::DeepRegionGuard<dm::Boxed<std::shared_ptr<BoxedRegion>>>> trace,
                            csg::Color4 const& color);

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_RENDER_UTIL_H
