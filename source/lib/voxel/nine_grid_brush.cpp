#include "pch.h"
#include "nine_grid_brush.h"

using namespace ::radiant;
using namespace ::radiant::voxel;

#define PACK16(x, y)    ((x << 16) | y)
#define UNPACK16(n)     std::tie((n >> 16) & 0xffff, n & 0xffff)

NineGridBrush::NineGridBrush(std::istream& in) :
   normal_(0, 0, -1),
   paint_mode_(Color),
   qubicle_file_(""),
   clip_whitespace_(false),
   y_offset_(0),
   maxHeight_(INT_MAX),
   gradiant_(0),
   slope_(1)
{
   in >> qubicle_file_;
}

NineGridBrush& NineGridBrush::SetNormal(csg::Point3 const& normal)
{
   normal_ = normal;
   return *this;
}

NineGridBrush& NineGridBrush::SetPaintMode(PaintMode mode)
{
   paint_mode_ = mode;
   return *this;
}

NineGridBrush& NineGridBrush::SetGridShape(csg::Region2 const& shape)
{
   shape_region2_ = shape;
   shapeBounds_ = shape.GetBounds();   

   return *this;
}

NineGridBrush& NineGridBrush::SetClipWhitespace(bool clip)
{
   clip_whitespace_ = clip;
   return *this;
}

NineGridBrush& NineGridBrush::SetSlope(float slope)
{
   slope_ = std::max(0.1f, std::min(1.0f, slope));
   return *this;
}

NineGridBrush& NineGridBrush::SetYOffset(int offset)
{
   y_offset_ = offset;
   return *this;
}

NineGridBrush& NineGridBrush::SetMaxHeight(int height)
{
   maxHeight_ =  height;
   return *this;
}

NineGridBrush& NineGridBrush::SetGradiantFlags(int flags)
{
   gradiant_ = flags;
   return *this;
}

csg::Region3 NineGridBrush::PaintOnce()
{
   return PaintThroughStencilOpt(nullptr);
}

csg::Region3 NineGridBrush::PaintThroughStencil(csg::Region3 const& stencil)
{
   return PaintThroughStencilOpt(&stencil);
}

