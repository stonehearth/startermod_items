#include "radiant.h"
#include "region2d.h"
#include "point2.h"
#include "ipoint3.h"

using namespace ::radiant;
using namespace ::radiant::math2d;

namespace protocol = ::radiant::protocol;

Region2d::Region2d() :
   space_(UNDEFINED_COORDINATE_SYSTEM)
{
}

Region2d::Region2d(CoordinateSystem cs) :
   space_(cs)
{
}

Region2d::Region2d(const Rect2d &r) :
   rects_(&r, &r + 1),
   space_(r.GetCoordinateSystem())
{
}

Region2d::Region2d(const Region2d &other) :
   rects_(other.rects_),
   space_(other.space_)
{
}

Region2d::Region2d(const Region2d &&other) :
   rects_(std::move(other.rects_)),
   space_(other.GetCoordinateSystem())
{
}

std::vector<Rect2d>::const_iterator Region2d::begin() const
{
   return rects_.begin();
}

std::vector<Rect2d>::const_iterator Region2d::end() const
{
   return rects_.end();
}

void Region2d::SaveValue(protocol::region2 *msg) const
{
   for (const Rect2d &r : rects_) {
      r.Encode(msg->add_rects());
   }
   msg->set_cs(space_);
}

void Region2d::Encode(protocol::shapelist *msg, const math3d::ipoint3& offset, const math3d::color4& c) const
{
   for (auto& rect : rects_) {
      rect.Encode(msg->add_quad(), offset, c);
   }
}

void Region2d::LoadValue(const protocol::region2 &msg)
{
   rects_.clear();
   for (int i = 0; i < msg.rects_size(); i++) {
      rects_.push_back(Rect2d(msg.rects(i)));
   }
   space_ = (CoordinateSystem)msg.cs();
}


bool Region2d::IsEmpty() const
{
   return rects_.empty();
}

bool Region2d::Contains(int x, int y) const
{
   auto cs = GetCoordinateSystem();
   for (const Rect2d &r : rects_) {
      if (r.Contains(x, y, cs)) {
         return true;
      }
   }
   return false;
}

bool Region2d::Contains(const ipoint2 &pt) const
{
   ASSERT(pt.GetCoordinateSystem() == GetCoordinateSystem());
   return Contains(pt.x, pt.y);
}

bool Region2d::Intersects(const Rect2d &other) const
{
   ASSERT(other.GetCoordinateSystem() == space_);

   for (const Rect2d &r : rects_) {
      if (r.Intersects(other)) {
         return true;
      }
   }
   return false;
}

bool Region2d::Intersects(const Region2d &other) const
{
   ASSERT(other.GetCoordinateSystem() == space_);

   for (const Rect2d &r : rects_) {
      if (other.Intersects(r)) {
         return true;
      }
   }
   return false;
}

unsigned int Region2d::GetSize() const
{
   return rects_.size();
}

Rect2d Region2d::GetBoundingBox() const
{
   Rect2d box(GetCoordinateSystem());

   if (rects_.empty()) {
      box.SetZero();
   } else {
      box = rects_[0];
      for (auto i = rects_.begin() + 1; i != rects_.end(); i++) {
         box = box.Union(*i);
      }
   }
   return box;
}


const Rect2d &Region2d::GetRect(unsigned int i) const
{
   return rects_[i];
}

// Jesus, this is expensive!
bool Region2d::operator==(const Region2d& other) const
{
   return &other == this || ((*this - other).IsEmpty() && (other - *this).IsEmpty());
}

bool Region2d::operator!=(const Region2d& other) const
{
   return &other != this && ((*this - other).IsEmpty() || (other - *this).IsEmpty());
}

Region2d& Region2d::operator=(Region2d&& other)
{
   rects_ = std::move(other.rects_);
   space_ = other.space_;
   return *this;
}

Region2d& Region2d::operator-=(const Region2d &other)
{
   ASSERT(other.GetCoordinateSystem() == space_);

   for (const Rect2d &r : other) {
      SubtractRect(r);
   }
   Optimize();
   return *this;
}


