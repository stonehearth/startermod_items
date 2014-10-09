#ifndef _RADIANT_CSG_ITERATORS_H
#define _RADIANT_CSG_ITERATORS_H

#include "region.h"

BEGIN_RADIANT_CSG_NAMESPACE

class Cube3PointIterator
{
public:
   Cube3PointIterator();
   Cube3PointIterator(Cube3 const& r);
   
   Point3 const& operator*() const;
   void operator++();
   bool operator!=(Cube3PointIterator const& rhs) const;
   
   bool IsEnd() const;

private:
   bool                 _finished;
   Point3               _current;
   Cube3 const*         _cube;
};

// ------------------------------------------------------------------------------------------

class Rect2PointIterator
{
public:
   Rect2PointIterator();
   Rect2PointIterator(Rect2 const& r);
   
   Point2 const& operator*() const;
   void operator++();
   bool operator!=(Rect2PointIterator const& rhs) const;
   
   bool IsEnd() const;

private:
   bool                 _finished;
   Point2               _current;
   Rect2 const*         _rect;
};

// ------------------------------------------------------------------------------------------

class Rect2fPointIterator
{
public:
   Rect2fPointIterator();
   Rect2fPointIterator(Rect2f const& r);
   
   Point2 const& operator*() const;
   void operator++();
   bool operator!=(Rect2fPointIterator const& rhs) const;

private:
   Rect2                _rect;
   Rect2PointIterator   _iterator;
};

// ------------------------------------------------------------------------------------------

class Cube3fPointIterator
{
public:
   Cube3fPointIterator();
   Cube3fPointIterator(Cube3f const& r);
   
   Point3 const& operator*() const;
   void operator++();
   bool operator!=(Cube3fPointIterator const& rhs) const;

private:
   Cube3                _cube;
   Cube3PointIterator   _iterator;
};

// ------------------------------------------------------------------------------------------

class Region3PointIterator
{
public:
   Region3PointIterator();
   Region3PointIterator(Region3 const& r);
   
   Point3 const& operator*() const;
   void operator++();
   bool operator!=(Region3PointIterator const& rhs) const;

private:
   bool CheckForEnd();

private:
   bool                 _finished;
   uint                 _cube;
   Region3 const&       _region;
   Cube3PointIterator   _cubeIterator;
};

// ------------------------------------------------------------------------------------------

class Region2PointIterator
{
public:
   Region2PointIterator();
   Region2PointIterator(Region2 const& r);
   
   Point2 const& operator*() const;
   void operator++();
   bool operator!=(Region2PointIterator const& rhs) const;

private:
   bool CheckForEnd();

private:
   bool                 _finished;
   uint                 _rect;
   Region2 const&       _region;
   Rect2PointIterator   _rectIterator;
};

// ------------------------------------------------------------------------------------------

class Region3fPointIterator
{
public:
   Region3fPointIterator();
   Region3fPointIterator(Region3f const& r);
   
   Point3 const& operator*() const;
   void operator++();
   bool operator!=(Region3fPointIterator const& rhs) const;

private:
   bool IsVisited() const;

private:
   bool                 _finished;
   uint                 _cube;
   Region3f const&      _region;
   Cube3                _cubeAsInt;
   Point3               _current;
};

// ------------------------------------------------------------------------------------------

class Region2fPointIterator
{
public:
   Region2fPointIterator();
   Region2fPointIterator(Region2f const& r);
   
   Point2 const& operator*() const;
   void operator++();
   bool operator!=(Region2fPointIterator const& rhs) const;

private:
   bool IsVisited() const;

private:
   bool                 _finished;
   uint                 _rect;
   Region2f const&      _region;
   Rect2                _rectAsInt;
   Point2               _current;
};

// ------------------------------------------------------------------------------------------

#define DECLARE_POINT_ITERATOR(T) \
class T ## PointRange \
{ \
public: \
   T ## PointRange(T const& container) : _container(container) { } \
 \
   T ## PointIterator begin() const { return T ## PointIterator(_container); } \
   T ## PointIterator end() const { return T ## PointIterator(); } \
 \
private: \
   T const& _container; \
}; \
 \
static inline T ## PointRange EachPoint(T const& cube) \
{ \
   return T ## PointRange(cube); \
} \

DECLARE_POINT_ITERATOR(Cube3)
DECLARE_POINT_ITERATOR(Cube3f)
DECLARE_POINT_ITERATOR(Rect2)
DECLARE_POINT_ITERATOR(Rect2f)
DECLARE_POINT_ITERATOR(Region2)
DECLARE_POINT_ITERATOR(Region2f)
DECLARE_POINT_ITERATOR(Region3)
DECLARE_POINT_ITERATOR(Region3f)

#undef DECLARE_CUBE_ITERATOR
#undef DECLARE_POINT_ITERATOR

END_RADIANT_CSG_NAMESPACE

#endif // _RADIANT_CSG_ITERATORS_H