csg::Region3 NineGridBrush::PaintThroughStencilOpt(csg::Region3 const* modelStencil)
{
   for (int i = 1; i <= 9; i++) {
      nine_grid_[i] = QubicleBrush(qubicle_file_.GetMatrix(BUILD_STRING(i)))
                          .SetPaintMode((QubicleBrush::PaintMode)paint_mode_)
                          .SetOffsetMode(QubicleBrush::Matrix)
                          .SetNormal(normal_)
                          .SetClipWhitespace(clip_whitespace_)
                          .PaintOnce();

      csg::Point3 bounds = nine_grid_[i].GetBounds().GetSize();
      brush_sizes_[i] = csg::Point2(bounds.x, bounds.z);
   }

   GridMap<short> heightmap(shapeBounds_, -1);
   GridMap<csg::Point2> gradiant(shapeBounds_, csg::Point2::zero);

   csg::EdgeMap<int, 2> edgeMap = csg::RegionTools<int, 2>().GetEdgeMap(shape_region2_);
   csg::Region2 ninegrid = ClassifyNineGrid(edgeMap);
   
   // Compute the height and gradiant of the roof.
   ComputeHeightMap(ninegrid, heightmap, gradiant);

   // finally, build the model for the grid
   csg::Region3 model;
   for (csg::Rect2 const& rect : ninegrid) {
      for (csg::Point2 const& src : rect) {
         int type = rect.GetTag();
         
         csg::Region3 brush = nine_grid_[type];
         csg::Point2 brushSize = brush_sizes_[type];

         csg::Point2 samplePos = src;

         // Revese the points depending on the gradiant...
         csg::Point2 g = gradiant.get(src, csg::Point2::zero);
         if (g.y) {
            samplePos.x = src.x - shapeBounds_.min.x;
            samplePos.y = src.y - shapeBounds_.min.y;
         } else {
            samplePos.x = shapeBounds_.max.y - src.y - 1;
            samplePos.y = shapeBounds_.max.x - src.x - 1;
         }

         // The sample offset is how far into the nine-grid section we need to
         // pull a column from.  It's based on the origin on the rect and wraps.
         csg::Point2 sampleOffset(samplePos.x % brushSize.x,
                                  samplePos.y % brushSize.y);


         // Make a stencil to pull just the column at sampleOffset out of the
         // brush and add it to the model
         csg::Cube3 stencil(csg::Point3(sampleOffset.x, -INT_MAX, sampleOffset.y),
                            csg::Point3(sampleOffset.x + 1, INT_MAX, sampleOffset.y + 1));

         csg::Region3 column = brush & stencil;

         // uncomment to color the region based on gradiant.
         /*
         {
            column = csg::Region3(csg::Cube3(csg::Point3(sampleOffset.x, 0, sampleOffset.y),
                                             csg::Point3(sampleOffset.x + 1, 1, sampleOffset.y + 1),
                                             csg::Color3(255 * abs(g.x), 255 * abs(g.y), 0).ToInteger()));
         }
         */

         // uncomment to color the region based on type.
         /*
         {
            static csg::Color3 colors[] = {
               csg::Color3::black,     // n/a
               csg::Color3::white,     // 1
               csg::Color3::orange,    // 2
               csg::Color3::red,       // 3
               csg::Color3::green,     // 4
               csg::Color3::grey,      // 5
               csg::Color3::brown,     // 6
               csg::Color3::blue,      // 7
               csg::Color3::pink,      // 8
               csg::Color3::purple,    // 9
            };
            column = csg::Region3(csg::Cube3(csg::Point3(sampleOffset.x, 0, sampleOffset.y),
                                             csg::Point3(sampleOffset.x + 1, 1, sampleOffset.y + 1),
                                             colors[type].ToInteger()));
         }
         */

         // dst offset is where to drop the column.  it's in the coordinate system
         // of the original Region2, offset by the computed height
         int height = static_cast<int>(heightmap.get(src, 0) * slope_);
         height = std::min(height, maxHeight_);

         csg::Point3 dstOffset = csg::Point3(src.x, height, src.y) - 
                                 csg::Point3(sampleOffset.x, 0, sampleOffset.y);

         if (modelStencil) {
            model.AddUnique(column.Translated(dstOffset) & *modelStencil);
         } else {
            model.AddUnique(column.Translated(dstOffset));
         }
      }
   }
   return model;
}

csg::Region2 NineGridBrush::ClassifyNineGrid(csg::EdgeMap<int, 2> const& edgeMap)
{
   int type = 0;
   csg::Point2 delta;
   csg::Region2 classified;

   for (csg::Rect2 const& r : shape_region2_) {
      classified.AddUnique(csg::Rect2(r.min, r.max, 5));
   }

   // Edges...
   for (csg::Edge<int, 2> const& edge : edgeMap.GetEdges()) {
      if (edge.normal == csg::Point2(0, -1)) {           // bottom edge
         type = 2, delta = csg::Point2(0, brush_sizes_[2].y);
      } else if (edge.normal == csg::Point2(-1, 0)) {    // left edge
         type = 4, delta = csg::Point2(brush_sizes_[4].x, 0);
      } else if (edge.normal == csg::Point2(0, 1)) {     // top edge
         type = 8, delta = csg::Point2(0, -brush_sizes_[8].y);
      } else if (edge.normal == csg::Point2(1, 0)) {     // right edge
         type = 6, delta = csg::Point2(-brush_sizes_[6].x, 0);
      } else {
         type = 0;
      }
      if (type != 0) {
         csg::Rect2 box = csg::Rect2::Construct(edge.min->location, edge.max->location + delta, type);
         classified.Add(box);
      }
   }

   // Corners...
   for (csg::EdgePoint<int, 2> const* point : edgeMap.GetPoints()) {
      csg::Rect2 box;
      if (point->accumulated_normals == csg::Point2(-1, -1)) {       // bottom left
         type = 1, delta = brush_sizes_[1];
      } else if (point->accumulated_normals == csg::Point2(1, -1)) { // bottom right
         type = 3, delta = csg::Point2(-brush_sizes_[3].x, brush_sizes_[3].y);
      } else if (point->accumulated_normals == csg::Point2(-1, 1)) { // top left
         type = 9, delta = csg::Point2(brush_sizes_[9].x, -brush_sizes_[9].y);
      } else if (point->accumulated_normals == csg::Point2(1, 1)) {  // top right
         type = 7, delta = -brush_sizes_[7];
      } else {
         type = 0;
      }
      if (type != 0) {
         csg::Rect2 box = csg::Rect2::Construct(point->location, point->location + delta, type);
         classified.Add(box);
      }
   }

   classified &= shape_region2_;

   return classified;
}

