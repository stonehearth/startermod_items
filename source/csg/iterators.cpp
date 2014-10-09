#include "pch.h"
#include "iterators.h"
#include "util.h"

using namespace ::radiant;
using namespace ::radiant::csg;

Cube3PointIterator::Cube3PointIterator() :
   _cube(&csg::Cube3::zero),
   _finished(true)
{
}

// XXX: Is there a way to ENFORCE that this must be kept alive for
// the duration of the loop?  Hmmmm.  I'd hate to have to copy it...
Cube3PointIterator::Cube3PointIterator(csg::Cube3 const& cube) :
   _cube(&cube),
   _current(cube.min)
{
   _finished = cube.GetArea() == 0;
}

Point3 const& Cube3PointIterator::operator*() const
{
   ASSERT(!_finished);

   return _current;
}

void Cube3PointIterator::operator++()
{
   ASSERT(!_finished);

   _current.z++;
   if (_current.z < _cube->max.z) {
      return;
   }

   _current.z = _cube->min.z;
   _current.x++;
   if (_current.x < _cube->max.x) {
      return;
   }

   _current.z = _cube->min.z;
   _current.x = _cube->min.x;
   _current.y++;
   if (_current.y < _cube->max.y) {
      return;
   }
 
   _finished = true;
}

bool Cube3PointIterator::operator!=(Cube3PointIterator const& rhs) const
{
   return _finished != rhs._finished;
}

bool Cube3PointIterator::IsEnd() const
{
   return _finished;
}

// ------------------------------------------------------------------------------------ 

Cube3fPointIterator::Cube3fPointIterator() :
   _iterator(Cube3PointIterator())
{
}

Cube3fPointIterator::Cube3fPointIterator(csg::Cube3f const& cube) :
   _cube(ToInt(cube)),
   _iterator(Cube3PointIterator(_cube))
{
}

Point3 const& Cube3fPointIterator::operator*() const
{
   return *_iterator;
}

void Cube3fPointIterator::operator++()
{
   ++_iterator;
}

bool Cube3fPointIterator::operator!=(Cube3fPointIterator const& rhs) const
{
   return _iterator != rhs._iterator;
}

// ------------------------------------------------------------------------------------ 

Rect2PointIterator::Rect2PointIterator() :
   _rect(&csg::Rect2::zero),
   _finished(true)
{
}

Rect2PointIterator::Rect2PointIterator(csg::Rect2 const& rect) :
   _rect(&rect),
   _current(rect.min)
{
   _finished = rect.min == rect.max;
}

Point2 const& Rect2PointIterator::operator*() const
{
   ASSERT(!_finished);

   return _current;
}

void Rect2PointIterator::operator++()
{
   ASSERT(!_finished);

   _current.x++;
   if (_current.x < _rect->max.x) {
      return;
   }

   _current.x = _rect->min.x;
   _current.y++;
   if (_current.y < _rect->max.y) {
      return;
   }
 
   _finished = true;
}

bool Rect2PointIterator::operator!=(Rect2PointIterator const& rhs) const
{
   return _finished != rhs._finished;
}

bool Rect2PointIterator::IsEnd() const
{
   return _finished;
}

// ------------------------------------------------------------------------------------ 

Rect2fPointIterator::Rect2fPointIterator() :
   _iterator(Rect2PointIterator())
{
}

Rect2fPointIterator::Rect2fPointIterator(csg::Rect2f const& rect) :
   _rect(ToInt(rect)),
   _iterator(Rect2PointIterator(_rect))
{
}

Point2 const& Rect2fPointIterator::operator*() const
{
   return *_iterator;
}

void Rect2fPointIterator::operator++()
{
   ++_iterator;
}

bool Rect2fPointIterator::operator!=(Rect2fPointIterator const& rhs) const
{
   return _iterator != rhs._iterator;
}

// ------------------------------------------------------------------------------------ 

Region3PointIterator::Region3PointIterator() :
   _region(csg::Region3::zero),
   _finished(true)
{
}

Region3PointIterator::Region3PointIterator(csg::Region3 const& region) :
   _cube(0),
   _region(region)
{
   if (region.IsEmpty()) {
      _finished = true;
      return;
   }
   _cubeIterator = Cube3PointIterator(region[0]);
   if (CheckForEnd()) {
    _finished = true;
   }
}

Point3 const& Region3PointIterator::operator*() const
{
   ASSERT(!_finished);

   return *_cubeIterator;
}

void Region3PointIterator::operator++()
{
   ASSERT(!_finished);
   ASSERT(_cube < _region.GetContents().size());

   ++_cubeIterator;
   while (!CheckForEnd() && _cubeIterator.IsEnd()) {
      _cubeIterator = Cube3PointIterator(_region[++_cube]);
   }
}

bool Region3PointIterator::operator!=(Region3PointIterator const& rhs) const
{
   return _finished != rhs._finished;
}

bool Region3PointIterator::CheckForEnd()
{
   _finished = (_cubeIterator.IsEnd() && _cube >= _region.GetContents().size() - 1);
   return _finished;
}

