local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3

local BuildScaffoldingManager = class()

function BuildScaffoldingManager:initialize()
   self._sv.ladder_builders = {}
end

function BuildScaffoldingManager:request_ladder_to(to, normal)
   local base = self:get_base_of_ladder_to(to)
   
   local ladder_builder = self._sv.ladder_builders[base]
   if not ladder_builder then
      ladder_builder = radiant.create_controller('stonehearth:ladder_builder', base, normal)
      self._sv.ladder_builders[base] = ladder_builder
   end
   ladder_builder:add_point(to)

   return radiant.lib.Destructor(function()
         ladder_builder:remove_point(solved_cb)
         -- how does it get destroyed?
      end)
end

-- returns the location where a ladder to reach `to` should be
-- built
function BuildScaffoldingManager:get_base_of_ladder_to(to)
   local base = Point3(to.x, to.y, to.z)
   local ladder_height = 0
   while not radiant.terrain.is_standable(base) do
      base.y = base.y - 1
      ladder_height = ladder_height + 1
   end
   return base
end

return BuildScaffoldingManager