Region2d Region2d::operator-(const Rect2d &r) const
{
   ASSERT(r.GetCoordinateSystem() == space_);

   Region2d result(*this);
   result -= r;
   return result;
}

Region2d Region2d::operator-(const Region2d &other) const
{
   ASSERT(other.GetCoordinateSystem() == space_);

   Region2d result(*this);
   result -= other;
   return result;
}

Region2d& Region2d::operator-=(const Rect2d &other)
{
   SubtractRect(other);
   Optimize();
   return *this;
}

Region2d& Region2d::operator-=(const Point2d &other)
{
   return (*this) -= Rect2d(other.x, other.y, other.x + 1, other.y + 1, other.GetCoordinateSystem());
}


void Region2d::SubtractRect(const Rect2d& other)
{
   ASSERT(other.GetCoordinateSystem() == space_);

   unsigned int i = 0;
   unsigned int size = rects_.size();
   std::vector<Rect2d> added;

   while (i < size) {
      Region2d replacement = rects_[i] - other;
      if (replacement.IsEmpty()) {
         rects_[i] = rects_[size - 1];
         size--;
      } else {
         rects_[i] = replacement.GetRect(0);
         for (unsigned int j = 1; j < replacement.GetSize(); j++) {
            added.push_back(replacement.GetRect(j));
         }
         i++;
      }
   }
   rects_.resize(size);
   rects_.insert(rects_.end(), added.begin(), added.end());
}


Region2d& Region2d::operator+=(const Rect2d &other)
{
   AddRect(other);
   Optimize();

   return *this;
}

Region2d& Region2d::operator+=(const Point2d &other)
{
   return (*this) += Rect2d(other.x, other.y, other.x + 1, other.y + 1, other.GetCoordinateSystem());
}

void Region2d::AddRect(const Rect2d& other)
{
   ASSERT(other.GetCoordinateSystem() == space_);

   Region2d unique(other);
   
   if (other.GetArea() > 0) {
      // xxx: could do a pre-pass here to remove rects from _rects which
      // are completely inside other.

      for (Rect2d &r : rects_) {
         unique -= r;
      }
      rects_.insert(rects_.end(), unique.rects_.begin(), unique.rects_.end());
   }
}

Region2d Region2d::Grow(int amount) const
{
   Region2d result(space_);

   for (const Rect2d &r : rects_) {
      result.AddRect(r.Grow(amount));
   }
   result.Optimize();
   return result;
}

Region2d Region2d::GrowCardinal(int amount) const
{
   Region2d result(space_);

   for (const Rect2d &r : rects_) {
      Point2d p0 = r.GetP0();
      Point2d p1 = r.GetP1();

      result.AddRect(Rect2d(p0.x, p0.y - 1, p1.x, p1.y + 1, space_));
      result.AddRect(Rect2d(p0.x - 1, p0.y, p0.x, p1.y, space_));
      result.AddRect(Rect2d(p1.x, p0.y, p1.x + 1, p1.y, space_));
   }
   result.Optimize();

   return result;
}

Region2d Region2d::Inset(int amount) const
{
   Region2d result(space_);

   ASSERT(amount == 1); // Only implemented for borders of 1 so far.

   for (const Rect2d &r : rects_) {
      result.AddRect(r.Inset(amount));

      r.IterateBorderPoints([&](Point2d pt) {
         if (!PointOnBorder(pt)) {
            result.AddRect(Rect2d(pt.x, pt.y, pt.x+1, pt.y+1, space_));
         }
      });
   }
   result.Optimize();

   return result;
}

Region2d Region2d::Offset(const Point2d &amount) const
{
   ASSERT(space_ == amount.GetCoordinateSystem());

   Region2d result(space_);

   for (const Rect2d &r : rects_) {
      result.AddRect(r + amount);
   }
   result.Optimize();

   return result;
}

Region2d Region2d::ProjectOnto(CoordinateSystem cs, int width) const
{
   ASSERT(cs != space_);
   Region2d result(cs);

   for (const Rect2d &r : rects_) {
      result.AddRect(r.ProjectOnto(cs, width));
   }
   result.Optimize();
   return result;
}

