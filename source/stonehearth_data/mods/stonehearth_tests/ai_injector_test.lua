local MicroWorld = require 'lib.micro_world'
local AiInjector = class(MicroWorld)
local Point3 = _radiant.csg.Point3

function AiInjector:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local worker = self:place_citizen(-12, 12)
   self:place_tree(12, -12)
   self:place_item('stonehearth.ball', 0, 0)

   self:at(1000, 
      function()
         --radiant.entities.goto_location(worker, Point3(12, 0, 2))
      end
   )
end

return AiInjector

