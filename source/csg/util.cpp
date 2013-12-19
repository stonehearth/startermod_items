#include "pch.h"
#include "util.h"

using namespace ::radiant;
using namespace ::radiant::csg;

// Most of this code is slated to be eliminated once tesselations moves to the
// server and we use region_tools for edging
#define EDGE_LOG(level)

#define CLIP_ZONE_BOUNDARIES

std::ostream& csg::operator<<(std::ostream& os, EdgePointX const& f)
{
   return os << "[EdgePointX (" << f.x << ", " << f.y << ")  facing: " << f.normals << "]";
}

std::ostream& csg::operator<<(std::ostream& os, EdgeX const& f)
{
   if (!f.start || !f.end) {
      return os << "[EdgeX with invalid start or end points]";
   }
   return os << "[EdgeX (" << f.start->x << ", " << f.start->y << ") to "
                          << f.end->x << ", " << f.end->y << ")  facing: "
                          << f.normal << "]";
}

std::ostream& csg::operator<<(std::ostream& os, EdgeList const& f)
{
   return os << "[EdgeList of " << f.points.size() << " points]";
}

int csg::GetChunkIndex(int value, int width)
{
   int index;
   if (value >= 0) {
      index = value / width;
   } else {
      index = ((value - (width - 1)) / width);
   }
   return index;
}

void csg::GetChunkIndex(int value, int chunk_width, int& index, int& offset)
{
   index = csg::GetChunkIndex(value, chunk_width);
   if (value >= 0) {
      offset = value - (index * chunk_width);
   } else {
      offset = -(index * chunk_width) - -value;
   }
   ASSERT(offset >= 0 && offset < chunk_width);
}

Point3 csg::GetChunkIndex(Point3 const& value, int chunk_width)
{
   Point3 index;
   for (int i = 0; i < 3; i++) {
      index[i] = GetChunkIndex(value[i], chunk_width);
   }
   return index;
}

void csg::GetChunkIndex(Point3 const& value, int chunk_width, Point3& index, Point3& offset)
{
   for (int i = 0; i < 3; i++) {
      GetChunkIndex(value[i], chunk_width, index[i], offset[i]);
   }
}

Cube3 csg::GetChunkIndex(Cube3 const& value, int chunk_width)
{
   Point3 ceil(chunk_width - 1, chunk_width - 1, chunk_width - 1);
   Cube3 index = Cube3(GetChunkIndex(value.min, chunk_width),
                       GetChunkIndex(value.max + ceil, chunk_width),
                       value.GetTag());
   return index;
}

bool csg::PartitionCubeIntoChunks(Cube3 const& cube, int width, std::function<bool (Point3 const& index, Cube3 const& cube)> cb)
{
   Point3 const& cmin = cube.GetMin();
   Point3 const& cmax = cube.GetMax();
   Cube3 chunks = GetChunkIndex(cube, width);

   for (Point3 const& cursor : chunks) {
      Cube3 c;
      for (int i = 0; i < 3; i++) {
         c.min[i] = std::max(cmin[i] - cursor[i] * width, 0);
         c.max[i] = std::min(cmax[i] - cursor[i] * width, width);
      }
      c.SetTag(cube.GetTag());

      // sometimes this generates cubes of 0 dimensions. not sure why yet...
      if (c.GetArea() > 0) {
         if (!cb(cursor, c)) {
            return false;
         }
      }
   }
   return true;
}


int csg::GetChunkAddress(int value, int width)
{
   return GetChunkIndex(value, width) * width;
}

Region3 csg::Reface(Region3 const& rgn, Point3 const& forward)
{
   Region3 result;   
   int cos_theta, sin_theta, tx = 0, tz = 0;

   if (forward == csg::Point3(0, 0, -1)) { // 0...
      return rgn;
   } else if (forward == csg::Point3(1, 0, 0)) { // 90...
      cos_theta = 0;
      sin_theta = 1;
      tx = 1;
   } else if (forward == csg::Point3(0, 0, 1)) { // 180...
      cos_theta = -1; 
      sin_theta = 0;
      tx = 1;
      tz = 1;
   } else if (forward == csg::Point3(-1, 0, 0)) { // 270...
      cos_theta = 0; 
      sin_theta = -1;
      tz = 1;
   } else {
      throw std::invalid_argument(BUILD_STRING("vector is not a valid normal refacing region: " << forward));
   }

   for (const auto& cube : rgn) {
      csg::Point3 const& min = cube.GetMin();
      csg::Point3 const& max = cube.GetMax();
      result.AddUnique(Cube3::Construct(Point3(min.x * cos_theta - min.z * sin_theta + tx,
                                               min.y,
                                               min.x * sin_theta + min.z * cos_theta + tz),
                                        Point3(max.x * cos_theta - max.z * sin_theta + tx,
                                               max.y,
                                               max.x * sin_theta + max.z * cos_theta + tz),
                                        cube.GetTag()));
   }
   return result;
}


