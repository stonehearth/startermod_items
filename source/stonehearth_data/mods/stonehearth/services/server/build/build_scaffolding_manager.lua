local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3

local BuildScaffoldingManager = class()

function BuildScaffoldingManager:initialize()
   self._sv.ladder_builders = {}
   self.__saved_variables:mark_changed()
end


function BuildScaffoldingManager:request_ladder_to(to, normal, removable)
   local base = self:get_base_of_ladder_to(to)
   
   local ladder_builder = self._sv.ladder_builders[base:key_value()]
   if not ladder_builder then
      ladder_builder = radiant.create_controller('stonehearth:ladder_builder', self, base, normal, removable)
      self._sv.ladder_builders[base:key_value()] = ladder_builder
      self.__saved_variables:mark_changed()
   end
   ladder_builder:add_point(to)

   return radiant.lib.Destructor(function()
         ladder_builder:remove_point(to)
      end)
end

function BuildScaffoldingManager:remove_ladder(base)
   local ladder_builder = self._sv.ladder_builders[base:key_value()]
   if ladder_builder then
      ladder_builder:clear_all_points()
   end
end

-- returns the location where a ladder to reach `to` should be
-- built
function BuildScaffoldingManager:get_base_of_ladder_to(to)
   local base = Point3(to.x, to.y, to.z)
   local ladder_height = 0

   while self:_should_build_rung(base - Point3.unit_y) do
      base.y = base.y - 1
      ladder_height = ladder_height + 1
   end
   return base
end

function BuildScaffoldingManager:_should_build_rung(pt)
   local function is_empty_enough(entity)
      if entity:get_uri() == 'stonehearth:scaffolding' then
         return true
      end
      if entity:get_component('stonehearth:fabricator') then
         return true
      end
      return false
   end
   
   -- if the space isn't blocked, definitely build one.
   if not radiant.terrain.is_blocked(pt) then
      return true
   end

   local entities = radiant.terrain.get_entities_at_point(pt)
   for _, entity in pairs(entities) do
      if not is_empty_enough(entity) then
         return false
      end
   end
   return true
end

function BuildScaffoldingManager:_destroy_builder(base, ladder_builder)
   assert(self._sv.ladder_builders[base:key_value()] == ladder_builder)

   radiant.destroy_controller(ladder_builder)
   self._sv.ladder_builders[base:key_value()] = nil
   self.__saved_variables:mark_changed()
end

return BuildScaffoldingManager
