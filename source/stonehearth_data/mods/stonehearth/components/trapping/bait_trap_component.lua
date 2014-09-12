local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()

local BaitTrapComponent = class()

function BaitTrapComponent:initialize(entity, json)
   self._trap = entity
   self._sv = self.__saved_variables:get_data()

   if not self._sv.initialized then
      self._sv.is_armed = true
      self._sv.range = json.range or 16
      self._sv.initialized = true
      self.__saved_variables:mark_changed()
   else
   end
end

function BaitTrapComponent:destroy()
   self:release()
end

function BaitTrapComponent:arm()
   if not self._sv.is_armed then
      if self._sv.trapped_entity then
         self:release()
      end

      self._sv.is_armed = true
      self:_set_model_variant(self._trap, 'default')
      self.__saved_variables:mark_changed()
   end
end

-- entity is allowed to be nil
function BaitTrapComponent:trigger(entity)
   if not self._sv.is_armed then
      return
   end

   local trap = self._trap
   self._sv.is_armed = false
   self:_set_model_variant(trap, 'sprung')

   if entity and entity:is_valid() then
      local mob = entity:add_component('mob')

      self._sv.trapped_entity = entity
      self._sv.trapped_entity_collision_type = mob:get_mob_collision_type()

      -- change to collision type so we can place it inside the collision region
      mob:set_mob_collision_type(_radiant.om.Mob.NONE)

      radiant.entities.add_child(trap, entity, Point3.zero)

      -- warning: this will inject a compelled action that will pre-empt the current action
      radiant.entities.add_buff(entity, 'stonehearth:buffs:snared')

      radiant.events.trigger_async(entity, 'stonehearth:trapped')
   end

   self.__saved_variables:mark_changed()
end

function BaitTrapComponent:release()
   local trap = self._trap
   local trapped_entity = self._sv.trapped_entity

   if trapped_entity and trapped_entity:is_valid() then
      local mob = trapped_entity:add_component('mob')
      mob:set_mob_collision_type(self._sv.trapped_entity_collision_type)

      radiant.entities.remove_buff(trapped_entity, 'stonehearth:buffs:snared')

      -- move the trapped entity outside the trap
      local origin = mob:get_world_grid_location()
      local location = radiant.terrain.find_closest_standable_point_to(origin, 4, trapped_entity)
      radiant.terrain.place_entity(trapped_entity, location)

      self._sv.trapped_entity = nil
      self._sv.trapped_entity_collision_type = nil

      self.__saved_variables:mark_changed()
   end
end

function BaitTrapComponent:is_armed()
   return self._sv.is_armed
end

function BaitTrapComponent:can_trap(target)
   local faction = radiant.entities.get_faction(target)
   return faction == 'critter'
end

function BaitTrapComponent:try_trap(target)
   -- drive these probabilities from trap and critter difficulty levels
   local catch_chance = 0.50
   local trigger_on_failure_chance = 0.50

   if not self._sv.is_armed then
      return false
   end

   local trapped = rng:get_real(0, 1) < catch_chance

   if trapped then
      self:trigger(target)
   else
      local triggered = rng:get_real(0, 1) < trigger_on_failure_chance

      if triggered then
         self:trigger(nil)
      end
   end

   return self._sv.trapped_entity ~= nil
end

function BaitTrapComponent:get_trapped_entity()
   return self._sv.trapped_entity
end

function BaitTrapComponent:in_range(target)
   local distance = radiant.entities.distance_between(target, self._trap)
   return distance <= self._sv.range
end

function BaitTrapComponent:get_range()
   return self._sv.range
end

function BaitTrapComponent:set_range(range)
   self._sv.range = range
   self.__saved_variables:mark_changed()
end

function BaitTrapComponent:_set_model_variant(entity, key)
   local render_info = entity:add_component('render_info')
   render_info:set_model_variant(key)
end

return BaitTrapComponent