Region3 csg::GetAdjacent(Region3 const& r, bool allow_diagonals, int min_y, int max_y)
{
   Region3 adjacent;

   for (const csg::Cube3& c : r) {
      csg::Point3 p0 = c.GetMin();
      csg::Point3 p1 = c.GetMax();
      if (allow_diagonals) {
         adjacent.Add(csg::Cube3(p0 + csg::Point3(-1, 0, -1), p1 + csg::Point3(1, 0, 1)));
      } else {
         csg::Point3 delta = p1 - p0;
         int x = delta.x, y = delta.y, z = delta.z;

         adjacent.Add(csg::Cube3(p0 - csg::Point3(0, 0, 1), p0 + csg::Point3(x, y, 0)));     // top
         adjacent.Add(csg::Cube3(p0 - csg::Point3(1, 0, 0), p0 + csg::Point3(0, y, z)));     // left
         adjacent.Add(csg::Cube3(p0 + csg::Point3(0, 0, z), p0 + csg::Point3(x, y, z + 1))); // bottom
         adjacent.Add(csg::Cube3(p0 + csg::Point3(x, 0, 0), p0 + csg::Point3(x + 1, y, z))); // right
      }
   }

   adjacent.Optimize();
   return adjacent;
}

void csg::HeightmapToRegion2(HeightMap<double> const& h, Region2& r)
{
   HeightMap<int> n(h);
   int size = n.get_size();
   int scale = n.get_scale();
   auto samples = n.get_samples();

   int search_offset = 0;
   int search_len = size * size;
   
   while (search_offset < search_len) {
      int search_value = samples[search_offset];
      if (search_value == INT_MAX) {
         search_offset++;
         continue;
      }
      int y0 = search_offset / size;
      int x0 = search_offset % size;
      int x1 = x0 + 1;
      int y1 = y0 + 1;
      while (x1 < size && samples[y0 * size + x1] == search_value) {
         x1++;
      }
      while (y1 < size) {
         int i;
         for (i = x0; i < x1; i++) {
            if (samples[y1 * size + i] != search_value) {
               break;
            }
         }
         if (i != x1) {
            break;
         }
         y1++;
      }

      // yay! add the rect
      r.AddUnique(Rect2(Point2(x0 * scale, y0 * scale),
                        Point2(x1 * scale, y1 * scale),
                        search_value));

      // clear the rect
      for (int y = y0; y < y1; y++) {
         for (int x = x0; x < x1; x++) {
            samples[y * size + x] = INT_MAX;
         }
      }
      search_offset += (x1 - x0);
   }
}

bool ::csg::Region3Intersects(const Region3& rgn, const csg::Ray3& ray, float& distance)
{
   float best = FLT_MAX;
   for (const auto &r : rgn) { 
      float candidate;
      if (Cube3Intersects(r, ray, candidate) && candidate < best) {
         best = candidate;
      }
   }
   if (best != FLT_MAX) {
      distance = best;
      return true;
   }
   return false;
}


