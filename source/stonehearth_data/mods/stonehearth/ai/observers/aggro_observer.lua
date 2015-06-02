local constants = require 'constants'
local log = radiant.log.create_logger('combat')

local AggroObserver = class()

function AggroObserver:initialize(entity)
   self._log = radiant.log.create_logger('combat')
                           :set_prefix('aggro_observer')
                           :set_entity(entity)

   self._entity = entity
   self._entity_traces = {}
   self._target_table = radiant.entities.get_target_table(self._entity, 'aggro')

   self:_add_sensor_trace()
   self:_add_amenity_trace()
end

function AggroObserver:destroy()
   self:_remove_sensor_trace()
   self:_remove_amenity_trace()
   self:_remove_all_entities_traces()
end

function AggroObserver:_add_sensor_trace()
   local sensor_list = self._entity:add_component('sensor_list')
   self._sensor = sensor_list:get_sensor('sight')
   assert(self._sensor)

   self._sensor_trace = self._sensor:trace_contents('aggro observer')
      :on_added(function(id, entity)
            self:_on_added_to_sensor(id, entity)
         end)
      :on_removed(function(id)
            self:_on_removed_from_sensor(id)
         end)
      :push_object_state()
end

function AggroObserver:_remove_sensor_trace()
   if self._sensor_trace then
      self._sensor_trace:destroy()
      self._sensor_trace = nil
   end
end

function AggroObserver:_add_amenity_trace()
   local pop = stonehearth.population:get_population(self._entity)
   if pop then
      -- what should we do when our amenity with some other faction changes?  we could easily recompute
      -- aggro for all our friends and foes, but what about people inside the sensor who were neutral that
      -- we didn't keep track of?  the simplest thing to do here is to start from scratch, so clear the
      -- target table and re-evaluate everything in the sensor.
      self._amenity_trace = radiant.events.listen(pop, 'stonehearth:amenity_changed', function()
            if self._sensor_trace then
               self._target_table:clear()
               self._sensor_trace:push_object_state()
            end
         end)
   end
end

function AggroObserver:_remove_amenity_trace()
   if self._amenity_trace then
      self._amenity_trace:destroy()
      self._amenity_trace = nil
   end
end

-- this may be called more than once for an entity, so make sure we can handle duplicates
function AggroObserver:_on_added_to_sensor(id, target)
   if not target or not target:is_valid() then
      return
   end

   -- we only track things that can be damaged / destroyed
   if not self:_is_killable(target) then
      return
   end

   self._log:spam('aggro sensor tracking %s', target)

   self:_add_entity_traces(target)
end

function AggroObserver:_on_removed_from_sensor(id)
   self:_remove_entity_traces(id)

   local target = radiant.entities.get_entity(id)

   if not target or not target:is_valid() then
      self._target_table:remove_entry(id)
   end
end

function AggroObserver:_add_entity_traces(target)
   local unit_info = target:get_component('unit_info')
   if not unit_info then
      return
   end

   local traces = {}

   traces.unit_info_trace = unit_info:trace_player_id('aggro observer')
      :on_changed(function()
            self:_update_target(target)
         end)

   self._entity_traces[target:get_id()] = traces

   self:_update_target(target)
end

function AggroObserver:_remove_entity_traces(id)
   local traces = self._entity_traces[id]
   if not traces then
      return
   end

   for name, trace in pairs(traces) do
      trace:destroy()
   end
   self._entity_traces[id] = nil
end

function AggroObserver:_remove_all_entities_traces()
   for id in pairs(self._entity_traces) do
      self:_remove_entity_traces(id)
   end
end

-- TODO: eventually when the killable state can change, add traces to track that here
function AggroObserver:_update_target(target)
   if stonehearth.player:are_players_friendly(target, self._entity) then
      -- listen for when your ally is hit
      self:_add_battery_trace(target)
      -- if the target was formerly hostile, remove it from the target table
      self._target_table:remove_entry(target:get_id())
      return
   end

   -- target is not friendly, remove the battery trace if there is one
   self:_remove_battery_trace(target)

   if stonehearth.player:are_players_hostile(target, self._entity) then
      -- make the target eligible to be attacked
      self:_add_hostile_to_aggro_table(target)
   end
end

function AggroObserver:_add_battery_trace(target)
   local traces = self._entity_traces[target:get_id()]

   if not traces.battery_trace then
      traces.battery_trace = radiant.events.listen(target, 'stonehearth:combat:battery', self, self._on_ally_battery)
   end
end

function AggroObserver:_remove_battery_trace(target)
   local traces = self._entity_traces[target:get_id()]

   if traces.battery_trace then
      traces.battery_trace:destroy()
      traces.battery_trace = nil
   end
end

function AggroObserver:_add_hostile_to_aggro_table(target)
   local value = self._target_table:get_value(target)
   if not value then
      -- the first time we see the target, we bump their aggro up by their menace attribute
      value = radiant.entities.get_attribute(target, 'menace', 1)
      self._target_table:modify_value(target, value)
   end
end

-- note that an entity is considered an ally of itself
function AggroObserver:_on_ally_battery(context)
   if not self:_is_killable(context.attacker) then
      return
   end

   -- we must be friendly with the target to care
   if not stonehearth.player:are_players_friendly(context.target, self._entity) then
      return
   end

   -- we must be hostile with the attacker to care
   if not stonehearth.player:are_players_hostile(context.attacker, self._entity) then
      return
   end

   local aggro = context.damage
   if context.target ~= self._entity then
      -- aggro from allies getting hit is less than self getting hit
      aggro = aggro * constants.combat.ALLY_AGGRO_RATIO
   end
   self._target_table:modify_value(context.attacker, aggro)
end

function AggroObserver:_is_killable(target)
   if not target or not target:is_valid() then
      self._log:spam('target is invalid in :_is_killable().  returning false')
      return false
   end

   local attributes_component = target:get_component('stonehearth:attributes')
   if not attributes_component then 
      self._log:spam('%s has no attributes component :_is_killable().  returning false', target)
      return false
   end

   local health = attributes_component:get_attribute('health')
   local is_killable = health and health > 0

   self._log:spam('%s health is %d in :_is_killable().  returning %s', target, tostring(health), tostring(is_killable))
   return is_killable
end

return AggroObserver
