local log = radiant.log.create_logger('combat')

local FindTargetObserver = class()

function FindTargetObserver:__init()
   self._enable_combat = radiant.util.get_config('enable_combat', true)
end

function FindTargetObserver:initialize(entity)
   self._entity = entity
   self._sight_sensor = self:_get_sight_sensor()

   self._sight_sensor_trace = self._sight_sensor:trace_contents('find target obs')
                                                   :on_added(function (id, entity)
                                                         self:_check_for_target()
                                                      end)
                                                   :on_removed(function (id)
                                                         self:_check_for_target()
                                                      end)

   self._target = nil
   self._last_attacker = nil
   self._last_attacked_time = 0
   self._retaliation_window = 5000
   self._task = nil
   self._listening_target_pre_destroy = false

   self._log = radiant.log.create_logger('combat')
                           :set_prefix('find_target_obs')
                           :set_entity(self._entity)

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
   self._aggro_table = self._entity:add_component('stonehearth:target_tables')
                                       :get_target_table('aggro')
   self._combat_state = self._entity:add_component('stonehearth:combat_state')
   self._table_change_listener = radiant.events.listen(self._aggro_table, 'stonehearth:target_table_changed', self, self._on_target_table_changed)
   self._stance_change_listener = radiant.events.listen(self._entity, 'stonehearth:combat:stance_changed', self, self._on_stance_changed)
   self._assault_listener = radiant.events.listen(self._entity, 'stonehearth:combat:assault', self, self._on_assault)
   self:_trace_entity_location()
   -- listen for stance change
end

function FindTargetObserver:_unsubscribe_from_events()
   self._sight_sensor_trace:destroy()
   self._sight_sensor_trace = nil

   self._stance_change_listener:destroy()
   self._stance_change_listener = nil

   self._assault_listener:destroy()
   self._assault_listener = nil

   -- This unlisten may log 'unlisten stonehearth:target_table_changed on unknown sender'
   -- when the target table component is destroyed before this observer, thus forcing an unpublish
   -- before we can unlisten. This is ok.
   self._table_change_listener:destroy()
   self._table_change_listener = nil

   self:_destroy_entity_location_trace()
end

function FindTargetObserver:_on_target_table_changed()
   -- TODO: don't check for target on every target table change
   -- We have O(n) listeners to target_table_changed
   -- We have O(n) triggers to target_table_changed
   -- Check_for_target is O(n)
   -- Combat is O(n^3) - too expensive
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
   self._trace_entity_location = radiant.entities.trace_grid_location(self._entity, 'find target observer')
      :on_changed(function()
            self:_on_grid_location_changed()
         end)
end

function FindTargetObserver:_destroy_entity_location_trace()
   if self._trace_entity_location then
      self._trace_entity_location:destroy()
      self._trace_entity_location = nil
   end
end

function FindTargetObserver:_listen_for_target_pre_destroy()
   if self._target and self._target:is_valid() then
      self._pre_destroy_listener = radiant.events.listen(self._target, 'radiant:entity:pre_destroy', self, self._on_target_pre_destroy)
      self._listening_target_pre_destroy = true
   end
end

function FindTargetObserver:_unlisten_from_target_pre_destroy()
   if self._listening_target_pre_destroy then
      -- because events are asynchronous, target may have already been destroyed
      self._pre_destroy_listener:destroy()
      self._pre_destroy_listener = nil
      self._listening_target_pre_destroy = false
   end
end

function FindTargetObserver:_check_for_target()
   if not self._entity:is_valid() then
      return
   end

   if self:_do_not_disturb() then
      self._log:spam('DND is set.  skipping target check...')
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
         self._log:info('switching targets from %s to %s', self._target, target)
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

   self._log:info('setting target to %s', tostring(target))

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
      self._log:info('stance is passive.  returning nil target.')
      return nil
   end

   if stance == 'defensive' then
      -- only attack those who attack you
      target = self:_get_retaliation_target()
   else
      assert(stance == 'aggressive')
      -- find the best target to attack
      target = self._aggro_table:get_top(function(entity, aggro)
            return self:_target_cost_benefit(entity, aggro)
         end)
   end

   self._log:info('stance is %s.  returning %s as target.', stance, tostring(target))
   
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

function FindTargetObserver:_target_cost_benefit(target, aggro)
   if not self:_can_see_target(target) then
      -- can't see?  not a candidate
      self._log:spam('considering target %s (aggro:%.2f .. cannot see!  ignoring)', target, aggro)
      return
   end
   if not self._combat_state:is_within_leash(target) then
      -- outside of leash?  don't run there!
      self._log:spam('considering target %s (aggro:%.2f .. too far away from leash!  ignoring)', target, aggro)
      return
   end
   local distance = radiant.entities.distance_between(self._entity, target)
   local score = aggro / distance
   self._log:spam('considering target %s (score:%.2f - aggro:%.2f distance:%.2f)', target, score, aggro, distance)
   return score
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
