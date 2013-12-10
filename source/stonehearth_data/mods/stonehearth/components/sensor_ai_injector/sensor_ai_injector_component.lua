local SensorAiInjectorComponent = class()

function SensorAiInjectorComponent:__init(entity)
   self._entity = entity
   self._injected_ais = {}
end

function SensorAiInjectorComponent:extend(json)
   assert(json.sensor)
   assert(json.ai)

   self._sensor = self._entity:get_component('sensor_list'):get_sensor(json.sensor)   
   self._ai = json.ai

   self.promise = self._sensor:trace_contents('ai injector')
                                    :on_added(
                                       function (id, entity)
                                          self:on_added_to_sensor(id, entity)
                                       end
                                    )
                                    :on_removed(
                                       function (id)
                                          self:on_removed_from_sensor(id)
                                       end
                                    ) 
end

function SensorAiInjectorComponent:on_added_to_sensor(entity_id, entity)
   local ai_component = entity:get_component('stonehearth:ai')
   if ai_component then
      radiant.log.info('%s entered ai transmission sensor for %s', tostring(entity), tostring(self._entity))
      local ai_service = radiant.mods.load('stonehearth').ai      
      self._injected_ais[entity_id] = ai_service:inject_ai(entity, self._ai, self._entity)
   end
end

function SensorAiInjectorComponent:on_removed_from_sensor(entity_id)
   local injected_ai = self._injected_ais[entity_id]
   if injected_ai then
      radiant.log.info('entity %d left ai transmission sensor for %s', entity_id, tostring(self._entity))  
      injected_ai:destroy()
      self._injected_ais[entity_id] = nil
   end
end

return SensorAiInjectorComponent