#include "pch.h"
#include <bitset>
#include "csg/util.h"
#include "qubicle_file.h"
#include "qubicle_brush.h"
#include <unordered_set>

using namespace ::radiant;
using namespace ::radiant::voxel;

static const struct {
   int i, u, v, mask;
   csg::Point3f normal;
} qubicle_node_edges__[] = {
   { 2, 0, 1, FRONT_MASK,  csg::Point3f( 0,  0,  1) },
   { 2, 0, 1, BACK_MASK,   csg::Point3f( 0,  0, -1) },
   { 1, 0, 2, TOP_MASK,    csg::Point3f( 0,  1,  0) },
   { 1, 0, 2, BOTTOM_MASK, csg::Point3f( 0, -1,  0) },
   { 0, 2, 1, RIGHT_MASK,  csg::Point3f(-1,  0,  0) },
   { 0, 2, 1, LEFT_MASK,   csg::Point3f( 1,  0,  0) },
};

QubicleBrush::QubicleBrush() :
   normal_(0, 0, -1),
   qubicle_matrix_(nullptr),
   paint_mode_(Color),
   offset_mode_(Matrix),
   qubicle_file_(""),
   clip_whitespace_(false),
   lod_level_(0)
{
}

QubicleBrush::QubicleBrush(std::istream& in) :
   normal_(0, 0, -1),
   paint_mode_(Color),
   offset_mode_(Matrix),
   qubicle_file_(""),
   clip_whitespace_(false),
   lod_level_(0)
{
   in >> qubicle_file_;
   qubicle_matrix_ = &qubicle_file_.begin()->second;
}

QubicleBrush::QubicleBrush(QubicleMatrix const* m) :
   normal_(0, 0, -1),
   qubicle_matrix_(m),
   paint_mode_(Color),
   offset_mode_(Matrix),
   qubicle_file_(""),
   clip_whitespace_(false),
   lod_level_(0)
{
}

QubicleBrush::QubicleBrush(QubicleMatrix const* m, int lod_level) :
   normal_(0, 0, -1),
   qubicle_matrix_(m),
   paint_mode_(Color),
   offset_mode_(Matrix),
   qubicle_file_(""),
   clip_whitespace_(false),
   lod_level_(lod_level)
{
}

QubicleBrush& QubicleBrush::SetNormal(csg::Point3 const& normal)
{
   normal_ = normal;
   return *this;
}

QubicleBrush& QubicleBrush::SetPaintMode(PaintMode mode)
{
   paint_mode_ = mode;
   return *this;
}

QubicleBrush& QubicleBrush::SetClipWhitespace(bool clip)
{
   clip_whitespace_ = clip;
   return *this;
}

QubicleBrush& QubicleBrush::SetOffsetMode(OffsetMode mode)
{
   offset_mode_ = mode;
   return *this;
}

csg::Region3 QubicleBrush::PaintOnce()
{
   return PreparePaintBrush();
}

csg::Region3 QubicleBrush::PaintThroughStencil(csg::Region3 const& stencil)
{   
   csg::Region3 brush = PreparePaintBrush();
   return IterateThroughStencil(brush, stencil);
}

csg::Region3 QubicleBrush::PreparePaintBrush()
{
   if (!qubicle_matrix_) {
      throw std::logic_error("could not find qubicle matrix for voxel brush");
   }

   const QubicleMatrix loddedMatrix = Lod(*qubicle_matrix_, lod_level_);
   csg::Region3 brush = MatrixToRegion3(loddedMatrix);

   // rotate if necessary...
   if (normal_ != csg::Point3(0, 0, -1)) {
      // this is hacky and stupid.  MatrixToRegion3 has access to the normal.
      // it should simply create boxes with the right position!
      brush = csg::Reface(brush, normal_);
   }
   return brush;
}

float colorDistance(int colorA, int colorB)
{
   const csg::Color4 cA = csg::Color4::FromInteger(colorA);
   const csg::Color4 cB = csg::Color4::FromInteger(colorB);

   const csg::Point3f hslA = cA.ToHsl();
   const csg::Point3f hslB = cB.ToHsl();

   float dH = hslA.x - hslB.x;
   float dS = hslA.y - hslB.y;
   float dL = hslA.z - hslB.z;

   // HSL distance, but we bias against lightness (valuing hue/saturation more.)
   return sqrt((dH * dH) + (dS * dS) + (dL * dL / 10.0f));
}

bool pairSort(const std::pair<int,int>& a, const std::pair<int,int>& b)
{
   /// Smallest -> Greatest
   return a.second > b.second;
}

