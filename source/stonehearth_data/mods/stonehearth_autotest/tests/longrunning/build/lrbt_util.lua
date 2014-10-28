local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3

local lrbt_util = {}

local WOODEN_COLUMN = 'stonehearth:wooden_column'
local WOODEN_WALL = 'stonehearth:wooden_wall'
local WOODEN_FLOOR = 'stonehearth:entities:wooden_floor'
local WOODEN_FLOOR_PATTERN = '/stonehearth/entities/build/wooden_floor/wooden_floor_diagonal.qb'
local WOODEN_ROOF = 'stonehearth:wooden_peaked_roof'
local STOREY_HEIGHT = stonehearth.constants.construction.STOREY_HEIGHT

function lrbt_util.get_area(structure)
   return structure:get_component('destination'):get_region():get():get_area()
end

function lrbt_util.count_structures(building)
   local counts = {}
   local structures = building:get_component('stonehearth:building')
                                 :get_all_structures()
   for type, entries in pairs(structures) do
      if next(entries) then
         counts[type] = 0
         for _, entry in pairs(entries) do
            counts[type] = counts[type] + 1
         end
      end
   end
   return counts
end

function lrbt_util.create_wooden_floor(session, cube)
   return stonehearth.build:add_floor(session, WOODEN_FLOOR, cube, WOODEN_FLOOR_PATTERN)
end

function lrbt_util.erase_floor(session, cube)
   stonehearth.build:erase_floor(session, cube)
end

function lrbt_util.create_wooden_wall(session, p0, p1)
   local t, n = 'x', 'z'
   if p0.x == p1.x then
      t, n = 'z', 'x'
   end
   local normal = Point3(0, 0, 0)
   if p0[t] < p1[t] then
      normal[n] = 1
   else
      p0, p1 = p1, p0
      normal[n] = -1
   end

   return stonehearth.build:add_wall(session, WOODEN_COLUMN, WOODEN_WALL, p0, p1, normal)
end

function lrbt_util.grow_wooden_walls(session, building)
   return stonehearth.build:grow_walls(building, WOODEN_COLUMN, WOODEN_WALL)
end

function lrbt_util.grow_wooden_roof(session, building)
   return stonehearth.build:grow_roof(building, WOODEN_ROOF, {
         nine_grid_gradiant = { 'left', 'right' },
         nine_grid_max_height = 10,
      })
end

function lrbt_util.create_endless_entity(autotest, x, y, uri)
   local trace
   local entity = autotest.env:create_entity(x, y, uri)
   trace = entity:trace('make more logs')
                     :on_destroyed(function()
                           trace:destroy()
                           if not autotest:is_finished() then
                              lrbt_util.create_endless_entity(autotest, x, y, uri)
                           end
                        end)
end


return lrbt_util
