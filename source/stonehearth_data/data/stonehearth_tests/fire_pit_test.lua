local Point3 = _radiant.csg.Point3
local MicroWorld = require 'lib.micro_world'
local FirePitTest = class(MicroWorld)

function FirePitTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local tree = self:place_tree(-12, -12)
   --local worker = self:place_citizen(-8, -8)
   local worker = self:place_citizen(3, -4)
   
   self:place('stonehearth_items.fire_pit', 1, 1)
   self:place('stonehearth_trees.oak_log', 1, 1)
   
end

function FirePitTest:place(entity_string, x, z)
   local rabbit_entity = radiant.entities.create_entity(entity_string)
   local location = Point3(x, 0, z)

   radiant.terrain.place_entity(rabbit_entity, location)
end


return FirePitTest