QubicleMatrix QubicleBrush::Lod(const QubicleMatrix& m, int lod_level)
{
   if (lod_level == 0) {
      return m;
   }

   const csg::Point3& size = m.GetSize();
   QubicleMatrix result(size, m.GetPosition(), m.GetName());

   // Lod level 1:
   // Collect up all color frequencies
   std::unordered_map<int, int> colorCounts;
   std::vector<std::pair<int, int> > colorVec;
   for (int x = 0; x < size.x; x ++) {
      for (int y = 0; y < size.y; y ++) {
         for (int z = 0; z < size.z; z ++) {
            int colorI = m.At(x,y,z);
            csg::Color4 color = csg::Color4::FromInteger(colorI);
            if (color.a > 0) {
               int colNoAlpha = csg::Color3::FromInteger(colorI).ToInteger();
               if (colorCounts.find(colNoAlpha) == colorCounts.end()) {
                  colorCounts[colNoAlpha] = 0;
               }
               colorCounts[colNoAlpha]++;
            }
         }
      }
   }

   // Sort colours by frequency (greatest to least).
   for (const auto& col : colorCounts) {
      colorVec.push_back(std::pair<int,int>(col.first, col.second));
   }
   std::sort(colorVec.begin(), colorVec.end(), pairSort);

   // Construct a palette of (at most 4) colours.  For each colour in the matrix, compare against 
   // the present palette, and if nothing too similar is present, add it to the palette.  Because 
   // we've sorted the colours by frequency, we'll definitely get the most prominent colours in the 
   // palette. Arguably, you could bias the minimum distance bigger as you add more colours, but 
   // beware!  Distances in colour space are weird, and reducing palettes in such a manner can 
   // produce...interesting...results.
   std::vector<int> finalPalette;
   for (const auto& colorFreq : colorVec) {
      const float MinDist = 0.15f;
      bool tooSimilar = false;
      for (const auto& paletteColor : finalPalette) {
         if (colorDistance(colorFreq.first, paletteColor) < MinDist) {
            tooSimilar = true;
            break;
         }
      }

      if (!tooSimilar) {
         finalPalette.push_back(colorFreq.first);
      }

      if (finalPalette.size() > 3) {
         break;
      }
   }

   // Map the colours onto the generated palette.
   std::unordered_map<int, int> colorMap;
   for (const auto& colorFreq : colorVec) {
      float minDist = 9999999;
      uint32 mostSimilarIdx = 0;

      for (uint32 j = 0; j < finalPalette.size(); j++) {
         float d = colorDistance(colorFreq.first, finalPalette[j]);
         if (d < minDist) {
            mostSimilarIdx = j;
            minDist = d;
         }
      }
      colorMap[colorFreq.first] = finalPalette[mostSimilarIdx];
   }

   // Replace the colors using the map.
   for (int x = 0; x < size.x; x++) {
      for (int y = 0; y < size.y; y++) {
         for (int z = 0; z < size.z; z++) {
            int colorI = m.At(x,y,z);
            csg::Color4 color = csg::Color4::FromInteger(colorI);
            if (color.a > 0) {
               colorI = csg::Color3::FromInteger(colorI).ToInteger();
               csg::Color4 mappedColor = csg::Color4::FromInteger(colorMap[colorI]);
               mappedColor.a = color.a;
               result.Set(x,y,z, mappedColor.ToInteger());
            } else {
               result.Set(x,y,z, colorI);
            }
         }
      }
   }

   return result;
}

csg::Region3 QubicleBrush::IterateThroughStencil(csg::Region3 const& brush,
                                                 csg::Region3 const& stencil)
{
   csg::Region3 model;
   csg::Cube3 brush_bounds = brush.GetBounds();
   csg::Cube3 stencil_bounds = stencil.GetBounds();
   csg::Point3 brush_size = brush_bounds.GetSize();
   csg::Point3 stencil_size = brush_bounds.GetSize();
   csg::Point3 const& brush_min = brush_bounds.GetMin();
   csg::Point3 const& stencil_min = stencil_bounds.GetMin();
   csg::Point3 const& brush_max = brush_bounds.GetMax();
   csg::Point3 const& stencil_max = stencil_bounds.GetMax();

   // Figure out the extents of our iterator...
   csg::Point3 min, max;
   for (int i = 0; i < 3; i++) {
      // how far back do we have to go to get the brush to cover the tile...
      int dist_to_min_edge = std::min(stencil_min[i] - brush_min[i], 0);
      min[i] = csg::GetChunkIndex(dist_to_min_edge, brush_size[i]);

      // and how far over?
      int start_pos = min[i] * brush_size[i];
      int dist_to_max_edge = std::max(stencil_max[i] - start_pos, 0);
      max[i] = csg::GetChunkIndex(dist_to_max_edge, brush_size[i]) + 1;
   }

   // Now iterate and draw the brush
   for (csg::Point3 i : csg::Cube3(min, max)) {
      csg::Point3 offset = i * brush_size;
      csg::Region3 stamped = brush.Translated(offset) & stencil;
      model.AddUnique(stamped);
   }
   return model;
}


