local log = radiant.log.create_logger('combat')

local FindTargetObserver = class()

function FindTargetObserver:__init()
   self._enable_combat = radiant.util.get_config('enable_combat', true)
end

function FindTargetObserver:initialize(entity, json)
   self._entity = entity
   self._sight_sensor = self:_get_sight_sensor()
   self._target = nil
   self._last_attacker = nil
   self._last_attacked_time = 0
   self._retaliation_window = 5000
   self._task = nil
   self._listening_target_pre_destroy = false

   self:_subscribe_to_events()
   self:_check_for_target()
end

function FindTargetObserver:destroy()
   self:_unsubscribe_from_events()
end

function FindTargetObserver:_get_sight_sensor()
   local sensor_list = self._entity:add_component('sensor_list')
   local sight_sensor = sensor_list:get_sensor('sight')
   -- might be ok for a unit not to have a sight sensor, but assert for now
   assert(sight_sensor and sight_sensor:is_valid())
   return sight_sensor
end

function FindTargetObserver:_subscribe_to_events()
   self._aggro_table = self._entity:add_component('stonehearth:target_tables'):get_target_table('aggro')
   radiant.events.listen(self._aggro_table, 'stonehearth:target_table_changed', self, self._on_target_table_changed)
   radiant.events.listen(self._entity, 'stonehearth:combat:stance_changed', self, self._on_stance_changed)
   radiant.events.listen(self._entity, 'stonehearth:combat:assault', self, self._on_assault)
   self:_trace_entity_location()
   -- listen for stance change
end

function FindTargetObserver:_unsubscribe_from_events()
   radiant.events.unlisten(self._aggro_table, 'stonehearth:target_table_changed', self, self._on_target_table_changed)
   radiant.events.unlisten(self._entity, 'stonehearth:combat:stance_changed', self, self._on_stance_changed)
   radiant.events.unlisten(self._entity, 'stonehearth:combat:assault', self, self._on_assault)
   self:_destroy_entity_location_trace()
end

function FindTargetObserver:_on_target_table_changed()
   self:_check_for_target()
end

function FindTargetObserver:_on_stance_changed()
   self:_check_for_target()
end

function FindTargetObserver:_on_target_pre_destroy()
   self:_check_for_target()
end

function FindTargetObserver:_on_assault(context)
   self._last_attacker = context.attacker
   self._last_attacked_time = radiant.gamestate.now()
   self:_check_for_target()
end

function FindTargetObserver:_on_grid_location_changed()
   if self._target then
      -- check if there is a better target
      self:_check_for_target()
   else
      -- do nothing, let another event initiate a target
   end
end

function FindTargetObserver:_trace_entity_location()
   local mob = self._entity:add_component('mob')
   local last_location = self._entity:add_component('mob'):get_world_grid_location()

   local on_moved = function()
      local current_location = mob:get_world_grid_location()
      if current_location ~= last_location then
         self:_on_grid_location_changed()
      end
      last_location = current_location
   end

   self._trace_entity_location = radiant.entities.trace_location(self._entity, 'find target observer')
      :on_changed(on_moved)
end

function FindTargetObserver:_destroy_entity_location_trace()
   if self._trace_entity_location then
      self._trace_entity_location:destroy()
      self._trace_entity_location = nil
   end
end

function FindTargetObserver:_listen_for_target_pre_destroy()
   if self._target and self._target:is_valid() then
      radiant.events.listen(self._target, 'radiant:entity:pre_destroy', self, self._on_target_pre_destroy)
      self._listening_target_pre_destroy = true
   end
end

function FindTargetObserver:_unlisten_from_target_pre_destroy()
   if self._listening_target_pre_destroy then
      radiant.events.unlisten(self._target, 'radiant:entity:pre_destroy', self, self._on_target_pre_destroy)
      self._listening_target_pre_destroy = false
   end
end

function FindTargetObserver:_check_for_target()
   if not self._entity:is_valid() then
      return
   end

   if self:_do_not_disturb() then
      -- don't interrupt an assault in progress
      return
   end

   -- ok for target to be nil. we may be abandoning the target or the target may be dying
   local target = self:_find_target()

   if self._task then
      -- self._task should be nil if it is completed
      assert(not self._task:is_completed())

      if target == self._target then
         -- same target, let the existing task run
         return
      end

      -- terminate task so we can initiate a new task with the preferred target
      if target and target:is_valid() and self._target and self._target:is_valid() then
         log:info('%s switching targets from %s to %s', self._entity, self._target, target)
      end

      self._task:destroy()
      self._task = nil
   end

   self:_attack_target(target)
end

function FindTargetObserver:_do_not_disturb()
   local assaulting = stonehearth.combat:get_assaulting(self._entity)
   return assaulting
end

function FindTargetObserver:_attack_target(target)
   assert(not self._task)

   if target ~= self._target then
      self:_unlisten_from_target_pre_destroy()
      stonehearth.combat:set_primary_target(self._entity, target)
      self._target = target
      self:_listen_for_target_pre_destroy()
   end

   if target and target:is_valid() then
      self._task = self._entity:add_component('stonehearth:ai')
                               :get_task_group('stonehearth:combat')
                               :create_task('stonehearth:combat:attack_after_cooldown', { target = target })
                               :set_priority(stonehearth.constants.priorities.combat.ACTIVE)
                               :once()
                               :notify_completed(
                                 function ()
                                    self._task = nil
                                    self:_check_for_target()
                                 end
                               )
                               :start()
   end
end

function FindTargetObserver:_find_target()
   if not self._enable_combat then
      return
   end

   local stance = stonehearth.combat:get_stance(self._entity)
   local target

   if stance == 'passive' then
      -- don't attack
      return nil
   end

   if stance == 'defensive' then
      -- only attack those who attack you
      target = self:_get_retaliation_target()
   else
      assert(stance == 'aggressive')
      -- find the best target to attack
      target = self:_calculate_target_cost_benefit()
   end

   if target ~= nil and target:is_valid() then
      return target
   end

   return nil
end

function FindTargetObserver:_can_see_target(target)
   if not target or not target:is_valid() then
      return false
   end

   local visible = self._sight_sensor:contains_contents(target:get_id())
   return visible
end

-- calculate a crude cost/benefit ratio so we don't switch targets over long distances unless it really matters
function FindTargetObserver:_calculate_target_cost_benefit()
   local targets = self._aggro_table:get_targets()
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

function FindTargetObserver:_get_retaliation_target()
   local now = radiant.gamestate.now()
   if now < self._last_attacked_time + self._retaliation_window then
      return self._last_attacker
   else
      return nil
   end
end

return FindTargetObserver
