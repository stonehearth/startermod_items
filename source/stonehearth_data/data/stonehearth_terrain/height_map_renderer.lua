local HeightMap = require 'height_map'
local ZoneType = require 'zone_type'

local Terrain = _radiant.om.Terrain
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region2 = _radiant.csg.Region2
local Region3 = _radiant.csg.Region3
local HeightMapCPP = _radiant.csg.HeightMap

local HeightMapRenderer = class()

-- delegate to C++ to "tesselate" heightmap into rectangles
function HeightMapRenderer.render_height_map_to_terrain(height_map, zone_params)
   local r2 = Region2()
   local r3 = Region3()
   local heightMapCPP = HeightMapCPP(height_map.width, 1) -- Assumes square map!
   local terrain = radiant._root_entity:add_component('terrain')

   HeightMapRenderer._copy_heightmap_to_CPP(heightMapCPP, height_map)
   _radiant.csg.convert_heightmap_to_region2(heightMapCPP, r2)

   for rect in r2:contents() do
      if rect.tag > 0 then
         HeightMapRenderer._add_land_to_region(r3, rect, rect.tag, zone_params);         
      end
   end

   terrain:add_region(r3)
end

function HeightMapRenderer._copy_heightmap_to_CPP(heightMapCPP, height_map)
   local row_offset = 0

   for j=1, height_map.height, 1 do
      for i=1, height_map.width, 1 do
         heightMapCPP:set(i-1, j-1, height_map[row_offset+i])
      end
      row_offset = row_offset + height_map.width
   end
end

function HeightMapRenderer._add_land_to_region(dst, rect, height, zone_params)
   local foothills_step_size = zone_params[ZoneType.Foothills].step_size
   local grass_transition_height = zone_params.grass_transition_height
   local rock_transition_height = zone_params.rock_transition_height

   dst:add_cube(Cube3(Point3(rect.min.x, -2, rect.min.y),
                      Point3(rect.max.x,  0, rect.max.y),
                Terrain.BEDROCK))

   -- Mountains
   if (height > rock_transition_height) and
      (height % foothills_step_size == 0) then
      
      dst:add_cube(Cube3(Point3(rect.min.x, 0,        rect.min.y),
                         Point3(rect.max.x, height-1, rect.max.y),
                   Terrain.TOPSOIL))

      dst:add_cube(Cube3(Point3(rect.min.x, height-1, rect.min.y),
                         Point3(rect.max.x, height,   rect.max.y),
                   Terrain.TOPSOIL)) -- need mountain plateau color
      return
   end

   -- Plains
   if (height <= grass_transition_height) then

      dst:add_cube(Cube3(Point3(rect.min.x, 0,        rect.min.y),
                         Point3(rect.max.x, height-1, rect.max.y),
                   Terrain.TOPSOIL))

      dst:add_cube(Cube3(Point3(rect.min.x, height-1, rect.min.y),
                         Point3(rect.max.x, height,   rect.max.y),
                   Terrain.PLAINS))
      return
   end

   -- Foothills
   if (height % foothills_step_size == 0) then

      dst:add_cube(Cube3(Point3(rect.min.x, 0,        rect.min.y),
                         Point3(rect.max.x, height-1, rect.max.y),
                   Terrain.TOPSOIL))

      dst:add_cube(Cube3(Point3(rect.min.x, height-1, rect.min.y),
                         Point3(rect.max.x, height,   rect.max.y),
                   Terrain.GRASS))
      return
   end

   dst:add_cube(Cube3(Point3(rect.min.x, 0,        rect.min.y),
                      Point3(rect.max.x, height,   rect.max.y),
                Terrain.TOPSOIL))
end

return HeightMapRenderer