csg::Region3 QubicleBrush::MatrixToRegion3(QubicleMatrix const& matrix)
{
   const csg::Point3& size = matrix.GetSize();
   const csg::Point3 matrix_position = matrix.GetPosition();   

   csg::Point3 offset = csg::Point3::zero;
   if (offset_mode_ == File) {
      offset = matrix_position;
   }


   csg::Region3 result;

   std::vector<bool> processed;
   processed.resize(size.x * size.y * size.z);


#define OFFSET(x_, y_, z_)          ((y_ * size.z * size.x) + (z_ * size.x) + x_)
#define PROCESSED(x, y, z)          processed[OFFSET(x, y, z)]
#define MATCHES(c, m)               ((paint_mode_ == Color) ? (csg::Color3::FromInteger(c).ToInteger() == csg::Color3::FromInteger(m).ToInteger()) : (csg::Color4::FromInteger(c).a > 0))

   // Strip out all the alpha=0 voxels before proceeding.
   for (int x = 0; x < size.x; x++) {
      for (int y = 0; y < size.y; y++) {
         for (int z = 0; z < size.z; z++) {
            csg::Color4 c = csg::Color4::FromInteger(matrix.At(x, y, z));
            if (c.a == 0) {
               PROCESSED(x, y, z) = true;
            }
         }
      }
   }

   // From: http://mikolalysenko.github.com/MinecraftMeshes2/js/greedy.js
   // There it is, that's a straw, you see? You watching? And my straw
   // reaches acroooooooss the room, and starts to drink your milkshake...
   // I... drink... your... milkshake!
   csg::Point3 i;
   for (int y = 0; y < size.y; y++) {
      for (int z = 0; z < size.z; z++) {
         int x = 0;
         while (x < size.x) {
            // skip voxels we've already processed
            if (PROCESSED(x, y, z)) {
               x++;
               continue;
            }
            int color = matrix.At(x, y, z);

            // we've found an unprocessed pixel, non-transparent at x, y, z.
            // get the biggest rect we can.  start off in the xz-plane
            int x_max, z_max, y_max, u, v, w;
            for (x_max = x + 1; x_max < size.x; x_max++) {
               int c = matrix.At(x_max, y, z);
               if (PROCESSED(x_max, y, z) ||  !MATCHES(c, color)) {
                  goto finished_x;
               }
            }

finished_x:
            for (z_max = z + 1; z_max < size.z; z_max++) {
               for (u = x; u < x_max; u++) {
                  int c = matrix.At(u, y, z_max);
                  if (PROCESSED(u, y, z_max) || !MATCHES(c, color)) {
                     goto finished_xz;
                  }
               }
            }

finished_xz:
            // Now go up in the y direction...
            for (y_max = y + 1; y_max < size.y; y_max++) {
               for (w = z; w < z_max; w++) {
                  for (u = x; u < x_max; u++) {
                     int c = matrix.At(u, y_max, w);
                     if (PROCESSED(u, y_max, w) || !MATCHES(c, color)) {
                        goto finished_xyz;
                     }
                  }
               }
            }

finished_xyz:
            // Add a cube of the current color and mark all this stuff as processed
            csg::Point3 cmin = csg::Point3(x, y, z);
            csg::Point3 cmax = csg::Point3(x_max, y_max, z_max);

            result.AddUnique(csg::Cube3(cmin + offset, cmax + offset, csg::Color3::FromInteger(color).ToInteger()));

            for (v = y; v < y_max; v++) {
               for (w = z; w < z_max; w++) {
                  for (u = x; u < x_max; u++) {
                     PROCESSED(u, v, w) = true;
                  }
               }
            }
            x = x_max;
         }
      }
   }
   for (int i = 0; i < size.x * size.y * size.z; i++) {
      ASSERT(processed[i]);
   }

   // strip off the white space...
   if (clip_whitespace_) {
      csg::Cube3 bounds = result.GetBounds();
      csg::Point3 offset;
      for (int i = 0; i < 3; i++) {
         offset[i] = std::max(bounds.GetMin()[i], 0);
      }
      if (offset != csg::Point3(0, 0, 0)) {
         result.Translate(-offset);
      }
   }
   return result;
}
