
local AvoidThreateningEntities = class()

function has_ai(entity)
   return entity and entity:is_valid() and entity:get_component('stonehearth:ai') ~= nil
end

-- inner helper class
local ThreatTracker = class()

function ThreatTracker:__init(threat)
   self._threat = threat
   self._traces = {}
end

function ThreatTracker:get_threat()
   return self._threat
end

function ThreatTracker:add_trace(name, trace)
   self._traces[name] = trace
end

function ThreatTracker:destroy()
   for name, trace in pairs(self._traces) do
      trace:destroy()
      self._traces[name] = nil
   end
end

function AvoidThreateningEntities:initialize(entity)
   self._entity = entity
   self._entity_traces = {}
   self._threats = {}

   local entity_data = radiant.entities.get_entity_data(self._entity, 'stonehearth:observers:avoid_threatening_entities')

   if entity_data and entity_data.treat_neutral_as_hostile ~= nil then
      self._treat_neutral_as_hostile = entity_data.treat_neutral_as_hostile
   else
      self._treat_neutral_as_hostile = false
   end

   self._min_avoidance_distance = entity_data and entity_data.min_avoidance_distance or 16
   self._max_avoidance_distance = entity_data and entity_data.max_avoidance_distance or 32
   self._courage = self._entity:add_component('stonehearth:attributes'):get_attribute('courage') or 0

   self:_add_attribute_traces()
   self:_add_sensor_trace()
   self:_add_panic_trace()
end

function AvoidThreateningEntities:destroy()
   for id, threat_tracker in pairs(self._threats) do
      threat_tracker:destroy()
      self._threats[id] = nil
   end

   self:_destroy_entity_traces()
end

function AvoidThreateningEntities:_destroy_entity_traces()
   for name, trace in pairs(self._entity_traces) do
      trace:destroy()
      self._entity_traces[name] = nil
   end
end

function AvoidThreateningEntities:_add_sensor_trace()
   local sensor_list = self._entity:add_component('sensor_list')
   self._sensor = sensor_list:get_sensor('sight')
   assert(self._sensor)

   local sensor_trace = self._sensor:trace_contents('trace threatening entities')
      :on_added(function(id)
            self:_on_added_to_sensor(id)
         end
      )
      :on_removed(function(id)
            self:_on_removed_from_sensor(id)
         end
      )
      :push_object_state()

   self._entity_traces['sensor'] = sensor_trace
end

function AvoidThreateningEntities:_add_panic_trace()
   local panic_trace = radiant.events.listen(self._entity, 'stonehearth:combat:panic_changed', self, self._on_panic_changed)
   self._entity_traces['panic'] = panic_trace
end

function AvoidThreateningEntities:_add_attribute_traces()
   local courage_trace = radiant.events.listen(self._entity, 'stonehearth:attribute_changed:courage', self, self._on_courage_changed)
   self._entity_traces['courage'] = courage_trace
end

-- this may be called more than once for an entity, so make sure to handle duplicates
function AvoidThreateningEntities:_on_added_to_sensor(id)
   local other_entity = radiant.entities.get_entity(id)

   if not other_entity then
      return
   end

   -- TODO: eventually, we should listen for faction/reputation changes in case a unit changes from ally to hostile or the reverse
   if self:_is_threatening(other_entity) then
      self:_observe_threat(other_entity)
      self:_check_avoid_entity(other_entity)
   end
end

function AvoidThreateningEntities:_on_removed_from_sensor(id)
   self:_unobserve_threat(id)
end

function AvoidThreateningEntities:_on_panic_changed()
   local threat = stonehearth.combat:get_panicking_from(self._entity)
   if not threat or not threat:is_valid() then
      self:_check_avoid_entity()
   end
end

function AvoidThreateningEntities:_on_courage_changed()
   local new_courage = self._entity:add_component('stonehearth:attributes'):get_attribute('courage') or 0

   if new_courage == self._courage then
      return
   end
      
   self._courage = new_courage

   for id, other_entity in self._sensor:each_contents() do
      -- can be made faster by checking direction of courage change and operating on subset
      if self:_is_threatening(other_entity) then
         self:_observe_threat(other_entity)
      else
         self:_unobserve_threat(other_entity:get_id())
      end
   end

   self:_check_avoid_entity()
end

function AvoidThreateningEntities:_on_threat_location_changed(threat)
   self:_check_avoid_entity(threat)
end

function AvoidThreateningEntities:_on_threat_menace_changed(threat)
   if self:_is_threatening(threat) then
      self:_check_avoid_entity(threat)
   else
      self:_unobserve_threat(threat:get_id())
   end
end