Region2d Region2d::GetVerticalSlice(int y) const
{
   ASSERT(GetCoordinateSystem() != math2d::XZ_PLANE);

   Region2d result(math2d::XZ_PLANE);

   for (const Rect2d &r : rects_) {
      result.AddRect(r.GetVerticalSlice(y));
   }
   result.Optimize();
   return result;
}

bool Region2d::PointOnBorder(const Point2d &pt) const
{
   CoordinateSystem cs = pt.GetCoordinateSystem();
   ASSERT(space_ == cs);

   return !Contains(pt.x - 1, pt.y - 1) ||
          !Contains(pt.x, pt.y - 1) ||
          !Contains(pt.x + 1, pt.y - 1) ||

          !Contains(pt.x - 1, pt.y) ||
          !Contains(pt.x + 1, pt.y) ||

          !Contains(pt.x - 1, pt.y + 1) ||
          !Contains(pt.x, pt.y + 1) ||
          !Contains(pt.x + 1, pt.y + 1);
}

void Region2d::Clear()
{
   rects_.clear();
}

void Region2d::SetCoordinateSystem(CoordinateSystem cs)
{
   ASSERT(rects_.empty());
   space_ = cs;
}

// TODO: Write some unit tests for this
std::vector<Line2d> Region2d::GetPerimeter() const
{
   std::vector<Line2d> result;
   CoordinateSystem cs = GetCoordinateSystem();

   // Add all 4 lines around each and every rect.
   for (auto& r : rects_) {
      result.push_back(Line2d(r.p0.x, r.p0.y, r.p1.x, r.p0.y, cs));
      result.push_back(Line2d(r.p1.x, r.p0.y, r.p1.x, r.p1.y, cs));
      result.push_back(Line2d(r.p0.x, r.p1.y, r.p1.x, r.p1.y, cs));
      result.push_back(Line2d(r.p0.x, r.p0.y, r.p0.x, r.p1.y, cs));
   }

   // Remove interior lines.  The interior ones are ones where either
   // endpoint is on the other line.
   int c = result.size();
   for (int i = 0; i < c; i++) {
      for (int j = i + 1; j < c; j++) {
         result[i].RemoveOverlap(result[j]);
      }
   }

   // Remove zero length lines
   int i = 0; 
   while (i < c) {
      if (result[i].LengthIsZero()) {
         result[i] = result[--c];
      } else {
         i++;
      }
   }
   result.resize(c);

   return result;
}

void Region2d::Optimize()
{
   bool changed;
   int c = rects_.size();
   //Region2d tester = *this;

   do {
      changed = false;
      for (int i = 0; i < c; ) {
         if (rects_[i].GetArea() == 0) {
            rects_[i] = rects_[--c];
         } else {
            for (int j = i + 1; j < c; ) {
               int a1 = rects_[i].GetArea() + rects_[j].GetArea();
               if (MergeRects(rects_[i], rects_[j])) {
                  int a2 = rects_[i].GetArea();
                  ASSERT(a1 == a2);
                  rects_[j] = rects_[--c];
                  changed = true;
               } else {
                  j++;
               }
            }
            i++;
         }
      }
   } while (changed);

   rects_.resize(c);

   //ASSERT(*this == tester);
}

bool Region2d::MergeRects(Rect2d &a, const Rect2d& b)
{
   if (a.p0.y == b.p0.y && a.p1.y == b.p1.y) {
      if (a.p1.x == b.p0.x) {
         // a is directly to the left of b.
         a.p1.x = b.p1.x;
         return true;
      }
      if (a.p0.x == b.p1.x) {
         // a is directly to the right of b.
         a.p0.x = b.p0.x;
         return true;
      }
   }
   if (a.p0.x == b.p0.x && a.p1.x == b.p1.x) {
      if (a.p1.y == b.p0.y) {
         // a is directly above b.
         a.p1.y = b.p1.y;
         return true;
      }
      if (a.p0.y == b.p1.y) {
         // a is directly below b.
         a.p0.y = b.p0.y;
         return true;
      }
   }
   return false;
}

std::ostream& math2d::operator<<(std::ostream& out, const Region2d& source) {
   return out << "(region2d...)";
}
