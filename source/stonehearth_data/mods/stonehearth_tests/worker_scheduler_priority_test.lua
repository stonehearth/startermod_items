local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3

local WorkerSchedulerPriorityTest = class(MicroWorld)

function WorkerSchedulerPriorityTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   -- A pile of wood...
   self:place_item_cluster('stonehearth:oak_log', 5, 5, 4, 4);

   -- Two workers near the wood...
   local worker = self:place_citizen(13, 13)
   local worker = self:place_citizen(12, 14)
   local faction = worker:get_component('unit_info'):get_faction()

   --- A stockpile close to the workers and the wood...
   self:place_stockpile_cmd(faction, 10, 10, 2, 2)
   
   --- A tasty wall, fairly far away from everything else
   local wall = radiant.entities.create_entity('stonehearth:wooden_wall')
   local rgn = _radiant.sim.alloc_region()
   rgn:modify():add_cube(Cube3(Point3(0, 0, 0), Point3(1, 4, 2)))
   wall:add_component('destination'):set_region(rgn)
   wall:add_component('stonehearth:construction_data'):set_normal(Point3(1, 0, 0))
   radiant.entities.set_faction(wall, faction)
   
   radiant.entities.move_to(wall, Point3(0, 1, 0))
   local root = radiant.entities.get_root_entity()
   local city_plan = root:add_component('stonehearth:city_plan')
   city_plan:add_blueprint(wall)
end

return WorkerSchedulerPriorityTest

