local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3

local log = radiant.log.create_logger('build.ladder')
local LadderManager = class()

function LadderManager:initialize()
   self._sv.ladder_builders = {}
   self._sv.scaffolding_builders = {}
   self._sv.next_id = 2
   self.__saved_variables:mark_changed()
end


-- xxx: this is only correct in a world where the terrain doesn't change. 
-- we should take a pass over all the ladder building code and make sure
-- it can handle cases where the ground around the latter starts moving.
function LadderManager:request_ladder_to(owner, to, normal, removable)
   log:detail('%s requesting ladder to %s', owner, to)
   if not self:_should_build_rung(to) then
      -- the destination is not actually a valid rung.  this will result in a 0
      -- height ladder!  we should probably make a ladder builder anyway and just
      -- not create it until the destination becomes reachable, but that's a project
      -- for another day (probably must be done before mining can be called "finished")
      log:detail('ignoring request: destination is not a valid rung position')
      return radiant.create_controller('radiant:destructor', radiant.bind('radiant.nop'))
   end
   
   log:detail('computing ladder base')
   local base = self:get_base_of_ladder_to(to)   
   local ladder_builder = self._sv.ladder_builders[base:key_value()]
   if not ladder_builder then
      local id = self:_get_next_id()

      log:detail('creating new ladder builder (lbid:%d)!', id)
      ladder_builder = radiant.create_controller('stonehearth:build:ladder_builder', self, id, owner, base, normal, removable)
      self._sv.ladder_builders[base:key_value()] = ladder_builder
      self.__saved_variables:mark_changed()
   end
   log:detail('adding %s to ladder builder (lbid:%d)', to, ladder_builder:get_id())
   ladder_builder:add_point(to)

   return radiant.create_controller('radiant:destructor',
                                    radiant.bind(ladder_builder, 'remove_point', to))
end

function LadderManager:remove_ladder(base)
   local ladder_builder = self._sv.ladder_builders[base:key_value()]
   if ladder_builder then
      ladder_builder:clear_all_points()
   end
end

-- returns the location where a ladder to reach `to` should be
-- built
function LadderManager:get_base_of_ladder_to(to)
   local base = Point3(to.x, to.y, to.z)
   local ladder_height = 0

   while self:_should_build_rung(base - Point3.unit_y) do
      base.y = base.y - 1
      ladder_height = ladder_height + 1
   end
   log:spam('returning ladders range of %s to %s', base, to)
   return base
end

function LadderManager:_should_build_rung(pt)
   local function is_empty_enough(entity)
      if entity:get_id() == 1 then
         log:spam('  - %s is terrain.  no.', entity)
         return false
      end
      if entity:get_uri() == 'stonehearth:build:prototypes:scaffolding' then
         log:spam('  - %s is scaffolding.  yes.', entity)
         return true
      end
      if entity:get_component('stonehearth:fabricator') then
         log:spam('  - %s is fabricator.  yes.', entity)
         return true
      end
      local rcs = entity:get_component('region_collision_shape')
      if not rcs or rcs:get_region_collision_type() == _radiant.om.RegionCollisionShape.NONE then
         log:spam('  - %s is non-opaque.  yes.', entity)
         return true
      end
      log:spam('  - %s has non-NONE collision shape.  no.', entity)
      return false
   end
   
   -- if the space isn't blocked, definitely build one.
   log:spam('checking all entities @ %s to see if we should build a rung.', pt);
   if not radiant.terrain.is_blocked(pt) then
      log:spam('  - point is not blocked.  yes.')
      return true
   end

   local entities = radiant.terrain.get_entities_at_point(pt)
   for _, entity in pairs(entities) do
      if not is_empty_enough(entity) then
         return false
      end
   end
   log:spam('  - no entities blocking!  yes.')
   return true
end

function LadderManager:_destroy_builder(base, ladder_builder)
   assert(self._sv.ladder_builders[base:key_value()] == ladder_builder)

   ladder_builder:destroy()
   self._sv.ladder_builders[base:key_value()] = nil
   self.__saved_variables:mark_changed()
end


function LadderManager:_get_next_id()
   local id = self._sv.next_id 
   self._sv.next_id = self._sv.next_id + 1
   self.__saved_variables:mark_changed()
   return id
end

return LadderManager
