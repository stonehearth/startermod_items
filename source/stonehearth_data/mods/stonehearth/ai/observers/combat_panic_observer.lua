local log = radiant.log.create_logger('combat')

local CombatPanicObserver = class()

function CombatPanicObserver:initialize(entity)
   self._entity = entity
   self._panic_threshold = self:_get_panic_threshold()

   self._battery_listener = radiant.events.listen(self._entity, 'stonehearth:combat:battery', self, self._on_battery)
end

function CombatPanicObserver:destroy()
   self._battery_listener:destroy()
   self._battery_listener = nil
end

-- just using health right now, but this could depend on other attributes as well
function CombatPanicObserver:_on_battery(context)
   local health = radiant.entities.get_attribute(self._entity, 'health')
   local max_health = radiant.entities.get_attribute(self._entity, 'max_health')

   if health / max_health <= self._panic_threshold then
      stonehearth.combat:set_panicking_from(self._entity, context.attacker)
   end
end

function CombatPanicObserver:_get_panic_threshold()
   local equipment_component = self._entity:get_component('stonehearth:equipment')
   local data

   -- check if any items override the threshold and return the first found
   if equipment_component ~= nil then
      local items = equipment_component:get_all_items()
      for _, item in pairs(items) do
         data = radiant.entities.get_entity_data(item, 'stonehearth:panic_threshold')
         if data then
            return data
         end
      end
   end

   -- check if the threshold exists on the entity itself
   data = radiant.entities.get_entity_data(self._entity, 'stonehearth:panic_threshold')
   if data then
      return data
   end

   -- return the default if none found
   return radiant.util.get_config('default_panic_threshold', 0.25)
end

return CombatPanicObserver
