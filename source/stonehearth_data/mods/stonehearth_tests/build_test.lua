local MicroWorld = require 'lib.micro_world'
local BuildTest = class(MicroWorld)

local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3

function BuildTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   self:place_item_cluster('stonehearth.oak_log', 11, 11, 1, 1);
   --local tree = self:place_tree(-12, -12)
   local worker = self:place_citizen(13, 13)
   
   local root = radiant.entities.get_root_entity()
   local city_plan = root:add_component('stonehearth:city_plan')

   -- create the wall blueprint...
   local blueprint = radiant.entities.create_entity('stonehearth.wooden_wall') 
   radiant.entities.set_faction(blueprint, worker)

   -- add it to the city plan.
   city_plan:add_blueprint(Point3(4, 1, 4), blueprint)

   -- give it some personality...
   --local wall = blueprint:get_component('stonehearth:wall')
   --wall:set_width(10)
end

return BuildTest