void NineGridBrush::ComputeHeightMap(csg::Region2 const& ninegrid, GridMap<short>& heightmap, GridMap<csg::Point2>& gradiant)
{
   GridMap<bool> updated(shapeBounds_, false);

   // Initialize the entire height of the roof to zero.
   for (csg::Rect2 const& rect : ninegrid) {
      for (csg::Point2 const& src : rect) {
         heightmap.set(src, 0);
      }
   }

   // Create a set of directions that the roof is allowed to grow
   // in based on the gradiant
   std::vector<csg::Point2> growDirections;

   if (gradiant_ & POSITIVE_X) {
      growDirections.push_back(csg::Point2( 1, 0));
   }
   if (gradiant_ & NEGATIVE_X) {
      growDirections.push_back(csg::Point2(-1, 0));
   }
   if (gradiant_ & POSITIVE_Z) {
      growDirections.push_back(csg::Point2( 0, 1));
   }
   if (gradiant_ & NEGATIVE_Z) {
      growDirections.push_back(csg::Point2( 0,-1));
   }
   if ((gradiant_ & POSITIVE_X) && (gradiant_ & POSITIVE_Z)) {
      growDirections.push_back(csg::Point2( 1, 1));
   }
   if ((gradiant_ & POSITIVE_X) && (gradiant_ & NEGATIVE_Z)) {
      growDirections.push_back(csg::Point2( 1,-1));
   }
   if ((gradiant_ & NEGATIVE_X) && (gradiant_ & POSITIVE_Z)) {
      growDirections.push_back(csg::Point2(-1, 1));
   }
   if ((gradiant_ & NEGATIVE_X) && (gradiant_ & NEGATIVE_Z)) {
      growDirections.push_back(csg::Point2(-1,-1));
   }

   // compute the fringe.
   std::vector<csg::Point2> fringe;
   for (csg::Rect2 const& rect : ninegrid) {
      for (csg::Point2 const& src : rect) {
         for (auto const& delta : growDirections) {
            if (heightmap.get(src + delta, -1) == -1) {
               gradiant.set(src, delta);
               updated.set(src, true);
               fringe.push_back(src);
               break;
            }
         }
      }
   }

   // keep growing the fringe until we run out of interior room
   bool more;
   do {
      more = false;
      std::vector<csg::Point2> nextfringe;
      for (csg::Point2 const& pt : fringe) {
         ASSERT(updated.get(pt, false));

         for (auto const& delta : growDirections) {
            csg::Point2 next = pt - delta;
            if (!updated.get(next, true)) {
               int height = heightmap.get(pt, 0);
               height++;

               gradiant.set(next, gradiant.get(pt, csg::Point2::zero));
               updated.set(next, true);
               heightmap.set(next, height);
               nextfringe.push_back(next);
               more = true;
            }
         }
      }
      fringe = std::move(nextfringe);
   } while (more);
}

