local Entity = _radiant.om.Entity
local log = radiant.log.create_logger('combat')

local FindTarget = class()
FindTarget.name = 'find target'
FindTarget.does = 'stonehearth:combat:find_target'
FindTarget.args = {}
FindTarget.think_output = {
   target = Entity,
}
FindTarget.version = 2
FindTarget.priority = 1

function FindTarget:__init()
   self._enable_combat = radiant.util.get_config('enable_combat', true)
end

function FindTarget:start_thinking(ai, entity, args)
   if not self._enable_combat then
      return
   end

   self._ai = ai
   self._entity = entity
   self._registered = false

   local sensor_list = self._entity:add_component('sensor_list')
   self._sight_sensor = sensor_list:get_sensor('sight')
   -- might be ok for a unit not to have a sight sensor, but assert for now
   assert(self._sight_sensor and self._sight_sensor:is_valid())

   local target = self:_get_target()

   if target ~= nil and target:is_valid() then
      ai:set_think_output({ target = target })
      return
   end

   self:_register_events()
end

function FindTarget:stop_thinking(ai, entity, args)
   self:_unregister_events()
   self._ai = nil
   self._entity = nil
end

function FindTarget:_register_events()
   if not self._registered then
      -- TODO: replace slow_poll with a listen on target table changed
      -- TODO: have engage add the engager to the target table
      radiant.events.listen(radiant, 'stonehearth:slow_poll', self, self._on_find_target)
      -- radiant.events.listen(self._entity, 'stonehearth:combat:engage', self, self._on_engage)
      self._registered = true
   end
end

function FindTarget:_unregister_events()
   if self._registered then
      radiant.events.unlisten(radiant, 'stonehearth:slow_poll', self, self._on_find_target)
      -- radiant.events.unlisten(self._entity, 'stonehearth:combat:engage', self, self._on_engage)
      self._registered = false
   end
end

-- function FindTarget:_on_engage(args)
--    local target = args.attacker

--    if target ~= nil and target:is_valid() then
--       self._ai:set_think_output({ target = args.attacker })
--       self:_unregister_events()
--    end
-- end

function FindTarget:_on_find_target()
   local target = self:_get_target()

   if target ~= nil and target:is_valid() then
      self._ai:set_think_output({ target = target })
      self:_unregister_events()
   end
end

function FindTarget:_get_target()
   local stance = stonehearth.combat:get_stance(self._entity)
   if stance == 'passive' then
      return nil
   end

   --return self:_calculate_target_simple()
   return self:_calculate_target_cost_benefit()
end

function FindTarget:_can_see_target(target)
   if not target or not target:is_valid() then
      return false
   end

   local visible = self._sight_sensor:contains_contents(target:get_id())
   return visible
end

-- calculate a crude cost/benefit ratio so we don't switch targets over long distances unless it really matters
function FindTarget:_calculate_target_cost_benefit()
   local target_table = radiant.entities.get_target_table(self._entity, 'aggro')
   local targets = target_table:get_targets()
   local best_target = nil
   local best_score = -1

   for target_id, aggro in pairs(targets) do
      local target = radiant.entities.get_entity(target_id)
      if self:_can_see_target(target) then
         local distance = radiant.entities.distance_between(self._entity, target)
         local score = aggro / distance
         if score > best_score then
            best_target = target
            best_score = score
         end
      end
   end

   return best_target
end

function FindTarget:_calculate_target_simple()
   local target_table = radiant.entities.get_target_table(self._entity, 'aggro')
   local target, score = target_table:get_top()

   -- if the highest score is <= 1 (minimal/no history on any of the targets), the pick the closest
   if target and score <= 1 then
      local targets = target_table:get_targets()
      target = self:_get_closest_target(targets)
   end

   return target
end

function FindTarget:_get_closest_target(targets)
   local target, target_distance, closest_distance
   local closest = nil

   for target_id in pairs(targets) do
      target = radiant.entities.get_entity(target_id)
      if target ~= nil and target:is_valid() then
         target_distance = radiant.entities.distance_between(self._entity, target)
         if closest == nil or target_distance < closest_distance then
            closest = target
            closest_distance = target_distance
         end
      end
   end
   return closest
end

return FindTarget
