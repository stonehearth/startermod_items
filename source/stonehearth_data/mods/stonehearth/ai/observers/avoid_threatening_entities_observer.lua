
local AvoidThreateningEntities = class()

function has_ai(entity)
   return entity and entity:is_valid() and entity:get_component('stonehearth:ai') ~= nil
end

function AvoidThreateningEntities:initialize(entity)
   self._entity = entity
   self._threats = {}
   self._traces = {}

   self._attributes_component = entity:add_component('stonehearth:attributes')
   radiant.events.listen(self._entity, 'stonehearth:attribute_changed:courage', self, self._on_courage_changed)

   -- TODO: make these configurable
   self._avoidance_radius = 16
   self._buffer_zone_radius = 24

   self:_add_sensor_trace()
end

function AvoidThreateningEntities:destroy()
   for id, trace in pairs(self._traces) do
      trace:destroy()
      self._traces[id] = nil
   end
end

function AvoidThreateningEntities:_add_sensor_trace()
   local sensor_list = self._entity:add_component('sensor_list')
   self._sensor = sensor_list:get_sensor('sight')
   assert(self._sensor)

   self._trace = self._sensor:trace_contents('trace non-friendly')
      :on_added(
         function (id)
            self:_on_added_to_sensor(id)
         end
      )
      :on_removed(
         function (id)
            self:_on_removed_from_sensor(id)
         end
      )
      :push_object_state()
end

-- this may be called more than once for an entity, so make sure we can handle duplicates
function AvoidThreateningEntities:_on_added_to_sensor(id)
   local other_entity = radiant.entities.get_entity(id)

   if not other_entity then
      return
   end

   -- TODO: eventually, we should listen for alliance change if a unit changes from ally to hostile or the reverse
   -- you can do this now by listening for events and then having that fn call _reevaluate_threat --sdee
   if self:_is_threatening(other_entity) then
      self:_observe_threat(other_entity)
      self:_check_avoid_entity(other_entity)
   end
end

function AvoidThreateningEntities:_on_removed_from_sensor(id)
   local other_entity = radiant.entities.get_entity(id)

   if not other_entity then
      return
   end

   self:_unobserve_threat(other_entity)
end

-- If the other person is hostile, and is threatening, then return true
-- Threat is true if the other entity's menace attribute is higher 
-- than your courage attribute. 
-- TODO: consider making this equation more complex, with saving throws etc
-- but for now it's super simple
function AvoidThreateningEntities:_is_threatening(other_entity)
   if other_entity == self._entity or not has_ai(other_entity) then
      return
   end

   local hostile = false
          
   if radiant.entities.is_hostile(other_entity, self._entity) then
      hostile = true
   end

   -- HACK until we have a reputation system in
   local own_faction = radiant.entities.get_faction(self._entity)
   local other_faction = radiant.entities.get_faction(other_entity)
   if own_faction == 'critter' and other_faction ~= own_faction then
      hostile = true
   end

   if hostile then
      local courage_attribute  = self._attributes_component:get_attribute('courage')
      local enemy_attribute = other_entity:add_component('stonehearth:attributes')
      local menace_attribute = enemy_attribute:get_attribute('menace')
      if courage_attribute and menace_attribute then
         return menace_attribute > courage_attribute
      end
   end
end

function AvoidThreateningEntities:_observe_threat(threat)
   local threat_id = threat:get_id()

   if not self._threats[threat_id] then
      self._threats[threat_id] = threat

      self._traces[threat_id] = radiant.entities.trace_location(threat, 'avoid non-friendly entities')
         :on_changed(
            function()
               self:_check_avoid_entity(threat)
            end
         )
   end
end

function AvoidThreateningEntities:_unobserve_threat(threat)
   local threat_id = threat:get_id()

   if self._threats[threat_id] then
      self._threats[threat_id] = nil

      self._traces[threat_id]:destroy()
      self._traces[threat_id] = nil
   end
end

-- When courage changes, our relationship to our threats change
-- Ooooo, deep. 
function AvoidThreateningEntities:_on_courage_changed()
   self:_reevaluate_threat()
end

--Check all the threats. Are we stills cared of them?
function AvoidThreateningEntities:_reevaluate_threat()
   for id, threat in pairs(self._threats) do
      if not self._is_threatening(threat) then
         self:_unobserve_threat(threat)
      end
   end
end

function AvoidThreateningEntities:_check_avoid_entity(threat)
   if self._avoid_entity_task then
      -- already executing
      return
   end

   local threat_distance

   if threat then
      threat_distance = radiant.entities.distance_between(threat, self._entity)
   else
      threat, threat_distance = self:_get_entity_to_avoid()
   end

   if threat then
      local run_distance = self._buffer_zone_radius - threat_distance
      self:_set_panicking_from(threat)
      --self:_create_avoid_entity_task(threat, run_distance)
   end
end

function AvoidThreateningEntities:_get_entity_to_avoid()
   local avoid_entity = nil
   local avoid_entity_distance = nil

   for id, threat in pairs(self._threats) do
      local threat_distance = radiant.entities.distance_between(threat, self._entity)

      if threat_distance <= self._avoidance_radius then
         if not avoid_entity or threat_distance < avoid_entity_distance then
            avoid_entity = threat
            avoid_entity_distance = threat_distance       
         end
      end
   end

   return avoid_entity, avoid_entity_distance
end

function AvoidThreateningEntities:_set_panicking_from(threat)
   stonehearth.combat:set_panicking_from(self._entity, threat)
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
