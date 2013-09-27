local SensorAiInjector = class()

function SensorAiInjector:__init(entity, data_binding)
   self._entity = entity
end

function SensorAiInjector:extend(json)
   assert(json.sensor)
   assert(json.ai)

   self._sensor = self._entity:get_component('sensor_list'):get_sensor(json.sensor)   
   self._ai = json.ai

   self.promise = self._sensor:get_contents():trace()

   self.promise:on_added(
      function (id)
         self:on_added_to_sensor(id)
      end
   ):on_removed(
      function (id)
         self:on_removed_to_sensor(id)
      end
   ) 
end

function SensorAiInjector:on_added_to_sensor(entity_id)
   local entity = radiant.entities.get_entity(entity_id)
   
   radiant.log.info('%s entered ai transmission sensor for %s', tostring(entity), tostring(self._entity))
   self._injected_actions_token = radiant.ai.inject_ai(entity, self._entity, self._ai)
end

function SensorAiInjector:on_removed_to_sensor(entity_id)
   local entity = radiant.entities.get_entity(entity_id)
   radiant.log.info('%s left ai transmission sensor for %s', tostring(entity), tostring(self._entity))
   
   radiant.ai.remove_inject_ai(self._injected_actions_token)
end

return SensorAiInjector