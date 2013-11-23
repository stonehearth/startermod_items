#include "csg/color.h"
#include "csg/sphere.h"
#include "csg/transform.h"
#include "csg/region.h"

IMPLEMENT_DM_EXTENSION(csg::Color3, Protocol::color)
IMPLEMENT_DM_EXTENSION(csg::Color4, Protocol::color)
IMPLEMENT_DM_EXTENSION(csg::Cube3, Protocol::cube3i)
IMPLEMENT_DM_EXTENSION(csg::Cube3f, Protocol::cube3f)
IMPLEMENT_DM_EXTENSION(csg::Point2, Protocol::point2i)
IMPLEMENT_DM_EXTENSION(csg::Point3, Protocol::point3i)
IMPLEMENT_DM_EXTENSION(csg::Point3f, Protocol::point3f)
IMPLEMENT_DM_EXTENSION(csg::Region3, Protocol::region3i)
IMPLEMENT_DM_EXTENSION(csg::Region2, Protocol::region2i)
IMPLEMENT_DM_EXTENSION(csg::Sphere, Protocol::sphere3f)
IMPLEMENT_DM_EXTENSION(csg::Transform, Protocol::transform)

BOXED(csg::Region3)
BOXED(csg::Region2)
BOXED(csg::Point3)
BOXED(csg::Point3f)
BOXED(csg::Cube3)
BOXED(csg::Cube3f)
BOXED(csg::Transform)