EdgeListPtr csg::Region2ToEdgeList(Region2 const& rgn, int height, Region3 const& clipper)
{
   EdgeListPtr edges = std::make_shared<EdgeList>();
   typedef std::unordered_map<int, Region1> edge_map; 
   
   auto add_region = [&](Region1 const& region, int plane_value, int plane, int normal_dir)
   {
      Point2 normal(0, 0);
      int coord = (plane == 0) ? 1 : 0;
      normal[plane] = normal_dir;

      std::vector<Line1> lines = region.GetContents();

      // Join adjacent: xxx - why not just region.Optimize()?
      int i = 0, c = lines.size();
      for (i = 0; i < c; i++) {
         Line1& lhs = lines[i];
         int j = i + 1;
         while (j < c) {
            Line1 const& rhs = lines[j];
            if (lhs.GetMax().x == rhs.GetMin().x) {
               lhs.SetMax(rhs.GetMax());
               lines[j] = lines[--c];
               j = i + 1;
               continue;
            }
            if (lhs.GetMin().x == rhs.GetMax().x) {
               lhs.SetMin(rhs.GetMin());
               lines[j] = lines[--c];
               j = i + 1;
               continue;
            }
            j++;
         }
      }
      lines.resize(c);

      // add optimized list
      for (Line1 const& r: lines) {
         Point2 p0, p1;
         p0[coord] = r.GetMin().x;
         p1[coord] = r.GetMax().x;
         p0[plane] = plane_value;
         p1[plane] = plane_value;
         edges->AddEdge(p0, p1, normal);
      }
   };

   auto add_edges = [&](edge_map const& edges, edge_map const& occluders, edge_map const& occluders2, int plane, int normal_dir) {
      for (auto const& entry : edges) {
         auto i = occluders.find(entry.first);
         auto j = occluders2.find(entry.first);
         if (i == occluders.end()) {
            if (j == occluders2.end()) {
               add_region(entry.second, entry.first, plane, normal_dir);
            } else {
               add_region(entry.second - j->second, entry.first, plane, normal_dir);
            }
         } else {
            if (j == occluders2.end()) {
               add_region(entry.second - i->second, entry.first, plane, normal_dir);
            } else {
               add_region(entry.second - i->second - j->second, entry.first, plane, normal_dir);
            }
         }
      } 
   };

   for (int plane = 0; plane < 2; plane++) {
      edge_map front, back, clipper_front, clipper_back;
     
      for (Cube3 const& c : clipper) {
         csg::Point3 const& min = c.GetMin();
         csg::Point3 const& max = c.GetMax();
         int coord3d = (plane == 0 ? 2 : 0);
         int plane3d = (plane == 0 ? 0 : 2);

         // line below was:
         //    if (min.y <= height && max.y >= height) {
         // height of this layer was defined by height = Cube.GetMax().y-1
         // therefore to intersect this layer, max.y-1 >= height or max.y > height
         if (min.y <= height && max.y > height) {
            Point1 p0(min[coord3d]);
            Point1 p1(max[coord3d]);
            Line1 segment(p0, p1);

            clipper_front[min[plane3d]] += segment;
            clipper_back[max[plane3d]] += segment;            
         }
      }

#ifdef CLIP_ZONE_BOUNDARIES
      // define additional clippers along the zone boundaries
      int zone_size = 256;
      Line1 zone_edge(Point1(0), Point1(zone_size));
      clipper_front[zone_size] += zone_edge;
      clipper_back[0] += zone_edge;
#endif

      for (Rect2 const& r : rgn) {
         Point2 const& min = r.GetMin();
         Point2 const& max = r.GetMax();
         int coord = (plane == 0 ? 1 : 0);

         Point1 p0(min[coord]);
         Point1 p1(max[coord]);
         Line1 segment(p0, p1);

         front[min[plane]] += segment;
         back[max[plane]] += segment;
      }

      add_edges(front, back, clipper_back,  plane, -1);
      add_edges(back, front, clipper_front, plane,  1);
   }

#if defined(LOG_EDGES)
   EDGE_LOG(5) << "Post-join";
   for (uint i = 0; i < edges->edges.size(); i++) {
      EdgePtr segment = edges->edges[i];
      EDGE_LOG(5) << "segment " << i << " " << *segment->start << " to " << *segment->end << ".  normal " << segment->normal;
   }
#endif

   return edges;
}


