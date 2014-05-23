local MicroWorld = require 'lib.micro_world'
local BuildTest = class(MicroWorld)

local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3

function BuildTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   self:place_item_cluster('stonehearth:oak_log', 8, 8, 7, 7)
   self:place_item_cluster('stonehearth:berry_basket', -8, -8, 2, 2)
   self:place_item_cluster('stonehearth:wooden_door_proxy', -8, 8, 2, 2)
   self:place_citizen(0, 0)
   --self:place_citizen(2, 2)
   
   if true then
      return
   end
   for i = -8, 8, 4 do
      self:place_citizen(0, i)
   end
   
   
   
   local root = radiant.entities.get_root_entity()
   local city_plan = root:add_component('stonehearth:city_plan')

   -- add two columns..
   local s = 6;
   local loc = {
      Point3(0, 0, 0),
      Point3(s, 0, 0),
      Point3(s, 0, s),
      Point3(0, 0, s),
   }
   local room = radiant.entities.create_entity()
   room:set_debug_text('container room...')

   local columns = {}
   local walls = {}

   for i = 1,4 do 
      columns[i] = radiant.entities.create_entity('stonehearth:wooden_column')
      columns[i]:add_component('mob'):set_location_grid_aligned(loc[i])
      radiant.entities.set_faction(columns[i], worker)
      --room:add_component('entity_container'):add_child(columns[i])
   end
   
   local last_col = 1
   for i = 1,4 do
      local next_col = last_col == 4 and 1 or (last_col + 1)
      walls[i] = radiant.entities.create_entity('stonehearth:wooden_wall')
      walls[i]:add_component('stonehearth:wall'):connect_to(columns[last_col], columns[next_col])
      radiant.entities.set_faction(walls[i], worker)
      room:add_component('entity_container'):add_child(walls[i])
      last_col = next_col
      break
   end
   
   -- add it to the city plan.
   city_plan:add_blueprint(Point3(2, 1, 2), room)

   -- give it some personality...
   --local wall = blueprint:get_component('stonehearth:wall')
   --wall:set_width(10)
end

return BuildTest