function AvoidThreateningEntities:_is_threatening(other_entity)
   local menace = self:_get_menace(other_entity)
   if menace <= self._courage then
      return false
   end

   local hostile = self:_is_hostile(other_entity)
   if not hostile then
      return false
   end

   return true
end

function AvoidThreateningEntities:_get_menace(other_entity)
   local attributes_component = other_entity:get_component('stonehearth:attributes')
   local menace = attributes_component and attributes_component:get_attribute('menace') or 0
   return menace
end

function AvoidThreateningEntities:_is_hostile(other_entity)
   if other_entity == self._entity then
      return false
   end

   if self._treat_neutral_as_hostile then
      return not radiant.entities.is_friendly(other_entity, self._entity)
   else
      return radiant.entities.is_hostile(other_entity, self._entity)
   end
end

function AvoidThreateningEntities:_observe_threat(threat)
   local threat_id = threat:get_id()

   if not self._threats[threat_id] then
      local threat_tracker = ThreatTracker(threat)

      local location_trace = radiant.entities.trace_grid_location(threat, 'avoid threatening entities')
         :on_changed(function()
               self:_on_threat_location_changed(threat)
            end
         )
      threat_tracker:add_trace('location', location_trace)
      
      local menace_trace = radiant.events.listen(threat, 'stonehearth:attribute_changed:menace', function()
            self:_on_threat_menace_changed(threat)
         end
      )
      threat_tracker:add_trace('menace', location_trace)

      self._threats[threat_id] = threat_tracker
   end
end

function AvoidThreateningEntities:_unobserve_threat(id)
   local threat_tracker = self._threats[id]

   if threat_tracker then
      threat_tracker:destroy()
      self._threats[id] = nil
   end
end

function AvoidThreateningEntities:_destroy_traces(threat_tracker)
   for key, trace in pairs(threat_tracker.traces) do
      trace:destroy()
      threat_tracker.traces[key] = nil
   end
end

function AvoidThreateningEntities:_get_threat_ratio(threat)
   local menace = self:_get_menace(threat)
   local threat_ratio = menace / self._courage
   return threat_ratio
end

function AvoidThreateningEntities:_get_avoidance_distance(threat_ratio)
   local avoidance_distance = math.min(self._min_avoidance_distance * threat_ratio, self._max_avoidance_distance)
   return avoidance_distance
end

function AvoidThreateningEntities:_check_avoid_entity(threat)
   if self:_currently_avoiding() then
      -- already executing
      return
   end

   if threat then
      -- checking this entity only
      local threat_distance = radiant.entities.distance_between(threat, self._entity)
      local threat_ratio = self:_get_threat_ratio(threat)
      local avoidance_distance = self:_get_avoidance_distance(threat_ratio)

      if threat_distance > avoidance_distance then
         return
      end
   else
      -- check all threats in sensor range
      threat = self:_get_entity_to_avoid()
   end

   if threat then
      self:_avoid_entity(threat)
      --local run_distance = self._buffer_zone_radius - threat_distance
      --self:_create_avoid_entity_task(threat, run_distance)
   end
end

function AvoidThreateningEntities:_get_entity_to_avoid()
   local greatest_threat = nil
   local greatest_threat_score = nil

   for id, threat_tracker in pairs(self._threats) do
      local threat = threat_tracker:get_threat()

      -- if not valid, we'll wait for the sensor to remove it
      if threat:is_valid() then
         local threat_distance = radiant.entities.distance_between(threat, self._entity)
         local threat_ratio = self:_get_threat_ratio(threat)
         local avoidance_distance = self:_get_avoidance_distance(threat_ratio)

         if threat_distance <= avoidance_distance then
            local threat_score = threat_ratio * 100 / threat_distance -- * 100 to make it easier for humans to read

            if not greatest_threat or threat_score > greatest_threat_score then
               greatest_threat = threat
               greatest_threat_score = threat_score
            end
         end
      end
   end

   return greatest_threat
end

function AvoidThreateningEntities:_avoid_entity(threat)
   stonehearth.combat:set_panicking_from(self._entity, threat)
end

function AvoidThreateningEntities:_currently_avoiding()
   local threat = stonehearth.combat:get_panicking_from(self._entity)
   return threat and threat:is_valid()
end

-- consider re-writing all panic as an injected task
-- function AvoidThreateningEntities:_create_avoid_entity_task(threat, run_distance)
--    self._avoid_entity_task = self._entity:add_component('stonehearth:ai')
--       :get_task_group('stonehearth:combat')
--       :create_task('stonehearth:combat:panic', { threat = threat })
--       :set_priority(stonehearth.constants.priorities.combat.PANIC)
--       :once()
--       :notify_completed(
--          function ()
--             self._avoid_entity_task = nil
--             self:_check_avoid_entity()
--          end
--       )
--       :start()
-- end

return AvoidThreateningEntities
