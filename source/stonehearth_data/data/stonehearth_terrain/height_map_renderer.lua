local HeightMap = require 'height_map'
local TerrainType = require 'terrain_type'

local Terrain = _radiant.om.Terrain
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region2 = _radiant.csg.Region2
local Region3 = _radiant.csg.Region3
local HeightMapCPP = _radiant.csg.HeightMap

local HeightMapRenderer = class()

-- delegate to C++ to "tesselate" heightmap into rectangles
function HeightMapRenderer.render_height_map_to_terrain(height_map, terrain_info)
   local r2 = Region2()
   local r3 = Region3()
   local height_map_cpp = HeightMapCPP(height_map.width, 1) -- Assumes square map!
   local terrain = radiant._root_entity:add_component('terrain')

   HeightMapRenderer._copy_heightmap_to_CPP(height_map_cpp, height_map)
   _radiant.csg.convert_heightmap_to_region2(height_map_cpp, r2)

   for rect in r2:contents() do
      if rect.tag > 0 then
         HeightMapRenderer._add_land_to_region(r3, rect, rect.tag, terrain_info);         
      end
   end

   terrain:add_region(r3)
end

function HeightMapRenderer._copy_heightmap_to_CPP(height_map_cpp, height_map)
   local row_offset = 0

   for j=1, height_map.height do
      for i=1, height_map.width do
         height_map_cpp:set(i-1, j-1, height_map[row_offset+i])
      end
      row_offset = row_offset + height_map.width
   end
end

function HeightMapRenderer._add_land_to_region(dst, rect, height, terrain_info)
   local foothills_step_size = terrain_info[TerrainType.Foothills].step_size
   local foothills_max_height = terrain_info[TerrainType.Foothills].max_height
   local tree_line = terrain_info.tree_line

   dst:add_cube(Cube3(Point3(rect.min.x, -2, rect.min.y),
                      Point3(rect.max.x,  0, rect.max.y),
                Terrain.BEDROCK))

   -- Mountains
   if height > terrain_info[TerrainType.Foothills].max_height then
      if height > tree_line then
         dst:add_cube(Cube3(Point3(rect.min.x, 0,         rect.min.y),
                            Point3(rect.max.x, tree_line, rect.max.y),
                      Terrain.TOPSOIL))

         dst:add_cube(Cube3(Point3(rect.min.x, tree_line, rect.min.y),
                            Point3(rect.max.x, height,    rect.max.y),
                      Terrain.BEDROCK))
      else
         dst:add_cube(Cube3(Point3(rect.min.x, 0,         rect.min.y),
                            Point3(rect.max.x, height,    rect.max.y),
                      Terrain.TOPSOIL))
      end
      return
   end

   -- Plains
   if height <= terrain_info[TerrainType.Plains].max_height then

      dst:add_cube(Cube3(Point3(rect.min.x, 0,        rect.min.y),
                         Point3(rect.max.x, height-1, rect.max.y),
                   Terrain.TOPSOIL))

      dst:add_cube(Cube3(Point3(rect.min.x, height-1, rect.min.y),
                         Point3(rect.max.x, height,   rect.max.y),
                   Terrain.PLAINS))
      return
   end

   -- Foothills
   if height % foothills_step_size == 0 then

      dst:add_cube(Cube3(Point3(rect.min.x, 0,        rect.min.y),
                         Point3(rect.max.x, height-1, rect.max.y),
                   Terrain.TOPSOIL))

      dst:add_cube(Cube3(Point3(rect.min.x, height-1, rect.min.y),
                         Point3(rect.max.x, height,   rect.max.y),
                   Terrain.GRASS))
   else
      dst:add_cube(Cube3(Point3(rect.min.x, 0,        rect.min.y),
                         Point3(rect.max.x, height,   rect.max.y),
                   Terrain.TOPSOIL))
   end
end

return HeightMapRenderer