// ------------------------------------------------------------------------------------ 

Region3fPointIterator::Region3fPointIterator() :
   _region(csg::Region3f::zero),
   _finished(true),
   _cube(0)
{
}

Region3fPointIterator::Region3fPointIterator(csg::Region3f const& region) :
   _region(region),
   _finished(false),
   _cube(0)
{
   if (region.IsEmpty()) {
      _finished = true;
      return;
   }
   _cubeAsInt = csg::ToInt(_region[_cube]);
   _current = _cubeAsInt.min;
}

Point3 const& Region3fPointIterator::operator*() const
{
   ASSERT(!_finished);

   return _current;
}

void Region3fPointIterator::operator++()
{
   ASSERT(!_finished);
   ASSERT(_cube < _region.GetContents().size());

   do {
      _current.z++;
      if (_current.z < _cubeAsInt.max.z && !IsVisited()) {
         return;
      }

      _current.z = _cubeAsInt.min.z;
      _current.x++;
      if (_current.x < _cubeAsInt.max.x && !IsVisited()) {
         return;
      }

      _current.z = _cubeAsInt.min.z;
      _current.x = _cubeAsInt.min.x;
      _current.y++;
      if (_current.y < _cubeAsInt.max.y && !IsVisited()) {
         return;
      }

      _cube++;
      if (_cube >= _region.GetContents().size()) {
         _finished = true;
         return;
      }
      _cubeAsInt = csg::ToInt(_region[_cube]);
      _current = _cubeAsInt.min;
      // The first point in the next cube might also be visited, so spin around
      // again!

   } while (IsVisited());
}

bool Region3fPointIterator::operator!=(Region3fPointIterator const& rhs) const
{
   return _finished != rhs._finished;
}

bool Region3fPointIterator::IsVisited() const
{
   // This works great for very small regions.  Not so much for large ones.  Revisit
   // if the profiler says we should care.
   for (uint i = 0; i < _cube; i++) {
      if (ToInt(_region[i]).Contains(_current)) {
         return true;
      }
   }
   return false;
}

// ------------------------------------------------------------------------------------ 

Region2PointIterator::Region2PointIterator() :
   _region(csg::Region2::zero),
   _finished(true)
{
}

Region2PointIterator::Region2PointIterator(csg::Region2 const& region) :
   _rect(0),
   _region(region)
{
   if (region.IsEmpty()) {
      _finished = true;
      return;
   }
   _rectIterator = Rect2PointIterator(region[0]);
   if (CheckForEnd()) {
    _finished = true;
   }
}

Point2 const& Region2PointIterator::operator*() const
{
   ASSERT(!_finished);

   return *_rectIterator;
}

void Region2PointIterator::operator++()
{
   ASSERT(!_finished);
   ASSERT(_rect < _region.GetContents().size());

   ++_rectIterator;
   while (!CheckForEnd() && _rectIterator.IsEnd()) {
      _rectIterator = Rect2PointIterator(_region[++_rect]);
   }
}

bool Region2PointIterator::operator!=(Region2PointIterator const& rhs) const
{
   return _finished != rhs._finished;
}

bool Region2PointIterator::CheckForEnd()
{
   _finished = (_rectIterator.IsEnd() && _rect >= _region.GetContents().size() - 1);
   return _finished;
}

// ------------------------------------------------------------------------------------ 

Region2fPointIterator::Region2fPointIterator() :
   _region(csg::Region2f::zero),
   _finished(true),
   _rect(0)
{
}

Region2fPointIterator::Region2fPointIterator(csg::Region2f const& region) :
   _region(region),
   _finished(false),
   _rect(0)
{
   if (region.IsEmpty()) {
      _finished = true;
      return;
   }
   _rectAsInt = csg::ToInt(_region[_rect]);
   _current = _rectAsInt.min;
}

Point2 const& Region2fPointIterator::operator*() const
{
   ASSERT(!_finished);

   return _current;
}

void Region2fPointIterator::operator++()
{
   ASSERT(!_finished);
   ASSERT(_rect < _region.GetContents().size());

   do {
      _current.x++;
      if (_current.x < _rectAsInt.max.x && !IsVisited()) {
         return;
      }

      _current.x = _rectAsInt.min.x;
      _current.y++;
      if (_current.y < _rectAsInt.max.y && !IsVisited()) {
         return;
      }

      _rect++;
      if (_rect >= _region.GetContents().size()) {
         _finished = true;
         return;
      }
      _rectAsInt = csg::ToInt(_region[_rect]);
      _current = _rectAsInt.min;
      // The first point in the next rect might also be visited, so spin around
      // again!

   } while (IsVisited());
}

bool Region2fPointIterator::operator!=(Region2fPointIterator const& rhs) const
{
   return _finished != rhs._finished;
}

bool Region2fPointIterator::IsVisited() const
{
   // This works great for very small regions.  Not so much for large ones.  Revisit
   // if the profiler says we should care.
   for (uint i = 0; i < _rect; i++) {
      if (ToInt(_region[i]).Contains(_current)) {
         return true;
      }
   }
   return false;
}
