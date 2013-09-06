native:log('requiring microworld')

local MicroWorld = radiant.mods.require('/stonehearth_tests/lib/micro_world.lua')
local TerrainGenerator = radiant.mods.require('/stonehearth_terrain/terrain_generator.lua')
local ZoneType = radiant.mods.require('/stonehearth_terrain/zone_type.lua')

local Terrain = _radiant.om.Terrain
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Region2 = _radiant.csg.Region2
local Region3 = _radiant.csg.Region3
local HeightMapCPP = _radiant.csg.HeightMap

local NewWorld = class(MicroWorld)

function NewWorld:__init()
   self[MicroWorld]:__init()
   self._terrain_generator = TerrainGenerator()
   self:create_world()
   self:place_objects()
end

function NewWorld:create_world()
   local height_map

   height_map = self._terrain_generator:generate_zone(ZoneType.Foothills)
   self:_render_height_map_to_terrain(height_map)
end

function NewWorld:place_objects()
   local camp_x = 100
   local camp_z = 100

   local tree = self:place_tree(camp_x-12, camp_z-12)
   --local worker = self:place_citizen(camp_x-8, camp_z-8)
   
   local worker = self:place_citizen(camp_x-3, camp_z-3)
   self:place_citizen(camp_x+0, camp_z-3)
   self:place_citizen(camp_x+3, camp_z-3)
   self:place_citizen(camp_x-3, camp_z+3)
   self:place_citizen(camp_x+0, camp_z+3)
   self:place_citizen(camp_x+3, camp_z+3)
   self:place_citizen(camp_x-3, camp_z+0)
   self:place_citizen(camp_x+3, camp_z+0)

   local faction = worker:get_component('unit_info'):get_faction()
   
   self:place_stockpile_cmd(faction, camp_x+8, camp_z-2, 4, 4)
end

----------

-- delegate to C++ to "tesselate" heightmap into rectangles
function NewWorld:_render_height_map_to_terrain(height_map)
   local r2 = Region2()
   local r3 = Region3()
   local heightMapCPP = HeightMapCPP(height_map.width, 1) -- Assumes square map!
   local terrain = radiant._root_entity:add_component('terrain')

   self:_copy_heightmap_to_CPP(heightMapCPP, height_map)
   _radiant.csg.convert_heightmap_to_region2(heightMapCPP, r2)

   for rect in r2:contents() do
      if rect.tag > 0 then
         self:_add_land_to_region(r3, rect, rect.tag);         
      end
   end

   terrain:add_region(r3)
end

function NewWorld:_copy_heightmap_to_CPP(heightMapCPP, height_map)
   local row_offset = 0

   for j=1, height_map.height, 1 do
      for i=1, height_map.width, 1 do
         heightMapCPP:set(i-1, j-1, math.abs(height_map[row_offset+i]))
      end
      row_offset = row_offset + height_map.width
   end
end

function NewWorld:_add_land_to_region(dst, rect, height)
   local foothills_quantization_size = 
      self._terrain_generator.zone_params[ZoneType.Foothills].quantization_size

   dst:add_cube(Cube3(Point3(rect.min.x, -2, rect.min.y),
                      Point3(rect.max.x,  0, rect.max.y),
                Terrain.BEDROCK))

   if height % foothills_quantization_size == 0 then
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

return NewWorld

