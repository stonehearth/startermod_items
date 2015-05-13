local log = radiant.log.create_logger('goblin_campaign')

-- This observer looks for one enemy in particular, Ogo's mammoth, Mountain
-- If Mountain is an enemy, it kicks off the "tame Mammoth" task
-- If Mountain is friendly, it kicks off a "hang out" task
local MountainMammothObserver = class()

function MountainMammothObserver:initialize(entity)
   self._entity = entity
   self:_add_sensor_trace()
end

function MountainMammothObserver:_add_sensor_trace()
   local sensor_list = self._entity:add_component('sensor_list')
   self._sensor = sensor_list:get_sensor('sight')
   assert(self._sensor)

   self._trace = self._sensor:trace_contents('trace Mountain the Mammoth')
                                 :on_added(function(id, entity)
                                       self:_on_added_to_sensor(id, entity)
                                    end)
                              :push_object_state()
end

--TODO: What happens if he enters/leaves the sight sensor repeatedly?
--TODO: Does the function fire mutiple times if he's in the sensor continuously?
function MountainMammothObserver:_on_added_to_sensor(id, other_entity)
   if not other_entity or not other_entity:is_valid() then
      return
   end
   if other_entity:get_uri() == 'stonehearth:monsters:goblin:mammoth:mountain' then
      self._log:spam('%s has entered sight sensor', other_entity)

      --If Mountain is currently hostile, kick off the taming task
      if radiant.entities.is_hostile(other_entity, self._entity) then
         --TODO: kick off taming task. 
      end
   end
end

function MountainMammothObserver:destroy()
   if self._trace then
      self._trace:destroy()
      self._trace = nil
   end
end

return MountainMammothObserver