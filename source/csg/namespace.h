#ifndef _RADIANT_CSG_NAMESPACE_H
#define _RADIANT_CSG_NAMESPACE_H

#define BEGIN_RADIANT_CSG_NAMESPACE  namespace radiant { namespace csg {
#define END_RADIANT_CSG_NAMESPACE    } }

BEGIN_RADIANT_CSG_NAMESPACE

class Ray3;

template <class S, int C> class Region;

typedef Region<int, 1>   Region1;
typedef Region<int, 2>   Region2;
typedef Region<float, 2> Region2f;
typedef Region<int, 3>   Region3;
typedef Region<float, 3> Region3f;

DECLARE_SHARED_POINTER_TYPES(Region1);
DECLARE_SHARED_POINTER_TYPES(Region2);
DECLARE_SHARED_POINTER_TYPES(Region2f);
DECLARE_SHARED_POINTER_TYPES(Region3);
DECLARE_SHARED_POINTER_TYPES(Region3f);

template <typename S, int C> class Point;

typedef Point<int, 1>      Point1;
typedef Point<int, 2>      Point2;
typedef Point<int, 3>      Point3;
typedef Point<int, 4>      Point4;
typedef Point<float, 1>    Point1f;
typedef Point<float, 2>    Point2f;
typedef Point<float, 3>    Point3f;
typedef Point<float, 4>    Point4f;

template <typename S, int C> class Cube;

typedef Cube<int, 1> Line1;
typedef Cube<int, 2> Rect2;
typedef Cube<int, 3> Cube3;
typedef Cube<float, 2> Rect2f;
typedef Cube<float, 3> Cube3f;

class Color3;
class Color4;

template <typename S, int C> class RegionTools;

typedef RegionTools<int, 2> RegionTools2;
typedef RegionTools<int, 3> RegionTools3;
typedef RegionTools<float, 2> RegionTools2f;
typedef RegionTools<float, 3> RegionTools3f;

template <typename S, int C> struct PlaneInfo;

typedef PlaneInfo<int, 2>      PlaneInfo2;
typedef PlaneInfo<int, 3>      PlaneInfo3;
typedef PlaneInfo<float, 2>    PlaneInfo2f;
typedef PlaneInfo<float, 3>    PlaneInfo3f;

template <typename S, int C> class EdgeMap;

typedef EdgeMap<int, 2> EdgeMap2;
typedef EdgeMap<int, 3> EdgeMap3;
typedef EdgeMap<float, 2> EdgeMap2f;
typedef EdgeMap<float, 3> EdgeMap3f;

template <typename S, int C> struct EdgeInfo;

typedef EdgeInfo<int, 2> EdgeInfo2;
typedef EdgeInfo<int, 3> EdgeInfo3;
typedef EdgeInfo<float, 2> EdgeInfo2f;
typedef EdgeInfo<float, 3> EdgeInfo3f;


END_RADIANT_CSG_NAMESPACE

#endif //  _RADIANT_CSG_NAMESPACE_H