csg::Region2 csg::EdgeListToRegion2(EdgeListPtr edges, int width, csg::Region2 const* clipper)
{
   csg::Region2 result;

#if defined(LOG_EDGES)
   EDGE_LOG(5) <<  "Converting segment list to region..." << width;
#endif

   for (EdgePtr segment : edges->edges) {
      if (segment->start->x <= segment->end->x && segment->start->y <= segment->end->y) {
         csg::Point2 p0, p1;
         csg::Rect2 rect;
         //csg::Point2 p0 = *segment->start;
         //csg::Point2 p1 = *segment->end;

         // draw a picture here...
         if (segment->normal.y == -1) {
            p0 = *segment->start;
            p1 = *segment->end - (segment->end->normals * width);
            //rect = csg::Rect2(p0, p1);
            rect = csg::Rect2::Construct(p0, p1);
         } else if (segment->normal.y == 1) {
            p0 = *segment->start - (segment->start->normals * width);
            p1 = *segment->end;
            //rect = csg::Rect2(p0, p1);
            rect = csg::Rect2::Construct(p0, p1);
         } else if (segment->normal.x == -1) {
            p0 = *segment->start - (segment->start->normals * width);
            p1 = *segment->end;
            rect = csg::Rect2::Construct(p0, p1);
         } else {
            p0 = *segment->start;
            p1 = *segment->end - (segment->end->normals * width);
            rect = csg::Rect2::Construct(p0, p1);
         }
#if defined(LOG_EDGES)
         EDGE_LOG(5) << "segment " << *segment->start << " to " << *segment->end << ".  normal " << segment->normal;
         EDGE_LOG(5) << "    start normal " << segment->end->normals;
         EDGE_LOG(5) << "    end normal   " << segment->end->normals;
         EDGE_LOG(5) << "    result       " << rect;
#endif
         result += rect;
      }
   }
   if (clipper) {
      result &= *clipper;
   }
   return result;
}

EdgeX::EdgeX(EdgePointPtr s, EdgePointPtr e, csg::Point2 const& norm) :
   start(s),
   end(e),
   normal(norm)
{
   int n = normal.x ? 0 : 1;
   ASSERT((*start)[n] == (*end)[n]);
}

void EdgeList::AddEdge(csg::Point2 const& start, csg::Point2 const& end, csg::Point2 const& normal)
{
   EdgePointPtr s = GetPoint(start, normal);
   EdgePointPtr e = GetPoint(end, normal);

   if (s->x > e->x || s->y > e->y) {
      std::swap(s, e);
   }
   ASSERT(s->x <= e->x && s->y <= e->y);
   EdgePtr segment = std::make_shared<EdgeX>(s, e, normal);

   //segment->start->edges.push_back(segment);
   //segment->end->edges.push_back(segment);
   //s->normals += normal;
   //e->normals += normal;
   edges.push_back(segment);
}

EdgePointPtr EdgeList::GetPoint(csg::Point2 const& pt, csg::Point2 const& normal)
{
   for (EdgePointPtr p : points) {
      if (p->x == pt.x && p->y == pt.y) {
         // Don't merge points with normals that point in opposite directions!
         if (normal.x > 0 && p->normals.x < 0 ||
             normal.y > 0 && p->normals.y < 0 ||
             normal.x < 0 && p->normals.x > 0 ||
             normal.y < 0 && p->normals.y > 0) {
            continue;
         }
         p->normals += normal;
         return p;
      }
   }
   EdgePointPtr p = std::make_shared<EdgePointX>(pt.x, pt.y, normal);
   points.push_back(p);
   return p;
}

void EdgeList::Inset(int distance)
{
   for (EdgePtr s : edges) {
      // only process edges that have not yet been moved
      csg::Point2 tweak = s->normal * distance;
      *s->end -= tweak;
      *s->start -= tweak;
   }
}

void EdgeList::Grow(int distance)
{
   Inset(-distance);
}

void EdgeList::Fragment()
{
   EdgeList fragmented;


   for (EdgePtr s : edges) {
      int t = s->normal.x ? 1 : 0;
      int n = s->normal.x ? 0 : 1;

      ASSERT((*s->start)[n] == (*s->end)[n]);

      int start = (*s->start)[t];
      int end = (*s->end)[t];
      if (start > end) {
         std::swap(start, end);
      }
      if (start < end) {
         csg::Point2 new_start, new_end;
         new_start[t] = (rand() % (end - start)) + start;
         new_end[t]   = (rand() % (end - start)) + start;

         if (new_start < new_end) {
            new_start[n] = new_end[n] = (*s->start)[n];
            fragmented.AddEdge(new_start, new_end, s->normal);
         }
      }
   }
   *this = fragmented;
}

